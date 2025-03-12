#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <assert.h>
#include "../memory/pool_allocator.h"

template <typename T>
struct Queue_Node {
	Queue_Node() = default;
	~Queue_Node() = default;

	T item;
	Queue_Node *next = NULL;
	Queue_Node(const T &item, Queue_Node *next) : item(item), next(next) {}
};

template <typename T>
struct Queue {
	Queue();
	~Queue();

	Queue_Node<T> *first = NULL;
	Queue_Node<T> *last = NULL;

	Pool_Allocator<Queue_Node<T>> pool_allocator;

	void push(const T& item);
	void clear();
	bool is_empty();
	void pop();
	T &front();
};

template<typename T>
Queue<T>::Queue()
{
	pool_allocator.init(16);
}

template<typename T>
Queue<T>::~Queue()
{
	clear();
	pool_allocator.free_memory();
}

template <typename T>
bool Queue<T>::is_empty()
{
	return first == NULL;
}

template <typename T>
void Queue<T>::push(const T& item)
{
	Queue_Node<T> *node = pool_allocator.allocate();
	node->item = item;
	if (last) {
		last->next = node;
	}
	else {
		first = node;
	}
	last = node;
}

template<typename T>
void Queue<T>::clear()
{
	Queue_Node<T>* node = NULL;
	while (first) {
		node = first;
		first = first->next;
		node->next = NULL;
		pool_allocator.free(node);
	}
	first = NULL;
	last = NULL;
}

template <typename T>
void Queue<T>::pop()
{
	Queue_Node<T>* node = first;
	if (node) {
		first = node->next;
		if (node == last) {
			last = NULL;
		}
		node->next = NULL;
		pool_allocator.free(node);
	}
}

template<typename T>
T &Queue<T>::front()
{
	assert(!is_empty());
	return first->item;
}

#endif
