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

inline Point_u32 convert_1d_to_3d_index(u32 one_dimensional_index, u32 height, u32 depth)
{
	u32 i = one_dimensional_index / (height * depth);
	u32 j = (one_dimensional_index % (height * depth)) / depth;
	u32 k = one_dimensional_index % depth;
	return Point_u32(i, j, k);
}

inline u32 convert_3d_to_1d_index(u32 x, u32 y, u32 z, u32 width, u32 depth)
{
	//return x + width * (y + depth * z);
	return x * (width * depth) + y * depth + z;
}

template <typename T>
struct Point2D {
	Point2D();
	Point2D(const T &x, const T &y);
	Point2D(const Point2D<T> &other);
	template <typename U>
	Point2D(const Point2D<U> &other);
	
	T x;
	T y;

	Point2D<T> &operator=(const Point2D<T> &other);
	template <typename U>
	Point2D<T> &operator=(const Point2D<U> &other);

	T &operator[](u32 index);

	Point2D<T> &operator+=(const T &value);
	Point2D<T> &operator-=(const T &value);
	Point2D<T> &operator*=(const T &value);
	Point2D<T> &operator/=(const T &value);

	Point2D<T> &operator+=(const Point2D<T> &other);
	Point2D<T> &operator-=(const Point2D<T> &other);
	Point2D<T> &operator*=(const Point2D<T> &other);
	Point2D<T> &operator/=(const Point2D<T> &other);
};

typedef Point2D<s32>   Point2DS32;
typedef Point2D<u32>   Point2DU32;
typedef Point2D<float> Point2DF32;

template <typename T>
inline Point2D<T> operator+(const Point2D<T> &first_point, const Point2D<T> &second_point);
template <typename T>
inline Point2D<T> operator-(const Point2D<T> &first_point, const Point2D<T> &second_point);
template <typename T> 
inline Point2D<T> operator*(const Point2D<T> &first_point, const Point2D<T> &second_point);
template <typename T>
inline Point2D<T> operator/(const Point2D<T> &first_point, const Point2D<T> &second_point);

template <typename T>
struct Point3D : Point2D<T> {
	Point3D();
	Point3D(const T &x, const T &y, const T &z);
	Point3D(const Point2D<T> &point, const T &z);
	Point3D(const Point3D<T> &other);
	template <typename U>
	Point3D(const Point3D<U> &other);

	T z;

	Point3D<T> &operator=(const Point3D<T> &other);
	template <typename U>
	Point3D<T> &operator=(const Point3D<U> &other);

	T &operator[](u32 index);

	Point3D<T> &operator+=(const T &value);
	Point3D<T> &operator-=(const T &value);
	Point3D<T> &operator*=(const T &value);
	Point3D<T> &operator/=(const T &value);

	Point3D<T> &operator+=(const Point3D<T> &other);
	Point3D<T> &operator-=(const Point3D<T> &other);
	Point3D<T> &operator*=(const Point3D<T> &other);
	Point3D<T> &operator/=(const Point3D<T> &other);
};

typedef Point3D<s32>   Point3DS32;
typedef Point3D<u32>   Point3DU32;
typedef Point3D<float> Point3DF32;

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
	Point3DS32 x = { 10, 20, 30 };
	Point3DS32 y = { 30, 20, 10 };
	auto resutl = x + y;
}

void update_test()
{
}

template<typename T>
inline Point2D<T>::Point2D()
{
}

template<typename T>
inline Point2D<T>::Point2D(const T &x, const T &y) : x(x), y(y)
{
}

template<typename T>
inline Point2D<T>::Point2D(const Point2D<T> &other)
{
	*this = other;
}

template<typename T>
template<typename U>
inline Point2D<T>::Point2D(const Point2D<U> &other)
{
	*this = other;
}

template<typename T>
inline Point2D<T> &Point2D<T>::operator=(const Point2D<T> &other)
{
	if (this != &other) {
		x = other.x;
		y = other.y;
	}
	return *this;
}

template<typename T>
template<typename U>
inline Point2D<T> &Point2D<T>::operator=(const Point2D<U> &other)
{
	if (this != &other) {
		x = static_cast<T>(other.x);
		y = static_cast<T>(other.y);
	}
	return *this;
}

template<typename T>
inline T &Point2D<T>::operator[](u32 index)
{
	assert(index < 2);
	return ((T *)&x)[index];
}

template<typename T>
inline Point2D<T> &Point2D<T>::operator+=(const T &value)
{
	x += value;
	y += value;
	return *this;
}

template<typename T>
inline Point2D<T> &Point2D<T>::operator-=(const T &value)
{
	x -= value;
	y -= value;
	return *this;
}

template<typename T>
inline Point2D<T> &Point2D<T>::operator*=(const T &value)
{
	x *= value;
	y *= value;
	return *this;
}

template<typename T>
inline Point2D<T> &Point2D<T>::operator/=(const T &value)
{
	x /= value;
	y /= value;
	return *this;
}

template<typename T>
inline Point2D<T> &Point2D<T>::operator+=(const Point2D<T> &other)
{
	x += other.x;
	y += other.y;
	return *this;
}

template<typename T>
inline Point2D<T> &Point2D<T>::operator-=(const Point2D<T> &other)
{
	x -= other.x;
	y -= other.y;
	return *this;
}

template<typename T>
inline Point2D<T> &Point2D<T>::operator*=(const Point2D<T> &other)
{
	x *= other.x;
	y *= other.y;
	return *this;
}

template<typename T>
inline Point2D<T> &Point2D<T>::operator/=(const Point2D<T> &other)
{
	x /= other.x;
	y /= other.y;
	return *this;
}

template<typename T>
inline Point2D<T> operator+(const Point2D<T> &first_point, const Point2D<T> &second_point)
{
	Point2D<T> result;
	result.x = first_point.x + second_point.x;
	result.y = first_point.y + second_point.y;
	return result;
}

template<typename T>
inline Point2D<T> operator-(const Point2D<T> &first_point, const Point2D<T> &second_point)
{
	Point2D<T> result;
	result.x = first_point.x - second_point.x;
	result.y = first_point.y - second_point.y;
	return result;
}

template<typename T>
inline Point2D<T> operator*(const Point2D<T> &first_point, const Point2D<T> &second_point)
{
	Point2D<T> result;
	result.x = first_point.x * second_point.x;
	result.y = first_point.y * second_point.y;
	return result;
}

template<typename T>
inline Point2D<T> operator/(const Point2D<T> &first_point, const Point2D<T> &second_point)
{
	Point2D<T> result;
	result.x = first_point.x / second_point.x;
	result.y = first_point.y / second_point.y;
	return result;
}

template<typename T>
inline Point3D<T>::Point3D()
{
}

template<typename T>
inline Point3D<T>::Point3D(const T &x, const T &y, const T &z) : Point2D<T>(x, y), z(z)
{
}

template<typename T>
Point3D<T>::Point3D(const Point2D<T> &point, const T &z) : Point2D<T>(point), z(z)
{
}

template<typename T>
inline Point3D<T>::Point3D(const Point3D<T> &other)
{
	*this = other;
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
		((Point2D<T>)*this) = (Point2D<T>)other;
		z = other.z;
	}
	return *this;
}

template<typename T>
template<typename U>
Point3D<T> &Point3D<T>::operator=(const Point3D<U> &other)
{
	if (this != &other) {
		((Point2D<T>)*this) = (Point2D<U>)other;
		z = static_cast<T>(other.z);
	}
	return *this;
}

template<typename T>
T &Point3D<T>::operator[](u32 index)
{
	assert(index < 3);
	return ((T *)this)[index];
}

#define TO_POINT2D(_this) ((Point2D<T>)*_this)

template<typename T>
Point3D<T> &Point3D<T>::operator+=(const T &value)
{
	TO_POINT2D(this) += value;
	z += value;
	return *this;
}

template<typename T>
Point3D<T> &Point3D<T>::operator-=(const T &value)
{
	TO_POINT2D(this) -= value;
	z -= value;
	return *this;
}

template<typename T>
Point3D<T> &Point3D<T>::operator*=(const T &value)
{
	TO_POINT2D(this) *= value;
	z *= value;
	return *this;
}

template<typename T>
Point3D<T> &Point3D<T>::operator/=(const T &value)
{
	TO_POINT2D(this) /= value;
	z /= value;
	return *this;
}

template<typename T>
Point3D<T> &Point3D<T>::operator+=(const Point3D<T> &other)
{
	TO_POINT2D(this) += other;
	z += other.z;
	return *this;
}

template<typename T>
Point3D<T> &Point3D<T>::operator-=(const Point3D<T> &other)
{
	TO_POINT2D(this) -= other;
	z -= other.z;
	return *this;
}

template<typename T>
Point3D<T> &Point3D<T>::operator*=(const Point3D<T> &other)
{
	TO_POINT2D(this) *= other;
	z *= other.z;
	return *this;
}

template<typename T>
Point3D<T> &Point3D<T>::operator/=(const Point3D<T> &other)
{
	TO_POINT2D(this) /= other;
	z /= other.z;
	return *this;
}

template<typename T>
inline Point3D<T> operator+(const Point3D<T> &first_point, const Point3D<T> &second_point)
{
	Point2D<T> result = (Point2D<T>)first_point + (Point2D<T>)second_point;
	return Point3D<T>(result, first_point.z + second_point.z);
}

template<typename T>
inline Point3D<T> operator-(const Point3D<T> &first_point, const Point3D<T> &second_point)
{
	Point2D<T> result = (Point2D<T>)first_point - (Point2D<T>)second_point;
	return Point3D<T>(result, first_point.z - second_point.z);
}

template<typename T>
inline Point3D<T> operator*(const Point3D<T> &first_point, const Point3D<T> &second_point)
{
	Point2D<T> result = (Point2D<T>)first_point * (Point2D<T>)second_point;
	return Point3D<T>(result, first_point.z * second_point.z);
}

template<typename T>
inline Point3D<T> operator/(const Point3D<T> &first_point, const Point3D<T> &second_point)
{
	Point2D<T> result = (Point2D<T>)first_point / (Point2D<T>)second_point;
	return Point3D<T>(result, first_point.z / second_point.z);
}