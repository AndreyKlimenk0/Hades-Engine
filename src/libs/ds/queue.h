#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <assert.h>


template <typename T>
struct Queue_Node {
	const T &item;
	Queue_Node *next;
	Queue_Node(const T &item, Queue_Node *next) : item(item), next(next) {}
};

template <typename T>
struct Queue {
	Queue_Node<T> *first = NULL;
	Queue_Node<T> *last = NULL;

	Queue() {}
	~Queue();
	void push(const T &item);
	bool is_empty();
	const T &pop();
};

template<typename T>
Queue<T>::~Queue()
{
	Queue_Node<T> *node = NULL;
	while (first) {
		node = first;
		first = first->next;
		node->next = NULL;
		delete node;
	}
}

template <typename T>
bool Queue<T>::is_empty()
{
	return first == NULL;
}

template <typename T>
void Queue<T>::push(const T &item)
{
	Queue_Node<T> * node = new Queue_Node<T>(item, NULL);
	if (last) {
		last->next = node;
	} else {
		first = node;
	}
	last = node;
}

template <typename T>
const T &Queue<T>::pop()
{
	assert(first != NULL);
	const T *item = NULL;
	Queue_Node<T> *node = first;
	if (node) {
		first = node->next;
		item = &node->item;
		if (node == last) {
			last = NULL;
		}
		node->next = NULL;
		delete node;
	}
	return *item;
}
#endif