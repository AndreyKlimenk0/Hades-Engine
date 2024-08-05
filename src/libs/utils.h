#ifndef UTILS_FUNCTION_H
#define UTILS_FUNCTION_H


template <typename T>
inline bool in_range(const T &start, const T &end, const T &value)
{
	return (start <= value) && (value <= end);
}

#endif 

