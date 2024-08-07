#include "test.h"
#include "../libs/str.h"
#include "../sys/sys.h"
#include "../libs/str.h"
#include "../libs/number_types.h"
#include "../libs/math/vector.h"

inline u32 pack_RGB(const Vector3 &rgb_value)
{
	u32 r = u32(255.0f * rgb_value.x);
	u32 g = u32(255.0f * rgb_value.y);
	u32 b = u32(255.0f * rgb_value.z);

	u32 result = 0;
	result |= r << 24;
	result |= g << 16;
	result |= b << 8;
	result |= 0xff;
	return result;
}

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
};

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

	void move(const T &x_delta, const T &y_delta, const T &z_delta = {0});
};

template <typename T>
inline Point3D<T> operator+(const Point3D<T> &first_point, const Point3D<T> &second_point);
template <typename T>
inline Point3D<T> operator-(const Point3D<T> &first_point, const Point3D<T> &second_point);
template <typename T>
inline Point3D<T> operator*(const Point3D<T> &first_point, const Point3D<T> &second_point);
template <typename T>
inline Point3D<T> operator/(const Point3D<T> &first_point, const Point3D<T> &second_point);

void test()
{
	Point3D<s32> x = { 10, 20 };
	Point3D<float> y = { 1.0f, 2.0f };
}

void update_test()
{
}

template<typename T>
inline Point3D<T>::Point3D() : x{0}, y{0}, z{0}
{
}

template<typename T>
Point3D<T>::Point3D(const T &x, const T &y) : x(x), y(y)
{
	z = {0};
}

template<typename T>
inline Point3D<T>::Point3D(const T &x, const T &y, const T &z) : x(x), y(y), z(z)
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
	if (this != &other) {
		x = static_cast<T>(other.x);
		y = static_cast<T>(other.y);
		z = static_cast<T>(other.z);
	}
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

template<typename T>
Size3D<T>::Size3D()
{
	width = {0};
	height = {0};
	depth = {0};
}

template<typename T>
Size3D<T>::Size3D(const T &width, const T &height) : width(width), height(height), depth{0}
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
	if (this != &other) {
		width = static_cast<T>(other.width);
		height = static_cast<T>(other.height);
		depth = static_cast<T>(other.depth);
	}
	return *this;
}

template<typename T>
T &Size3D<T>::operator[](u32 index)
{
	assert(index < 3);
	((T *)this)[index];
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
