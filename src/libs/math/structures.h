#ifndef MATH_STRUCTURES_H
#define MATH_STRUCTURES_H

#include <assert.h>
#include <string.h>

#include "vector.h"
#include "functions.h"
#include "../number_types.h"

struct Ray {
	Ray();
	Ray(const Vector3 &_origin, const Vector3 &_direction);

	float len;
	Vector3 origin;
	Vector3 direction;
};

template <typename T>
struct Size3D {
	Size3D();
	Size3D(const T &width, const T &height);
	Size3D(const T &width, const T &height, const T &depth);
	Size3D(const Size3D<T> &other);
	template <typename U>
	Size3D(const Size3D<U> &other);

	T width;
	T height;
	T depth;

	Size3D<T> &operator=(const Size3D<T> &other);
	template <typename U>
	Size3D<T> &operator=(const Size3D<U> &other);

	T &operator[](u32 index);

	Size3D<T> &operator+=(const T &value);
	Size3D<T> &operator-=(const T &value);
	Size3D<T> &operator*=(const T &value);
	Size3D<T> &operator/=(const T &value);

	Size3D<T> &operator+=(const Size3D<T> &other);
	Size3D<T> &operator-=(const Size3D<T> &other);
	Size3D<T> &operator*=(const Size3D<T> &other);
	Size3D<T> &operator/=(const Size3D<T> &other);

	void set(const T &_width, const T &_height);
	void set(const T &_width, const T &_height, const T &_depth);

	T find_area();
	Vector2 to_vector2();
	Vector3 to_vector3();
};

typedef Size3D<s32> Size_s32;
typedef Size3D<u32> Size_u32;
typedef Size3D<float> Size_f32;

template <typename T>
inline Size3D<T> operator+(const Size3D<T> &size, const T &value);
template <typename T>
inline Size3D<T> operator-(const Size3D<T> &size, const T &value);
template <typename T>
inline Size3D<T> operator*(const Size3D<T> &size, const T &value);
template <typename T>
inline Size3D<T> operator/(const Size3D<T> &size, const T &value);

template <typename T>
inline Size3D<T> operator+(const Size3D<T> &first_size, const Size3D<T> &second_size);
template <typename T>
inline Size3D<T> operator-(const Size3D<T> &first_size, const Size3D<T> &second_size);
template <typename T>
inline Size3D<T> operator*(const Size3D<T> &first_size, const Size3D<T> &second_size);
template <typename T>
inline Size3D<T> operator/(const Size3D<T> &first_size, const Size3D<T> &second_size);

template <typename T>
struct Point3D {
	Point3D();
	Point3D(const T &x, const T &y);
	Point3D(const T &x, const T &y, const T &z);
	Point3D(const Size3D<T> &size);
	Point3D(const Point3D<T> &other);
	template <typename U>
	Point3D(const Point3D<U> &other);
	Point3D(const Vector2 &vector);
	Point3D(const Vector3 &vector);

	T x;
	T y;
	T z;

	Point3D<T> &operator=(const Point3D<T> &other);
	template <typename U>
	Point3D<T> &operator=(const Point3D<U> &other);

	Point3D<T> &operator=(const Vector2 &vector);
	Point3D<T> &operator=(const Vector3 &vector);

	T &operator[](u32 index);

	Point3D<T> &operator+=(const T &value);
	Point3D<T> &operator-=(const T &value);
	Point3D<T> &operator*=(const T &value);
	Point3D<T> &operator/=(const T &value);

	Point3D<T> &operator+=(const Point3D<T> &other);
	Point3D<T> &operator-=(const Point3D<T> &other);
	Point3D<T> &operator*=(const Point3D<T> &other);
	Point3D<T> &operator/=(const Point3D<T> &other);

	void move(const T &x_delta, const T &y_delta, const T &z_delta = { 0 });
	Vector2 to_vector2() const;
	Vector3 to_vector3() const;
};

typedef Point3D<s32> Point_s32;
typedef Point3D<u32> Point_u32;
typedef Point3D<float> Point_f32;

template <typename T>
inline Point3D<T> operator+(const Point3D<T> &point, const T &value);
template <typename T>
inline Point3D<T> operator-(const Point3D<T> &point, const T &value);
template <typename T>
inline Point3D<T> operator*(const Point3D<T> &point, const T &value);
template <typename T>
inline Point3D<T> operator/(const Point3D<T> &point, const T &value);

template <typename T>
inline Point3D<T> operator+(const Point3D<T> &first_point, const Point3D<T> &second_point);
template <typename T>
inline Point3D<T> operator-(const Point3D<T> &first_point, const Point3D<T> &second_point);
template <typename T>
inline Point3D<T> operator*(const Point3D<T> &first_point, const Point3D<T> &second_point);
template <typename T>
inline Point3D<T> operator/(const Point3D<T> &first_point, const Point3D<T> &second_point);

template <typename T>
struct Triangle {
	Triangle();
	Triangle(const Point3D<T> &a, const Point3D<T> &b, const Point3D<T> &c);

	Point3D<T> a;
	Point3D<T> b;
	Point3D<T> c;

	T find_area() const;
};

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
	Rect(T x, T y, T width, T height);
	Rect(Size3D<T> &size);

	T x;
	T y;
	T width;
	T height;

	T &operator[](u32 index);

	void set(Size3D<T> &size);
	void set(T _x, T _y);
	void move(T x_delta, T y_delta);
	void offset_y(T _y);
	void offset_x(T _x);
	void set_size(T _width, T _height);
	Size3D<T> get_size();

	T right();
	T bottom();
};

typedef Rect<s32> Rect_s32;
typedef Rect<u32> Rect_u32;
typedef Rect<float> Rect_f32;

template <typename T, typename U>
struct Pair {
	Pair();
	~Pair();
	Pair(const T &first, const U &second);

	T first;
	U second;
};

template<typename T>
Size3D<T>::Size3D()
{
	width = { 0 };
	height = { 0 };
	depth = { 0 };
}

template<typename T>
Size3D<T>::Size3D(const T &width, const T &height) : width(width), height(height), depth{ 0 }
{
}

template<typename T>
Size3D<T>::Size3D(const T &width, const T &height, const T &depth) : width(width), height(height), depth(depth)
{
}

template<typename T>
Size3D<T>::Size3D(const Size3D<T> &other)
{
	*this = other;
}

template<typename T>
template<typename U>
Size3D<T>::Size3D(const Size3D<U> &other)
{
	*this = other;
}

template<typename T>
Size3D<T> &Size3D<T>::operator=(const Size3D<T> &other)
{
	if (this != &other) {
		width = other.width;
		height = other.height;
		depth = other.depth;
	}
	return *this;
}

template<typename T>
template<typename U>
Size3D<T> &Size3D<T>::operator=(const Size3D<U> &other)
{
	width = static_cast<T>(other.width);
	height = static_cast<T>(other.height);
	depth = static_cast<T>(other.depth);
	return *this;
}

template<typename T>
T &Size3D<T>::operator[](u32 index)
{
	assert(index < 3);
	return ((T *)this)[index];
}

template<typename T>
Size3D<T> &Size3D<T>::operator+=(const T &value)
{
	width += value;
	height += value;
	depth += value;
	return *this;
}

template<typename T>
Size3D<T> &Size3D<T>::operator-=(const T &value)
{
	width -= value;
	height -= value;
	depth -= value;
	return *this;
}

template<typename T>
Size3D<T> &Size3D<T>::operator*=(const T &value)
{
	width *= value;
	height *= value;
	depth *= value;
	return *this;
}

template<typename T>
Size3D<T> &Size3D<T>::operator/=(const T &value)
{
	width /= value;
	height /= value;
	depth /= value;
	return *this;
}

template<typename T>
Size3D<T> &Size3D<T>::operator+=(const Size3D<T> &other)
{
	width += other.width;
	height += other.height;
	depth += other.depth;
	return *this;
}

template<typename T>
Size3D<T> &Size3D<T>::operator-=(const Size3D<T> &other)
{
	width -= other.width;
	height -= other.height;
	depth -= other.depth;
	return *this;
}

template<typename T>
Size3D<T> &Size3D<T>::operator*=(const Size3D<T> &other)
{
	width *= other.width;
	height *= other.height;
	depth *= other.depth;
	return *this;
}

template<typename T>
Size3D<T> &Size3D<T>::operator/=(const Size3D<T> &other)
{
	width /= other.width;
	height /= other.height;
	depth /= other.depth;
	return *this;
}

template<typename T>
inline void Size3D<T>::set(const T &_width, const T &_height)
{
	width = _width;
	height = _height;
}

template<typename T>
inline void Size3D<T>::set(const T &_width, const T &_height, const T &_depth)
{
	width = _width;
	height = _height;
	depth = _depth;
}


template<typename T>
inline T Size3D<T>::find_area()
{
	return width * height * depth;
}

template<typename T>
inline Vector2 Size3D<T>::to_vector2()
{
	return Vector2(static_cast<float>(width), static_cast<float>(height));
}

template<typename T>
inline Vector3 Size3D<T>::to_vector3()
{
	return Vector3(static_cast<float>(width), static_cast<float>(height), static_cast<float>(depth));
}

template<typename T>
inline Size3D<T> operator+(const Size3D<T> &size, const T &value)
{
	Size3D<T> result = size;
	result += value;
	return result;
}

template<typename T>
inline Size3D<T> operator-(const Size3D<T> &size, const T &value)
{
	Size3D<T> result = size;
	result -= value;
	return result;
}

template<typename T>
inline Size3D<T> operator*(const Size3D<T> &size, const T &value)
{
	Size3D<T> result = size;
	result *= value;
	return result;
}

template<typename T>
inline Size3D<T> operator/(const Size3D<T> &size, const T &value)
{
	Size3D<T> result = size;
	result /= value;
	return result;
}

template<typename T>
inline Size3D<T> operator+(const Size3D<T> &first_size, const Size3D<T> &second_size)
{
	Size3D<T> result = first_size;
	result += second_size;
	return result;
}

template<typename T>
inline Size3D<T> operator-(const Size3D<T> &first_size, const Size3D<T> &second_size)
{
	Size3D<T> result = first_size;
	result -= second_size;
	return result;
}

template<typename T>
inline Size3D<T> operator*(const Size3D<T> &first_size, const Size3D<T> &second_size)
{
	Size3D<T> result = first_size;
	result *= second_size;
	return result;
}

template<typename T>
inline Size3D<T> operator/(const Size3D<T> &first_size, const Size3D<T> &second_size)
{
	Size3D<T> result = first_size;
	result /= second_size;
	return result;
}

template<typename T>
inline Point3D<T>::Point3D() : x{0}, y{0}, z{0}
{
}

template<typename T>
Point3D<T>::Point3D(const T &x, const T &y) : x(x), y(y), z{0}
{
}

template<typename T>
inline Point3D<T>::Point3D(const T &x, const T &y, const T &z) : x(x), y(y), z(z)
{
}

template<typename T>
inline Point3D<T>::Point3D(const Size3D<T> &size) : x(size.width), y(size.height), z(size.depth)
{
}

template<typename T>
inline Point3D<T>::Point3D(const Point3D<T> &other)
{
	*this = other;
}

template<typename T>
Point3D<T>::Point3D(const Vector2 &vector)
{
	*this = vector;
}

template<typename T>
Point3D<T>::Point3D(const Vector3 &vector)
{
	*this = vector;
}

template<typename T>
template<typename U>
Point3D<T>::Point3D(const Point3D<U> &other)
{
	*this = other;
}

template<typename T>
Point3D<T> &Point3D<T>::operator=(const Point3D<T> &other)
{
	if (this != &other) {
		x = other.x;
		y = other.y;
		z = other.z;
	}
	return *this;
}

template<typename T>
template<typename U>
Point3D<T> &Point3D<T>::operator=(const Point3D<U> &other)
{
	x = static_cast<T>(other.x);
	y = static_cast<T>(other.y);
	z = static_cast<T>(other.z);
	return *this;
}

template<typename T>
Point3D<T> &Point3D<T>::operator=(const Vector2 &vector)
{
	x = static_cast<T>(vector.x);
	y = static_cast<T>(vector.y);
	z = { 0 };
	return *this;
}

template<typename T>
Point3D<T> &Point3D<T>::operator=(const Vector3 &vector)
{
	x = static_cast<T>(vector.x);
	y = static_cast<T>(vector.y);
	z = static_cast<T>(vector.z);
	return *this;
}

template<typename T>
T &Point3D<T>::operator[](u32 index)
{
	assert(index < 3);
	return ((T *)this)[index];
}

template<typename T>
Point3D<T> &Point3D<T>::operator+=(const T &value)
{
	x += value;
	y += value;
	z += value;
	return *this;
}

template<typename T>
Point3D<T> &Point3D<T>::operator-=(const T &value)
{
	x -= value;
	y -= value;
	z -= value;
	return *this;
}

template<typename T>
Point3D<T> &Point3D<T>::operator*=(const T &value)
{
	x *= value;
	y *= value;
	z *= value;
	return *this;
}

template<typename T>
Point3D<T> &Point3D<T>::operator/=(const T &value)
{
	x /= value;
	y /= value;
	z /= value;
	return *this;
}

template<typename T>
Point3D<T> &Point3D<T>::operator+=(const Point3D<T> &other)
{
	x += other.x;
	y += other.y;
	z += other.z;
	return *this;
}

template<typename T>
Point3D<T> &Point3D<T>::operator-=(const Point3D<T> &other)
{
	x -= other.x;
	y -= other.y;
	z -= other.z;
	return *this;
}

template<typename T>
Point3D<T> &Point3D<T>::operator*=(const Point3D<T> &other)
{
	x *= other.x;
	y *= other.y;
	z *= other.z;
	return *this;
}

template<typename T>
Point3D<T> &Point3D<T>::operator/=(const Point3D<T> &other)
{
	x /= other.x;
	y /= other.y;
	z /= other.z;
	return *this;
}

template<typename T>
void Point3D<T>::move(const T &x_delta, const T &y_delta, const T &z_delta)
{
	*this += Point3D<T>(x_delta, y_delta, z_delta);
}

template<typename T>
inline Vector2 Point3D<T>::to_vector2() const 
{
	return Vector2(static_cast<float>(x), static_cast<float>(y));
}

template<typename T>
inline Vector3 Point3D<T>::to_vector3() const
{
	return Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}

template<typename T>
inline Point3D<T> operator+(const Point3D<T> &point, const T &value)
{
	Point3D<T> result = point;
	point += value;
	return result;
}

template<typename T>
inline Point3D<T> operator-(const Point3D<T> &point, const T &value)
{
	Point3D<T> result = point;
	point -= value;
	return result;
}

template<typename T>
inline Point3D<T> operator*(const Point3D<T> &point, const T &value)
{
	Point3D<T> result = point;
	point *= value;
	return result;
}

template<typename T>
inline Point3D<T> operator/(const Point3D<T> &point, const T &value)
{
	Point3D<T> result = point;
	result /= value;
	return result;
}

template<typename T>
inline Point3D<T> operator+(const Point3D<T> &first_point, const Point3D<T> &second_point)
{
	Point3D<T> result = first_point;
	result += second_point;
	return result;
}

template<typename T>
inline Point3D<T> operator-(const Point3D<T> &first_point, const Point3D<T> &second_point)
{
	Point3D<T> result = first_point;
	result -= second_point;
	return result;
}

template<typename T>
inline Point3D<T> operator*(const Point3D<T> &first_point, const Point3D<T> &second_point)
{
	Point3D<T> result = first_point;
	result *= second_point;
	return result;
}

template<typename T>
inline Point3D<T> operator/(const Point3D<T> &first_point, const Point3D<T> &second_point)
{
	Point3D<T> result = first_point;
	result /= second_point;
	return result;
}

template <typename T>
Triangle<T>::Triangle()
{
}

template <typename T>
Triangle<T>::Triangle(const Point3D<T> &a, const Point3D<T> &b, const Point3D<T> &c) : a(a), b(b), c(c)
{
}

template <typename T>
inline T Triangle<T>::find_area() const
{
	Point3D<T> p1 = b - a;
	Point3D<T> p2 = c - a;
	float parallelogram_area = length(cross(p1.to_vector3(), p2.to_vector3()));
	return (T)(parallelogram_area * 0.5f);
}

template<typename T>
inline Rect<T>::Rect()
{
	memset((void *)this, 0, sizeof(Rect<T>));
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
inline Rect<T>::Rect(Size3D<T> &size)
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
inline void Rect<T>::set(Size3D<T> &size)
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
inline void Rect<T>::move(T x_delta, T y_delta)
{
	x += x_delta;
	y += y_delta;
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
inline Size3D<T> Rect<T>::get_size()
{
	return Size3D<T>(width, height);
}

template<typename T, typename U>
inline Pair<T, U>::Pair()
{
}

template<typename T, typename U>
inline Pair<T, U>::~Pair()
{
}

template<typename T, typename U>
inline Pair<T, U>::Pair(const T &first, const U &second) : first(first), second(second)
{
}
#endif
