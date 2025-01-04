#ifndef MEMORY_H
#define MEMORY_H

#include <assert.h>
#include "../libs/number_types.h"

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

#endif
