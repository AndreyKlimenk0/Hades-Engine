#ifndef ARRAY_H
#define ARRAY_H

#include <string.h>
#include <assert.h>

#define DO_STRING_JOIN(arg1, arg2) arg1 ## arg2
#define STRING_JOIN(arg1, arg2) DO_STRING_JOIN(arg1, arg2)

#define For(data_struct, ptr) for (int _i = 0; (_i < data_struct.count ? data_struct.set_pointer_to_item(&ptr, _i), true : false); _i++)

template <typename T>
struct Array {
	Array(int _size = 8);
	~Array();

	T *items = NULL;
	int count;
	int size;

	bool is_empty();
	void resize();
	void shutdown();
	void push(const T &item);
	void set_pointer_to_item(T *ptr, int index);
	void set_pointer_to_item(T **ptr, int index);

	T &operator[](int i);
	const T &operator[](int i) const;

	T &pop();
	T &at(int index);
	const T &at(int index) const;
	T &last_item() { return items[count - 1]; }

	void clear() 
	{
		if (items) {
			delete[] items;
			items = NULL;
		}
		count = 0;
		size = 8;
		resize();
	}
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
#include <stdlib.h>

template <typename T>
Array<T>::~Array()
{
	if (items) {
		delete[] items;
		items = NULL;
	}
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
	//memcpy(items, temp_array, sizeof(T) * size);
	for (int i = 0; i < count; i++) {
		items[i] = temp_array[i];
	}
	size = size * 2;
	delete[] temp_array;
}

template <typename T>
void Array<T>::push(const T &item)
{
	if (count >= size) {
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

template <typename T>
bool Array<T>::is_empty()
{
	assert(count >= 0);

	return count == 0;
}

template <typename T>
void Array<T>::set_pointer_to_item(T *ptr, int index)
{
	assert(count > index);

	*ptr = items[index];
}

template <typename T>
void Array<T>::set_pointer_to_item(T **ptr, int index)
{
	assert(count > index);
	*ptr = &items[index];
}

#endif