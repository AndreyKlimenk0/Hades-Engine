#ifndef SPARSE_SET_H
#define SPARSE_SET_H

#include <assert.h>
#include <stdint.h> //MAX_UINT

#include "array.h"
#include "../number_types.h"

template <typename T>
struct Sparse_Set {
	struct Element {
		u32 sparse_index;
		T value;
	};

	Array<u32> sparse_array;
	Array<Element> dense_array;

	void remove(u32 index);
	bool empty();
	u32 sparse_count();
	u32 dense_count();
	u32 push(const T &item);
	T &get_sparse(u32 index);
	T &get_dense(u32 index);
};

template <typename T>
inline void Sparse_Set<T>::remove(u32 index)
{
	if (index >= sparse_array.count) {
		return;
	}
	u32 dense_index = sparse_array[index];

	if (dense_index >= dense_array.count) {
		return;
	}
	u32 sparse_index = dense_array[dense_index].sparse_index;

	assert(index == sparse_index);

	dense_array[dense_index].value = dense_array[dense_array.count - 1].value;

	u32 swaped_element_sparse_index = dense_array[dense_array.count - 1].sparse_index;
	sparse_array[swaped_element_sparse_index] = dense_index;

	dense_array[dense_index].sparse_index = swaped_element_sparse_index;

	sparse_array[index] = UINT32_MAX;

	dense_array.count--;
}

template <typename T>
inline bool Sparse_Set<T>::empty()
{
	return dense_array.is_empty();
}

template <typename T>
inline u32 Sparse_Set<T>::sparse_count()
{
	return sparse_array.count;
}

template <typename T>
inline u32 Sparse_Set<T>::dense_count()
{
	return dense_array.count;
}

template <typename T>
inline u32 Sparse_Set<T>::push(const T &value)
{
	u32 dense_index = dense_array.push({ sparse_array.count, value });
	u32 sparse_index = sparse_array.push(dense_index);
	return sparse_index;
}

template <typename T>
inline T &Sparse_Set<T>::get_sparse(u32 index)
{
	assert(index < sparse_array.count);
	u32 dense_index = sparse_array[index];
	assert(dense_index < dense_array.count);
	u32 sparse_index = dense_array[dense_index].sparse_index;
	assert(index == sparse_index);
	return  dense_array[dense_index].value;
}

template <typename T>
inline T &Sparse_Set<T>::get_dense(u32 index)
{
	assert(index < dense_array.count);
	return  dense_array[index].value;
}

#endif
