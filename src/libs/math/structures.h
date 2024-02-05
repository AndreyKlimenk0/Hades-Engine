#ifndef MATH_STRUCTURES_H
#define MATH_STRUCTURES_H

#include <assert.h>
#include <string.h>

#include "vector.h"
#include "functions.h"
#include "../../win32/win_types.h"

struct Ray {
	Ray() {}
	Ray(const Vector3 &_origin, const Vector3 &_direction);

	float len;
	Vector3 origin;
	Vector3 direction;
};

template <typename T>
struct Size {
	Size();
	Size(T width, T height);

	T width;
	T height;

	T &operator[](u32 index);
};

template <typename T>
inline Size<T>::Size()
{
	memset((void *)this, 0, sizeof(Size<T>));
}

template <typename T>
inline Size<T>::Size(T width, T height) : width(width), height(height)
{
}

template <typename T>
inline T &Size<T>::operator[](u32 index)
{
	assert(index < 2);
	return (&width)[index];
}

typedef Size<u32> Size_u32;
typedef Size<s32> Size_s32;

template <typename T>
struct Pointv2 {
	Pointv2();
	Pointv2(const T &_x, const T &_y);
	Pointv2(const T &_x, const T &_y, const T &_z);

	T x;
	T y;
	T z;

	T &operator[](int index);
	Pointv2<T> &operator=(const Vector3 &vector);
};

template<typename T>
inline Pointv2<T>::Pointv2()
{
	memset((void *)this, 0, sizeof(Pointv2<T>));
}

template<typename T>
inline Pointv2<T>::Pointv2(const T &_x, const T &_y)
{
	memset((void *)this, 0, sizeof(Pointv2<T>));
	x = _x;
	y = _y;
}

template<typename T>
inline Pointv2<T>::Pointv2(const T &_x, const T &_y, const T &_z)
{
	x = _x;
	y = _y;
	z = _z;
}

template <typename T>
inline T &Pointv2<T>::operator[](int index)
{
	assert(index < 3);
	return ((T *)&x)[index];
}

template<typename T>
inline Pointv2<T> &Pointv2<T>::operator=(const Vector3 &vector)
{
	x = vector.x;
	y = vector.y;
	z = vector.z;

	return *this;
}

template <typename T>
inline Pointv2<T> operator-(const Pointv2<T> &point1, const Pointv2<T> &point2)
{
	return Pointv2<T>(point1.x - point2.x, point1.y - point2.y, point1.z - point2.z);
}

template <typename T>
inline T find_distance(Pointv2<T> *point1, Pointv2<T> *point2)
{
	Pointv2<T> temp = *point1 - *point2;
	return (T)math::sqrt(math::pow2(temp.x) + math::pow2(temp.y) + math::pow2(temp.z));
}

template <typename T>
struct Triangle {
	Triangle();
	Triangle(Pointv2<T> a, Pointv2<T> b, Pointv2<T> c);

	Pointv2<T> a;
	Pointv2<T> b;
	Pointv2<T> c;

	T find_area();
};

template <typename T>
Triangle<T>::Triangle()
{
}

template <typename T>
Triangle<T>::Triangle(Pointv2<T> a, Pointv2<T> b, Pointv2<T> c) : a(a), b(b), c(c) 
{
}

template <typename T>
inline T Triangle<T>::find_area()
{
	Vector3 v1 = Vector3((float)b.x, (float)b.y, (float)b.z) - Vector3((float)a.x, (float)a.y, (float)a.z);
	Vector3 v2 = Vector3((float)c.x, (float)c.y, (float)c.z) - Vector3((float)a.x, (float)a.y, (float)a.z);
	float parallelogram_area = length(cross(v1, v2));
	return (T)(parallelogram_area * 0.5f);
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
	Rect();
	Rect(T _width, T _height);
	Rect(T _x, T _y, T _width, T _height);
	Rect(Size<T> &size);

	T x;
	T y;
	T width;
	T height;

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

template<typename T>
inline Rect<T>::Rect()
{
	memset((void *)this, 0, sizeof(Rect<T>));
}

template<typename T>
inline Rect<T>::Rect(T _width, T _height)
{
	memset((void *)this, 0, sizeof(Rect<T>));
	
	width = _width;
	height = _height;
}

template<typename T>
inline Rect<T>::Rect(T _x, T _y, T _width, T _height)
{
	x = _x;
	y = _y;
	width = _width;
	height = _height;
}

template<typename T>
inline Rect<T>::Rect(Size<T>& size)
{
	memset((void *)this, 0, sizeof(Rect<T>));

	width = size.width;
	height = size.height;
}

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

typedef Pointv2<s32> Point_s32;
typedef Pointv2<u32> Point_u32;
typedef Pointv2<float> Point_f32;

typedef Rect<u32> Rect_u32;
typedef Rect<s32> Rect_s32;
typedef Rect<float> Rect_f32;

#endif