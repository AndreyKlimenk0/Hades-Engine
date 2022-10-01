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
	Node<T> *previous = NULL;
};

template <typename T>
struct Linked_List {
	Linked_List(bool handle_node_item_deleting = false) : handle_node_item_deleting(handle_node_item_deleting) {};
	~Linked_List();

	bool handle_node_item_deleting;
	bool is_pointer_type = is_pointer<T>::value;

	u32 count = 0;

	Node<T> *first_node = NULL;
	Node<T> *last_node = NULL;

	void print();
	void append_back(const T &item);
	void append_front(const T &item);

	void append_front(Node<T> *node);

	void remove(const T &item);
	void remove(Node<T> *node);
	
	void delete_node(Node<T> *node);

	void clear();
	
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
void Linked_List<T>::delete_node(Node<T> *node)
{
	static Run_Time_Pointer_Deleter<T> deleter;
	
	if (is_pointer_type && handle_node_item_deleting) {
		deleter(node->item);
		DELETE_PTR(node);
	} else {
		DELETE_PTR(node);
	}
}

template<typename T>
inline void Linked_List<T>::clear()
{
	Node<T> *node = first_node;
	while (node != NULL) {
		Node<T> *next = node->next;
		delete node;
		node = next;
	}
	count = 0;
	first_node = NULL;
	last_node = NULL;
}

template <typename T>
void Linked_List<T>::remove(const T &item)
{
	Node<T> *current_node = first_node;
	while (current_node) {
		if (current_node->item == item) {
			current_node->next ? current_node->next->previous = current_node->previous : last_node = current_node->previous;
			current_node->previous ? current_node->previous->next = current_node->next : first_node = current_node->next;
			delete_node(current_node);
			count -= 1;
			break;
		}
		current_node = current_node->next;
	}
}

template<typename T>
inline void Linked_List<T>::remove(Node<T>* node)
{
	if (node->previous) {
		node->previous->next = node->next;
	} else {
		first_node = node->next;
	}

	if (node->next) {
		node->next->previous = node->previous;
	} else {
		node->next = node->previous;
	}
	node->next = NULL;
	node->previous = NULL;
	count -= 1;
}

template <typename T>
void Linked_List<T>::print()
{
	Node<T> *node = first_node;
	while (node != NULL) {
		::print("----------------------------------------");
		if (node->previous) {
			::print("Previous node item: ", node->previous->item);
		} else {
			::print("Previous node is NULL");
		}
		if (node->next) {
			::print("Next node item: ", node->next->item);
		} else {
			::print("Next node is NULL");
		}
		::print("Node item: ", node->item);

		node = node->next;
	}
}

template<typename T>
inline void Linked_List<T>::append_back(const T & item)
{
	if (!first_node) {
		first_node = new Node<T>(item);
		last_node = first_node;
	} else {
		last_node->next = new Node<T>(item);
		last_node->next->previous = last_node;
		last_node = last_node->next;
	}
	count++;
}

template<typename T>
inline void Linked_List<T>::append_front(const T &item)
{
	Node<T> *new_node = new Node<T>(item);
	if (first_node) {
		first_node->previous = new_node;
	}
	new_node->next = first_node;
	first_node = new_node;

	
	if (!last_node) {
		last_node = first_node;
	}
	count++;
}

template<typename T>
inline void Linked_List<T>::append_front(Node<T>* node)
{
	if (first_node) {
		first_node->previous = node;
	}
	node->next = first_node;
	first_node = node;

	
	if (!last_node) {
		last_node = node;
	}
	count++;
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