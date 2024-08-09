#ifndef UTILS_FUNCTION_H
#define UTILS_FUNCTION_H

#include "number_types.h"
#include "math/structures.h"

template <typename T>
inline bool in_range(const T &start, const T &end, const T &value)
{
	return (start <= value) && (value <= end);
}

inline Point_u32 convert_1d_to_3d_index(u32 one_dimensional_index, u32 height, u32 depth)
{
	u32 i = one_dimensional_index / (height * depth);
	u32 j = (one_dimensional_index % (height * depth)) / depth;
	u32 k = one_dimensional_index % depth;
	return Point_u32(i, j, k);
}

inline u32 convert_3d_to_1d_index(u32 x, u32 y, u32 z, u32 width, u32 depth)
{
	return x *(width * depth) + y * depth + z;
}
#endif 

