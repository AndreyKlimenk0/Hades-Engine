#ifndef ARRAY_H
#define ARRAY_H

#include <string.h>
#include <assert.h>

template <typename T>
struct Array {
	T *array;
	int count;
	int size;

	Array(int _size = 8);
	~Array() { if (array) delete[] array; }

	T &operator[](int i);
	const T &operator[](int i) const;

	T &pop();
	T &at(int index);
	void resize();
	void shutdown();
	void push(const T &item);
	
};

template <typename T>
T &Array<T>::at(int index)
{
	return array[index];
}

template <typename T>
Array<T>::Array(int _size)
{
	array = NULL;
	count = 0;
	size = _size;
	resize();
}

template <typename T>
T &Array<T>::operator[](int i)
{
	assert(count > i);
	return array[i];
}

template <typename T>
const T &Array<T>::operator[](int i) const
{
	assert(count > i);
	return array[i];
}


template <typename T>
void Array<T>::resize()
{
	if (!array) {
		array = new T[size];
		return;
	}
	
	T *temp_array = array;
	array = new T[size * 2];
	memcpy(array, temp_array, sizeof(T) * size);
	size = size * 2;
	delete[] temp_array;
}

template <typename T>
void Array<T>::push(const T &item)
{
	if (count + 1 >= size) {
		resize();
	}
	array[count++] = item;
}

template <typename T>
T &Array<T>::pop()
{
	assert(count > 0);
	return array[--count];
}

template <typename T>
void Array<T>::shutdown()
{
	if (array) delete[] array;
	size = 0;
	count = 0;
}
#endif