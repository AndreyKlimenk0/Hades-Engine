#ifndef ARRAY_H
#define ARRAY_H

#include <string.h>


template <typename T>
struct Array {
	T *array = NULL;
	int count;
	int size;

	Array(int _size = 8);
	~Array() { delete[] array; }
	void push(const T &item);
	T &pop();
	void resize();
};

template <typename T>
Array<T>::Array(int _size)
{
	count = 0;
	size = _size;
	resize();
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
	return array[--count];
}


#endif