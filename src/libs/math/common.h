#ifndef COMMON_MATH_H
#define COMMON_MATH_H

#include <assert.h>
#include <math.h>
#include "../../win32/win_types.h"

const float PI = 3.1415926535f;
const float RADIANS_360 = 6.28319f;

inline s32 radians_to_degrees(float radians)
{
	return (s32)(radians * (180.0f / PI));
}

inline float degress_to_radians(s32 degrees)
{
	return ((float)degrees * PI) / 180.0f;
}

#ifdef max
	#undef max
#endif

#ifdef min
	#undef min
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
	inline T min(T x, T y)
	{
		return (x > y) ? y : x;
	}
	
	template <typename T>
	inline T abs(T value)
	{
		return (value > 0) ? value : -value;
	}

	inline float arccos(float value)
	{
		return (float)::acos((double)value);
	}

	inline float cos(float value)
	{
		return (float)::cos((double)value);
	}

	inline float sin(float value)
	{
		return (float)::sin((double)value);
	}

	template< typename T>
	inline T pow2(T value)
	{
		return (T)::pow((double)value, 2);
	}

	inline float sqrt(float value)
	{
		return (float)::sqrt((double)value);
	}
};

template <typename T>
struct Size {
	Size() {}
	Size(T width, T height) : width(width), height(height) {}

	T width = 0;
	T height = 0;

	T &operator[](u32 index);
};

template <typename T>
inline T &Size<T>::operator[](u32 index)
{
	assert(index < 2);
	return (&width)[index];
}


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
	Point_V2() {};
	Point_V2(const T &x, const T &y) : x(x), y(y) {};
	T x;
	T y;

	T &operator[](int index);
};

template <typename T>
T &Point_V2<T>::operator[](int index)
{
	assert(index < 2);
	return ((T *)&x)[index];
}

template <typename T>
Point_V2<T> operator-(const Point_V2<T> &point1, const Point_V2<T> &point2)
{
	return Point_V2<T>(point1.x - point2.x, point1.y - point2.y);
}

template <typename T>
float slope(const Point_V2<T> &point1, const Point_V2<T> &point2)
{
	T y_delta = point2.y - point1.y;
	T x_delta = point2.x - point1.x;
	if (x_delta == 0.0f) {
		return 0.0f;
	}
	return (float)y_delta / (float)x_delta;
}

template <typename T>
float distance(Point_V2<T> *point1, Point_V2<T> *point2)
{
	return math::sqrt(math::pow2(point1->x - point2->x) + math::pow2(point1->y - point2->y));
}

typedef Point_V2<s32> Point_s32;
typedef Point_V2<u32> Point_u32;
typedef Point_V2<float> Point_f32;

template <typename T>
struct Triangle {
	Triangle() {};
	Triangle(Point_V2<T> a, Point_V2<T> b, Point_V2<T> c) : a(a), b(b), c(c) {};
	
	Point_V2<T> a;
	Point_V2<T> b;
	Point_V2<T> c;

	T get_area();
};

template <typename T>
inline T Triangle<T>::get_area()
{
	return math::abs((b - a).x * (c - a).y - (c - a).x * (b - a).y);
}

enum Rect_Side {
	RECT_SIDE_LEFT,
	RECT_SIDE_RIGHT,
	RECT_SIDE_TOP,
	RECT_SIDE_BOTTOM,
	RECT_SIDE_LEFT_TOP,
	RECT_SIDE_RIGHT_TOP,
	RECT_SIDE_LEFT_BOTTOM,
	RECT_SIDE_RIGHT_BOTTOM,
};

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

	T &operator[](u32 index);

	void set(Size<T> &size);
	void set(T _x, T _y);
	void offset_y(T _y);
	void offset_x(T _x);
	void set_size(T _width, T _height);
	Size<T> get_size();

	T right();
	T bottom();
};

template <typename T>
inline T &Rect<T>::operator[](u32 index)
{
	assert(index < 4);
	return (&x)[index];
}

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
template<typename T>
inline void Rect<T>::offset_y(T _y)
{
	y += _y;
	height -= _y;
}

template<typename T>
inline void Rect<T>::offset_x(T _x)
{
	x += _x;
	width -= _x;
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
inline void Rect<T>::set_size(T _width, T _height)
{
	width = _width;
	height = _height;
}

template <typename T>
inline Size<T> Rect<T>::get_size()
{
	return Size<T>(width, height);
}

typedef Rect<u32> Rect_u32;
typedef Rect<s32> Rect_s32;
typedef Rect<float> Rect_f32;

#endif
