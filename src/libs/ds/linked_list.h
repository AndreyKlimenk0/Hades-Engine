#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdlib.h>
#include "../../sys/sys_local.h"
#include "../../win32/win_types.h"
#include <type_traits>

using namespace std;


template <typename T>
struct Node {
	Node() {};
	Node(const T &item) : item(item) {}

	T item;
	Node<T> *next = NULL;
};

template <typename T>
struct Linked_List {
	Linked_List(bool handle_node_item_deleting = true) : handle_node_item_deleting(handle_node_item_deleting) {};
	~Linked_List();

	bool handle_node_item_deleting;
	bool is_pointer_type = is_pointer<T>::value;

	u32 count = 0;

	Node<T> *first_node = NULL;
	Node<T> *last_node = NULL;

	void print();
	void append(const T &item);
	void append(Node<T> *node);

	//void remove(const T &item);
	void remove(const T &item, T &item_buffer);
	void remove_node(const T &item, Node<T> **node_ptr);
	void delete_node(Node<T> *node);
	
	void set_pointer_to_item(T *ptr, u32 index);
	void set_pointer_to_item(T **ptr, u32 index);
	
	T &find(u32 index);
	T &find(const T &item);
	
	Node<T> *find_node(u32 index);
	Node<T> *find_node(const T &item);
};


template <typename T>
struct Run_Time_Pointer_Deleter {
	void operator ()(T item) {}
};

template <typename T>
struct Run_Time_Pointer_Deleter<T *> {
	void operator ()(T *item) { DELETE_PTR(item); }
};

template <typename T>
Linked_List<T>::~Linked_List()
{
	Run_Time_Pointer_Deleter<T> deleter;

	if (handle_node_item_deleting && is_pointer_type) {
		Node<T> *node = first_node;
		while (node != NULL) {
			Node<T> *next = node->next;
			deleter(node->item);
			delete node;
			node = next;
		}
	} else {
		Node<T> *node = first_node;
		while (node != NULL) {
			Node<T> *next = node->next;
			delete node;
			node = next;
		}
	}
}

template <typename T>
void Linked_List<T>::append(const T &item)
{
	if (!first_node) {
		first_node = new Node<T>(item);
		last_node = first_node;
	} else {
		last_node->next = new Node<T>(item);
		last_node = last_node->next;
	}
	count++;
}

template<typename T>
inline void Linked_List<T>::append(Node<T>* node)
{
	assert(node);

	if (!first_node) {
		first_node = node;
		last_node = first_node;
	} else {
		last_node->next = node;
		last_node = last_node->next;
	}
	count++;
}

template <typename T>
void Linked_List<T>::delete_node(Node<T> *node)
{
	static Run_Time_Pointer_Deleter<T> deleter;
	
	if (is_pointer_type && handle_node_item_deleting) {
		deleter(node->item);
		delete node;
	} else {
		delete node;
	}
}

template <typename T>
void Linked_List<T>::remove(const T &item, T &item_buffer)
{
	Node<T> *removing_node = NULL;
	if (first_node->item == item) {
		item_buffer = first_node->item;

		removing_node = first_node;
		first_node = first_node->next;

		delete_node(removing_node);
	} else {
		Node<T> *node = first_node;
		while ((node->next != NULL) && (node->next->item != item)) {
			node = node->next;
		}
		assert(node->next);

		removing_node = node->next;
		node->next = removing_node->next;

		item_buffer = removing_node->item;

		delete_node(removing_node);
	}
	count--;
}

template<typename T>
inline void Linked_List<T>::remove_node(const T & item, Node<T> **node_ptr)
{
	Node<T> *removing_node = NULL;
	if (first_node->item == item) {
		*node_ptr = first_node;
		first_node = first_node->next;
		(*node_ptr)->next = NULL;

		//delete_node(removing_node);
	} else {
		Node<T> *node = first_node;
		while ((node->next != NULL) && (node->next->item != item)) {
			node = node->next;
		}
		assert(node->next);

		removing_node = node->next;
		node->next = removing_node->next;

		removing_node->next = NULL;
		*node_ptr = removing_node;

		//delete_node(removing_node);
	}
	count--;
}

template <typename T>
void Linked_List<T>::print()
{
	Node<T> *node = first_node;
	while (node != NULL) {
		::print(node->item);
		node = node->next;
	}
}

template <typename T>
void Linked_List<T>::set_pointer_to_item(T *ptr, u32 index)
{
	T &item = find(index);
	*ptr = item;
}

template <typename T>
void Linked_List<T>::set_pointer_to_item(T **ptr, u32 index)
{
	T &item = find(index);
	*ptr = &item;
}

template <typename T>
T &Linked_List<T>::find(u32 index)
{
	Node<T> *node = find_node(index);
	assert(node);

	return node->item;
}

template <typename T>
T &Linked_List<T>::find(const T &item)
{
	Node<T> *node = find_node(item);
	assert(node);

	return node->item;
}

template<typename T>
Node<T>* Linked_List<T>::find_node(u32 index)
{
	assert(count > index);
	assert(first_node);

	u32 node_index = 0;

	Node<T> *node = first_node;
	while ((node != NULL) && node_index != index) {
		node = node->next;
		node_index++;
	}
	return node;
}

template<typename T>
Node<T>* Linked_List<T>::find_node(const T &item)
{
	assert(first_node);

	Node<T> *node = first_node;
	while ((node != NULL) && node->item != item) {
		node = node->next;
	}
	return node;
}
#endif