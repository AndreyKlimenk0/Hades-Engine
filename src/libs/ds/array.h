#ifndef ARRAY_H
#define ARRAY_H

#include <string.h>
#include <assert.h>

template <typename T>
struct Array {
	T *items;
	int count;
	int size;

	Array(int _size = 8);
	~Array() { if (items) delete[] items; }

	T &operator[](int i);
	const T &operator[](int i) const;

	T &pop();
	T &at(int index);
	const T &at(int index) const;
	void resize();

	void abort() 
	{
		if (items) {
			delete[] items;
			items = NULL;
		}
		count = 0;
		size = 8;
		resize();
	}
	void shutdown();
	void push(const T &item);
	
};

template <typename T>
T &Array<T>::at(int index)
{
	return items[index];
}

template <typename T>
const T &Array<T>::at(int index) const
{
	return items[index];
}

template <typename T>
Array<T>::Array(int _size)
{
	items = NULL;
	count = 0;
	size = _size;
	resize();
}

template <typename T>
T &Array<T>::operator[](int i)
{
	assert(count > i);
	return items[i];
}

template <typename T>
const T &Array<T>::operator[](int i) const
{
	assert(count > i);
	return items[i];
}


template <typename T>
void Array<T>::resize()
{
	if (!items) {
		items = new T[size];
		return;
	}
	
	T *temp_array = items;
	items = new T[size * 2];
	memcpy(items, temp_array, sizeof(T) * size);
	size = size * 2;
	delete[] temp_array;
}

template <typename T>
void Array<T>::push(const T &item)
{
	if (count + 1 >= size) {
		resize();
	}
	items[count++] = item;
}

template <typename T>
T &Array<T>::pop()
{
	assert(count > 0);
	return items[--count];
}

template <typename T>
void Array<T>::shutdown()
{
	if (items) {
		delete[] items;
		items = NULL;
	}

	size = 0;
	count = 0;
}
#endif