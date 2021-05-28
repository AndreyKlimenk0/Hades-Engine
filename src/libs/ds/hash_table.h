#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../../sys/sys_local.h"

#define HT_PRIME_1 151
#define HT_PRIME_2 163


int hash(const char* string, const int factor, const int table_count);
int double_hash(const char* string, const int table_count, const int attempt);

template <typename _Key_, typename _Value_>
struct Hash_Node {
	_Key_ key;
	_Value_ value;
	Hash_Node(const _Key_ &key, const _Value_ &value) : key(key), value(value) {}
	bool compare(const _Key_ &key2) { return key == key2; }
};

template <typename _Value_>
struct Hash_Node<const char *, _Value_> {
	String key;
	_Value_ value;
	Hash_Node(const char *key, const _Value_ &value) : key(key), value(value) {}
	bool compare(const String &key2) { return key == key2; }
};

template <typename _Value_>
struct Hash_Node<String, _Value_> {
	String key;
	_Value_ value;
	Hash_Node(const String &key, const _Value_ &value) : key(key), value(value) {}
	bool compare(const String key2) { return key == key2; }
};

template <typename _Key_, typename _Value_>
struct Hash_Table {
	Hash_Table(int _size = 53);
	~Hash_Table();

	Hash_Node<_Key_, _Value_> **nodes;
	int size;
	int count;

	void set(const _Key_ &key, const _Value_ &value);
	bool get(const _Key_ &key, _Value_ &value);
	bool get(const _Key_ &key, _Value_ *value);

	_Value_ &operator[](const _Key_ &key);
};

template<typename _Key_, typename _Value_>
_Value_ &Hash_Table<_Key_, _Value_>::operator[](const _Key_ &key)
{
	int index = double_hash(key, size, 0);
	Hash_Node<_Key_, _Value_> *node = nodes[index];
	int i = 1;
	while (node != NULL) {
		if (node->compare(key)) {
			return node->value;
		}
		index = double_hash(key, size, i);
		node = nodes[index];
		i++;
	}
	assert(false);
	return node->value;
}

template <typename _Key_, typename _Value_>
Hash_Table<_Key_, _Value_>::Hash_Table(int _size)
{
	size = _size;
	count = 0;
	nodes = new Hash_Node<_Key_, _Value_> *[size];
	memset(nodes, 0, sizeof(Hash_Node<_Key_, _Value_> *) * size);
}

template <typename _Key_, typename _Value_>
Hash_Table<_Key_, _Value_>::~Hash_Table()
{
	delete[] nodes;
	nodes = NULL;
}


template <typename _Key_, typename _Value_>
inline void Hash_Table<_Key_, _Value_>::set(const _Key_ &key, const _Value_ &value)
{
	int index = double_hash(key, size, 0);
	Hash_Node<_Key_, _Value_> *node = nodes[index];
	int i = 1;
	while (node != NULL) {
		index = double_hash(key, size, i);
		node = nodes[index];
		i++;
	}
	nodes[index] = new Hash_Node<_Key_, _Value_>(key, value);
	count++;
}

template <typename _Key_, typename _Value_>
inline bool Hash_Table<_Key_, _Value_>::get(const _Key_ &key, _Value_ &value)
{
	int index = double_hash(key, size, 0);
	Hash_Node<_Key_, _Value_> *node = nodes[index];
	int i = 1;
	while (node != NULL) {
		if (node->compare(key)) {
			value = node->value;
			return true;
		}
		index = double_hash(key, size, i);
		node = &node[index];
		i++;
	}
	return false;
}

template <typename _Key_, typename _Value_>
inline bool Hash_Table<_Key_, _Value_>::get(const _Key_ &key, _Value_ *value)
{
	int index = double_hash(key, size, 0);
	Hash_Node<_Key_, _Value_> *node = nodes[index];
	int i = 1;
	while (node != NULL) {
		if (node->compare(key)) {
			*value = node->value;
			return true;
		}
		index = double_hash(key, size, i);
		node = &node[index];
		i++;
	}
	return false;
}
#endif