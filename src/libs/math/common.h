#ifndef COMMON_MATH_H
#define COMMON_MATH_H

#include "../../win32/win_types.h"

//float Pi = 3.1415926535f;
//


#ifdef max
#undef max
#endif

namespace math {

	template <typename T>
	inline T clamp(T value, T min, T max)
	{
		return value < min ? min : (value > max ? max : value);
	}
	template <typename T>
	inline T max(T x, T y)
	{
		return (x > y) ? x : y;
	}
	
	template <typename T>
	inline T abs(T value)
	{
		return (value > 0) ? value : -value;
	}
};

template <typename T>
struct Size {
	Size() {}
	Size(T width, T height) : width(width), height(height) {}

	T width = 0;
	T height = 0;
};

typedef Size<u32> Size_u32;
typedef Size<s32> Size_s32;

template <typename T>
struct Pair {
	Pair() {}
	Pair(const T &first, const T &second) : first(first), second(second) {}
	
	T first;
	T second;
};

typedef Pair<s32> Pair_s32;

template <typename T>
struct Point_V2 {
	T x;
	T y;
};

typedef Point_V2<s32> Point_s32;

template <typename T>
struct Rect {
	Rect() {}
	Rect(T width, T height) : width(width), height(height) {}
	Rect(T x, T y, T width, T height) : x(x), y(y), width(width), height(height) {}
	Rect(Size<T> &size) : width(size.width), height(size.height) {}
	
	T x = 0;
	T y = 0;
	T width = 0;
	T height = 0;

	void set(Size<T> &size);
	void set(T _x, T _y);
	void set_wh(T _width, T _height);

	T right();
	T bottom();
};

template <typename T>
inline void Rect<T>::set(Size<T> &size)
{
	width = size.width;
	height = size.height;
}
template <typename T>
inline void Rect<T>::set(T _x, T _y)
{
	x = _x;
	y = _y;
}
template <typename T>
inline T Rect<T>::right()
{
	return x + width;
}
template <typename T>
inline T Rect<T>::bottom()
{
	return y + height;
}
template <typename T>
inline void Rect<T>::set_wh(T _width, T _height)
{
	width = _width;
	height = _height;
}

typedef Rect<u32> Rect_u32;
typedef Rect<s32> Rect_s32;
typedef Rect<float> Rect_f32;

#endif