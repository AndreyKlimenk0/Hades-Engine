#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <map>
#include <string>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define HT_PRIME_1 151
#define HT_PRIME_2 163

int hash(const char* string, const int factor, const int table_count)
{
	long hash = 0;
	const int len = strlen(string);
	for (int i = 0; i < len; i++) {
		hash += (long)pow(factor, len - (i + 1)) * string[i];
		hash = hash % table_count;
	}
	return (int)hash;
}

int double_hash(const char* string, const int table_count, const int attempt)
{
	const int hash_a = hash(string, HT_PRIME_1, table_count);
	const int hash_b = hash(string, HT_PRIME_2, table_count);
	return (hash_a + (attempt * (hash_b + 1))) % table_count;
}

template <typename _Key_, typename _Value_>
struct Hash_Node {
	_Key_ key;
	_Value_ value;
	Hash_Node(const _Key_ &key, const _Value_ &value) : key(key), value(value) {}
};

template <typename _Key_, typename _Value_>
struct Hash_Table {
	Hash_Node<_Key_, _Value_> **nodes;
	int size;
	int count;
	
	Hash_Table(int _size = 53);
	~Hash_Table();

	_Value_ & operator[](const _Key_ &key);
	//_Value_ &operator[](const _Key_ &key, const _Value_ &value);
	void set(const _Key_ &key, const _Value_ &value);
	bool get(const _Key_ &key, _Value_ &value);
};

template <typename _Key_, typename _Value_>
Hash_Table<_Key_, _Value_>::Hash_Table(int _size)
{
	size = _size;
	count = 0;
	nodes = new Hash_Node<_Key_, _Value_>*[size];
	memset(nodes, 0, sizeof(Hash_Node<_Key_, _Value_> *) * size);
}

template <typename _Key_, typename _Value_>
Hash_Table<_Key_, _Value_>::~Hash_Table()
{
	delete nodes;
	nodes = NULL;
}

template <typename _Key_, typename _Value_>
_Value_ &Hash_Table<_Key_, _Value_>::operator[](const _Key_ &key)
{
	_Value_ value;
	get(key, value);
	return value;
}

template <typename _Key_, typename _Value_>
void Hash_Table<_Key_, _Value_>::set(const _Key_ &key, const _Value_ &value)
{
	int index = double_hash(key, size, 0);
	Hash_Node<_Key_, _Value_> *node = nodes[index];
	int i = 1;
	while (node != NULL) {
		index = double_hash(key, size, i);
		node = &node[index];
		i++;
	}
	nodes[index] = new Hash_Node<_Key_, _Value_>(key, value);
	count++;
}

template <typename _Key_, typename _Value_>
bool Hash_Table<_Key_, _Value_>::get(const _Key_ &key, _Value_ &value)
{
	int index = double_hash(key, size, 0);
	Hash_Node<_Key_, _Value_> *node = nodes[index];
	int i = 1;
	while (node != NULL) {
		if (!strcmp(key, node->key)) {
			value = node->value;
			return true;
		}
		index = double_hash(key, size, i);
		node = &node[index];
		i++;
	}
	return false;
}
#endif