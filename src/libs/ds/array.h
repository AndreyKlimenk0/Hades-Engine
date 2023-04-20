#ifndef ARRAY_H
#define ARRAY_H

#include <string.h>
#include <assert.h>
#include "../../win32/win_local.h"
#include "../../win32/win_types.h"


#define DO_STRING_JOIN(arg1, arg2) arg1 ## arg2
#define STRING_JOIN(arg1, arg2) DO_STRING_JOIN(arg1, arg2)

#define For(data_struct, ptr) for (u32 _i = 0; (_i < data_struct.count ? data_struct.set_pointer_to_item(&ptr, _i), true : false); _i++)

template <typename T>
struct Array {
	Array(u32 _size = 8);
	~Array();

	T *items = NULL;
	u32 count;
	u32 size;

	Array(const Array<T> &other);
	Array<T> &operator=(const Array<T> &other);

	void resize(u32 _size);
	void remove(u32 index);
	void reserve(u32 _count);
	void shutdown();
	void set_pointer_to_item(T *ptr, u32 index);
	void set_pointer_to_item(T **ptr, u32 index);
	bool is_empty();
	u32 push(const T &item);
	u32 get_size();

	T &operator[](u32 i);
	const T &operator[](u32 i) const;

	T &pop();
	T &get(u32 index);
	const T &get(u32 index) const;

	T &first_item()
	{
		assert(count > 0);
		return items[0];
	}
	T &last_item()
	{
		assert(count > 0);
		return items[count - 1];
	}

	void clear() 
	{
		if (items) {
			delete[] items;
			items = NULL;
		}
		count = 0;
		size = 8;
		resize(size);
	}
};

template <typename T>
inline void merge(Array<T> *dst, Array<T> *src)
{
	if ((dst->count + src->count) > dst->size) {
		dst->resize(dst->count + src->count);
	}
	memcpy((void *)&dst->items[dst->count], (void *)src->items, sizeof(T) * src->count);
	dst->count += src->count;
}


template <typename T>
void Array<T>::reserve(u32 _count)
{
	resize(_count);
	count = _count;
}

template <typename T>
T &Array<T>::get(u32 index)
{
	return items[index];
}

template <typename T>
const T &Array<T>::get(u32 index) const
{
	return items[index];
}

template <typename T>
Array<T>::Array(u32 _size)
{
	items = NULL;
	count = 0;
	size = _size;
	resize(_size);
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
inline T &Array<T>::operator[](u32 i)
{
	assert(size > i);
	return items[i];
}

template <typename T>
inline const T &Array<T>::operator[](u32 i) const
{
	assert(size > i);
	return items[i];
}

template <typename T>
void Array<T>::resize(u32 new_size)
{
	if (!items) {
		items = new T[new_size];
		return;
	}

	T *temp_array = items;
	items = new T[new_size];


	for (u32 i = 0; i < count; i++) {
		items[i] = temp_array[i];
	}

	size = new_size;
	delete[] temp_array;
}

template<typename T>
inline void Array<T>::remove(u32 index)
{
	assert(count > index);


	T *new_data = new T[size];

	void *first_part_dest_str = new_data;
	void *first_part_src_str = items;

	memcpy(first_part_dest_str, first_part_src_str, sizeof(T) * index);

	void *second_part_dest_str = &new_data[index];
	void *second_part_src_str = &items[index + 1];

	memcpy(second_part_dest_str, second_part_src_str, sizeof(T) * (count - index - 1));

	delete[] items;
	
	count -= 1;
	items = new_data;
}

template <typename T>
u32 Array<T>::push(const T &item)
{
	if (count >= size) {
		resize(size * 2);
	}
	items[count] = item;
	return count++;
}

template<typename T>
inline u32 Array<T>::get_size()
{
	return sizeof(T) * count;
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

template<typename T>
inline Array<T>::Array(const Array<T>& other)
{
	*this = other;
}

template<typename T>
inline Array<T>& Array<T>::operator=(const Array<T>& other)
{
	count = other.count;
	size = other.size;
	if (items) delete[] items, items = NULL;
	items = new T[other.size];
	for (u32 i = 0; i < count; i++) {
		items[i] = other.items[i];
	}
	return *this;
}

template <typename T>
inline bool Array<T>::is_empty()
{
	assert(count >= 0);

	return count == 0;
}

template <typename T>
inline void Array<T>::set_pointer_to_item(T *ptr, u32 index)
{
	assert(count > index);

	*ptr = items[index];
}

template <typename T>
inline void Array<T>::set_pointer_to_item(T **ptr, u32 index)
{
	assert(count > index);
	*ptr = &items[index];
}

#endif