#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../../sys/sys_local.h"
#include "../../win32/win_types.h"


u32 fast_hash(const char *data);

inline u32 hash(const char* string, int factor, int table_count)
{
	unsigned long hash = 5381;
    int c;

	while (c = *string++) {
        hash = factor * ((hash << 5) + hash) + c; 
	}
    return (u32)(hash % table_count);
}

inline u32 hash(int number, int table_count, int attempt)
{
	char *str = to_string(number);
	u32 h = hash(str, table_count, attempt);
	free_string(str);
	return h;
}

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

	typedef Hash_Node<_Key_, _Value_> Table_Entry;

	Array<Table_Entry *> nodes;
	
	u32 size = 0;
	u32 count = 0;
	u32 max_loop = 0;
	u32 table_size = 0;
	u32 hash_factor1 = 7;
	u32 hash_facotr2 = 11;

	void clear();
	void rehash();
	void insert_entry(Table_Entry *entry);
	void free_table(Table_Entry **table, u32 _table_size);

	void set(const _Key_ &key, const _Value_ &value);

	bool key_in_table(const _Key_ &key);
	bool get(const _Key_ &key, _Value_ &value);
	bool get(const _Key_ &key, _Value_ *value);

	u32 hash1(const _Key_ &key);
	u32 hash2(const _Key_ &key);

	//_Value_ &operator[](u32 index);
	_Value_ &operator[](const _Key_ &key);

	Hash_Node<_Key_, _Value_> *get_node(u32 index);
	Hash_Node<_Key_, _Value_> *get_table_entry(const _Key_ &key);
};

template<typename _Key_, typename _Value_>
Hash_Table<_Key_, _Value_>::Hash_Table(int _size)
{
	rand();
	hash_factor1 = rand();
	hash_facotr2 = rand() % 256;
	size = _size;
	table_size = _size * 2;
	max_loop = table_size;

	nodes.resize(table_size);

	memset(nodes.items, 0, sizeof(Table_Entry *) * table_size);
}

template<typename _Key_, typename _Value_>
Hash_Table<_Key_, _Value_>::~Hash_Table()
{
	for (u32 i = 0; i < table_size; i++) {
		delete nodes[i];
	}
}

template<typename _Key_, typename _Value_>
u32 Hash_Table<_Key_, _Value_>::hash1(const _Key_ &key)
{
	return (u32)hash((_Key_)key, hash_factor1, size);
}

template<typename _Key_, typename _Value_>
u32 Hash_Table<_Key_, _Value_>::hash2(const _Key_ &key)
{
	return (u32)hash((_Key_)key, hash_facotr2, size) + size;
}

//template<typename _Key_, typename _Value_>
//_Value_ &Hash_Table<_Key_, _Value_>::operator[](u32 index)
//{
//	Table_Entry *entry = get_node(index);
//	return entry->value;
//}

template<typename _Key_, typename _Value_>
_Value_ &Hash_Table<_Key_, _Value_>::operator[](const _Key_ &key)
{
	Table_Entry *entry = get_table_entry(key);
	if (!entry) {
		_Value_ value;
		entry = new Table_Entry(key, value);
		insert_entry(entry);
	}
	return entry->value;
}

template<typename _Key_, typename _Value_>
Hash_Node<_Key_, _Value_>* Hash_Table<_Key_, _Value_>::get_node(u32 index)
{
	assert(count > index);

	u32 node_count = 0;
	for (u32 i = 0; i < table_size; i++) {
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
Hash_Node<_Key_, _Value_> *Hash_Table<_Key_, _Value_>::get_table_entry(const _Key_ &key)
{
	u32 _hash1 = hash1(key);
	u32 _hash2 = hash2(key);

	if ((nodes[_hash1] != NULL) && (nodes[_hash1]->key == key)) {
		return nodes[_hash1];
	}

	if ((nodes[_hash2] != NULL) && (nodes[_hash2]->key == key)) {
		return nodes[_hash2];
	}
	return NULL;
}


template<typename _Key_, typename _Value_>
inline void Hash_Table<_Key_, _Value_>::clear()
{
	Table_Entry *table_entry = NULL;
	For(nodes, table_entry) {
		if (table_entry) {
			DELETE_PTR(table_entry);
		}
	}
	nodes.clear();
}

template<typename _Key_, typename _Value_>
void Hash_Table<_Key_, _Value_>::rehash()
{	
	u32 old_table_size = table_size;
	u32 _size = size * 2;
	
	hash_factor1 = rand();
	hash_facotr2 = rand() % 256;
	size = _size;
	count = 0;
	table_size = table_size *2;
	max_loop = table_size;

	Table_Entry **old_nodes = nodes.items;

	nodes.items = new Table_Entry *[table_size];
	nodes.size = table_size;
	memset(nodes.items, 0, sizeof(Table_Entry *) * table_size);

	for (u32 i = 0; i < old_table_size; i++) {
		Table_Entry *entry = old_nodes[i];
		if (entry != NULL) {
			insert_entry(entry);
		}
	}
	delete[] old_nodes;
}

template<typename _Key_, typename _Value_>
void Hash_Table<_Key_, _Value_>::free_table(Table_Entry **table, u32 _table_size)
{
	delete[] table;
	table = NULL;
}

template<typename _Key_, typename _Value_>
void Hash_Table<_Key_, _Value_>::insert_entry(Table_Entry *entry)
{
	_Key_ _key = entry->key;
	
	Table_Entry *inserting_entry = entry;
	Table_Entry **existing_entry = NULL;

	for (u32 i = 0; i < max_loop; i++) {
		u32 hashies[] = { hash1(_key), hash2(_key) };

		u32 index = hashies[i % 2];
		
		if (nodes[index] == NULL) {
			nodes[index] = inserting_entry;
			break;
		}
		Table_Entry *temp = nodes[index];
		nodes[index] = inserting_entry;
		inserting_entry = temp;
		_key = temp->key;

		if ((i + 1) >= max_loop) {
			rehash();
			i = 0;
		}
	}
	count++;
}

template<typename _Key_, typename _Value_>
void Hash_Table<_Key_, _Value_>::set(const _Key_ &key, const _Value_ &value)
{
	Table_Entry *entry = get_table_entry(key);
	if (entry) {
		entry->value = value;
		return;
	}

	Table_Entry *new_entry = new Table_Entry(key, value);
	insert_entry(new_entry);
}

template<typename _Key_, typename _Value_>
bool Hash_Table<_Key_, _Value_>::key_in_table(const _Key_ &key)
{
	Table_Entry *entry = get_table_entry(key);
	if (entry) {
		return true;
	}
	return false;
}

template<typename _Key_, typename _Value_>
bool Hash_Table<_Key_, _Value_>::get(const _Key_ &key, _Value_ &value)
{
	Table_Entry *entry = get_table_entry(key);
	if (entry) {
		value = entry->value;
		return true;
	}
	return false;
}

template<typename _Key_, typename _Value_>
bool Hash_Table<_Key_, _Value_>::get(const _Key_ & key, _Value_ *value)
{
	Table_Entry *entry = get_table_entry(key);
	if (entry) {
		*value = entry->value;
		return true;
	}
	return false;
}
#endif