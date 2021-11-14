#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../../sys/sys_local.h"

//#define HT_PRIME_1 151
#define HT_PRIME_1 1007887
//#define HT_PRIME_2 163
//#define HT_PRIME_2 1007957
#define HT_PRIME_2 2008393


int hash(char c, const int factor, const int table_count);
int double_hash(char c, const int table_count, const int attempt);

int hash(const char* string, const int factor, const int table_count);
int double_hash(const char* string, const int table_count, const int attempt);

int double_hash(int number, const int table_count, const int attempt);

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
	Hash_Table(int _size = 8);
	~Hash_Table();

	Hash_Node<_Key_, _Value_> **nodes;
	u32 size;
	u32 count;

	void resize();

	void set(const _Key_ &key, const _Value_ &value);
	bool get(const _Key_ &key, _Value_ &value);
	bool get(const _Key_ &key, _Value_ *value);

	_Value_ &operator[](const _Key_ &key);
	_Value_ &operator[](u32 index);

	Hash_Node<_Key_, _Value_> *get_node(u32 index);
};

template<typename _Key_, typename _Value_>
_Value_ &Hash_Table<_Key_, _Value_>::operator[](u32 index)
{
	assert(count > index);

	u32 node_count = 0;
	for (int i = 0; i < size; i++) {
		Hash_Node<_Key_, _Value_> *node = nodes[i];
		if ((node != NULL) && (node_count == index)) {
			return node->value;
		} else {
			if (node != NULL) {
				node_count += 1;
			}
		}
	}
	assert(false);
}

template<typename _Key_, typename _Value_>
inline Hash_Node<_Key_, _Value_>* Hash_Table<_Key_, _Value_>::get_node(u32 index)
{
	assert(count > index);

	u32 node_count = 0;
	for (int i = 0; i < size; i++) {
		Hash_Node<_Key_, _Value_> *node = nodes[i];
		if ((node != NULL) && (node_count == index)) {
			return node;
		} else {
			if (node != NULL) {
				node_count += 1;
			}
		}
	}
	assert(false);
	return NULL;
}

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
void Hash_Table<_Key_, _Value_>::resize()
{
	Hash_Node<_Key_, _Value_> **temp_nodes = nodes;
	
	size *= 2;
	nodes = new Hash_Node<_Key_, _Value_> *[size];
	
	memset(nodes, 0, sizeof(Hash_Node<_Key_, _Value_> *) * size);
	memcpy(nodes, temp_nodes, sizeof(Hash_Node<_Key_, _Value_> *) * count);

	delete[] temp_nodes;
}

template <typename _Key_, typename _Value_>
inline void Hash_Table<_Key_, _Value_>::set(const _Key_ &key, const _Value_ &value)
{
	if (size == count) {
		resize();
	}

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
		node = nodes[index];
		i++;
	}
	return false;
}

template <typename _Key_, typename _Value_>
inline bool Hash_Table<_Key_, _Value_>::get(const _Key_ &key, _Value_ *value)
{
	int index = double_hash(key, size, 0);
	Hash_Node<_Key_, _Value_> *node = nodes[index];
	u32 i = 1;
	u32 c = 0;
	while ((node != NULL) && (c < count)) {
		if (node->compare(key)) {
			*value = node->value;
			return true;
		}
		index = double_hash(key, size, i);
		node = nodes[index];
		i++;
		c++;
	}
	return false;
}
#endif