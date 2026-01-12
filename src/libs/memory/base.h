#ifndef MEMORY_BASE_H
#define MEMORY_BASE_H

#include <assert.h>
#include "../number_types.h"

template <typename T>
inline T align_address(T address, T alignment)
{
	if (alignment > 0) {
		const T mask = alignment - 1;
		assert((alignment & mask) == 0); // pwr of 2
		return (address + mask) & ~mask;
	}
	return address;
}

template <typename T>
inline T megabytes_to_bytes(T megabytes)
{
	return 1048576 * megabytes;
}

template <typename T>
inline u64 pointer_address(T *pointer)
{
	assert(pointer);

	T **double_ptr = &pointer;
	return (u64)*double_ptr;
}
#endif