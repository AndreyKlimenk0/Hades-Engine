#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>
#include <assert.h>
#include <DirectXMath.h>

using namespace DirectX;

struct Vector2 {
	float x;
	float y;
	
	Vector2() {};
	Vector2(float _x, float _y) : x(_x), y(_y) {}

	float  &operator[](int i);
	const float  &operator[](int i) const;
	Vector2 operator+(float scalar);
	Vector2 operator+(const Vector2 &other);
	Vector2 operator-(float scalar);
	Vector2 operator-(const Vector2 &other);
	Vector2 operator*(float scalar);
	Vector2 operator*(const Vector2 &other);
	Vector2 operator/(float scalar);
	Vector2 operator/(const Vector2 &other);

	Vector2 &operator+=(float);
	Vector2 &operator+=(const Vector2 &other);
	Vector2 &operator-=(float);
	Vector2 &operator-=(const Vector2 &other);
	Vector2 &operator*=(float);
	Vector2 &operator*=(const Vector2 &other);
	Vector2 &operator/=(float);
	Vector2 &operator/=(const Vector2 &other);
	
	operator XMVECTOR();

	float   length();
	Vector2 normalize();
	float dot(const Vector2 & other);
};

inline float &Vector2::operator[](int i)
{
	assert(i < 2);
	return (&x)[i];
}

inline const float &Vector2::operator[](int i) const
{
	assert(i < 2);
	return (&x)[i];
}

inline Vector2 Vector2::operator+(float scalar) 
{
	return Vector2(x + scalar, y + scalar);
}

inline Vector2 Vector2::operator+(const Vector2 &other) 
{
	return Vector2(x + other.x, y + other.y);
}

inline Vector2 Vector2::operator-(float scalar) 
{
	return Vector2(x - scalar, y - scalar);
}

inline Vector2 Vector2::operator-(const Vector2 &other) 
{
	return Vector2(x - other.x, y - other.y);
}

inline Vector2 Vector2::operator*(float scalar) 
{
	return Vector2(x * scalar, y * scalar);
}
inline Vector2 Vector2::operator*(const Vector2 &other) 
{
	return Vector2(x * other.x, y * other.y);
}

inline Vector2 Vector2::operator/(float scalar) 
{
	float inver = 1.0f / scalar;
	return Vector2(x * inver, y * inver);
}

inline Vector2 Vector2::operator/(const Vector2 &other) 
{
	float inver_x = 1.0f / other.x;
	float inver_y = 1.0f / other.y;
	return Vector2(x * inver_x, y * inver_y);
}

inline Vector2 &Vector2::operator+=(float scalar)
{
	x += scalar;
	y += scalar;
	return *this;
}

inline Vector2 &Vector2::operator+=(const Vector2 &other)
{
	x += other.x;
	y += other.y;
	return *this;
}

inline Vector2 &Vector2::operator-=(float scalar)
{
	x -= scalar;
	y -= scalar;
	return *this;
}

inline Vector2 &Vector2::operator-=(const Vector2 &other)
{
	x -= other.x;
	y -= other.y;
	return *this;
}

inline Vector2 &Vector2::operator*=(float scalar)
{
	x *= scalar;
	y *= scalar;
	return *this;
}

inline Vector2 &Vector2::operator*=(const Vector2 &other)
{
	x *= other.x;
	y *= other.y;
	return *this;
}

inline Vector2 &Vector2::operator/=(float scalar)
{
	float inver = 1.0f / scalar;
	x *= inver;
	y *= inver;
	return *this;
}

inline Vector2 &Vector2::operator/=(const Vector2 &other)
{
	x *= 1.0f / other.x;
	y *= 1.0f / other.y;
	return *this;
}

inline Vector2::operator XMVECTOR()
{
	return XMVectorSet(x, y, 0.0f, 0.0f);
}

inline float Vector2::length()
{
	return (float)sqrt(x * x + y * y);
}

inline Vector2 Vector2::normalize()
{
	return Vector2(x / length(), y / length());
}

inline float Vector2::dot(const Vector2 &other)
{
	return x * other.x + y * other.y;
}

struct Vector3 {
	float x;
	float y;
	float z;

	Vector3() {};
	Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

	float  &operator[](int i);
	const float  &operator[](int i) const;
	Vector3 operator+(float scalar);
	Vector3 operator+(const Vector3 &other);
	Vector3 operator-(float scalar);
	Vector3 operator-(const Vector3 &other);
	Vector3 operator*(float scalar);
	Vector3 operator*(const Vector3 &other);
	Vector3 operator/(float scalar);
	Vector3 operator/(const Vector3 &other);

	Vector3 &operator+=(float);
	Vector3 &operator+=(const Vector3 &other);
	Vector3 &operator-=(float);
	Vector3 &operator-=(const Vector3 &other);
	Vector3 &operator*=(float);
	Vector3 &operator*=(const Vector3 &other);
	Vector3 &operator/=(float);
	Vector3 &operator/=(const Vector3 &other);

	operator XMVECTOR();
	operator Vector2();

	Vector3 normalize();
	Vector3 cross(const Vector3 &other);
	float   length();
	float dot(const Vector3 & other);
};

inline float &Vector3::operator[](int i)
{
	assert(i < 3);
	return (&x)[i];
}

inline const float &Vector3::operator[](int i) const
{
	assert(i < 3);
	return (&x)[i];
}

inline Vector3 Vector3::operator+(float scalar)
{
	return Vector3(x + scalar, y + scalar, z + scalar);
}

inline Vector3 Vector3::operator+(const Vector3 &other)
{
	return Vector3(x + other.x, y + other.y, z + other.z);
}

inline Vector3 Vector3::operator-(float scalar)
{
	return Vector3(x - scalar, y - scalar, z - scalar);
}

inline Vector3 Vector3::operator-(const Vector3 &other)
{
	return Vector3(x - other.x, y - other.y, z - other.z);
}

inline Vector3 Vector3::operator*(float scalar)
{
	return Vector3(x * scalar, y * scalar, z * scalar);
}

inline Vector3 Vector3::operator*(const Vector3 &other)
{
	return Vector3(x * other.x, y * other.y, z * other.z);
}

inline Vector3 Vector3::operator/(float scalar)
{
	float inver = 1.0f / scalar;
	return Vector3(x * inver, y * inver, z * inver);
}

inline Vector3 Vector3::operator/(const Vector3 &other)
{
	float inver_x = 1.0f / other.x;
	float inver_y = 1.0f / other.y;
	float inver_z = 1.0f / other.z;
	return Vector3(x * inver_x, y * inver_y, z * inver_z);
}

inline Vector3 &Vector3::operator+=(float scalar)
{
	x += scalar;
	y += scalar;
	z += scalar;
	return *this;
}

inline Vector3 &Vector3::operator+=(const Vector3 &other)
{
	x += other.x;
	y += other.y;
	z += other.z;
	return *this;
}

inline Vector3 &Vector3::operator-=(float scalar)
{
	x -= scalar;
	y -= scalar;
	z -= scalar;
	return *this;
}

inline Vector3 &Vector3::operator-=(const Vector3 &other)
{
	x -= other.x;
	y -= other.y;
	z -= other.z;
	return *this;
}

inline Vector3 &Vector3::operator*=(float scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
	return *this;
}

inline Vector3 &Vector3::operator*=(const Vector3 &other)
{
	x *= other.x;
	y *= other.y;
	z *= other.z;
	return *this;
}

inline Vector3 &Vector3::operator/=(float scalar)
{
	float inver = 1.0f / scalar;
	x *= inver;
	y *= inver;
	z *= inver;
	return *this;
}

inline Vector3 &Vector3::operator/=(const Vector3 &other)
{
	x *= 1.0f / other.x;
	y *= 1.0f / other.y;
	z *= 1.0f / other.z;
	return *this;
}

inline Vector3::operator XMVECTOR()
{
	return XMVectorSet(x, y, z, 0.0f);
}

inline Vector3::operator Vector2()
{
	return Vector2(x, y);
}

inline float Vector3::length()
{
	return (float)sqrt(x * x + y * y + z * z);
}

inline Vector3 Vector3::normalize()
{
	return Vector3(x / length(), y / length(), z / length());
}

inline float Vector3::dot(const Vector3 &other)
{
	return x * other.x + y * other.y + z * other.z;
}

inline Vector3 Vector3::cross(const  Vector3 &other)
{
	return Vector3(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
}


struct Vector4 {
	float x;
	float y;
	float z;
	float w;

	Vector4() {};
	Vector4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

	float  &operator[](int i);
	const float  &operator[](int i) const;
	Vector4 operator+(float scalar);
	Vector4 operator+(const Vector4 &other);
	Vector4 operator-(float scalar);
	Vector4 operator-(const Vector4 &other);
	Vector4 operator*(float scalar);
	Vector4 operator*(const Vector4 &other);
	Vector4 operator/(float scalar);
	Vector4 operator/(const Vector4 &other);

	Vector4 &operator+=(float);
	Vector4 &operator+=(const Vector4 &other);
	Vector4 &operator-=(float);
	Vector4 &operator-=(const Vector4 &other);
	Vector4 &operator*=(float);
	Vector4 &operator*=(const Vector4 &other);
	Vector4 &operator/=(float);
	Vector4 &operator/=(const Vector4 &other);

	operator XMVECTOR();

	Vector4 normalize();
	float   length();
	float dot(const Vector4 & other);
};

inline float &Vector4::operator[](int i)
{
	assert(i < 4);
	return (&x)[i];
}

inline const float &Vector4::operator[](int i) const
{
	assert(i < 4);
	return (&x)[i];
}

inline Vector4 Vector4::operator+(float scalar)
{
	return Vector4(x + scalar, y + scalar, z + scalar, w + scalar);
}

inline Vector4 Vector4::operator+(const Vector4 &other)
{
	return Vector4(x + other.x, y + other.y, z + other.z, w + other.w);
}

inline Vector4 Vector4::operator-(float scalar)
{
	return Vector4(x - scalar, y - scalar, z - scalar, w - scalar);
}

inline Vector4 Vector4::operator-(const Vector4 &other)
{
	return Vector4(x - other.x, y - other.y, z - other.z, w - other.w);
}

inline Vector4 Vector4::operator*(float scalar)
{
	return Vector4(x * scalar, y * scalar, z * scalar, w * scalar);
}

inline Vector4 Vector4::operator*(const Vector4 &other)
{
	return Vector4(x * other.x, y * other.y, z * other.z, w * other.w);
}

inline Vector4 Vector4::operator/(float scalar)
{
	float inver = 1.0f / scalar;
	return Vector4(x * inver, y * inver, z * inver, w * inver);
}

inline Vector4 Vector4::operator/(const Vector4 &other)
{
	float inver_x = 1.0f / other.x;
	float inver_y = 1.0f / other.y;
	float inver_z = 1.0f / other.z;
	float inver_w = 1.0f / other.w;
	return Vector4(x * inver_x, y * inver_y, z * inver_z, w * inver_w);
}

inline Vector4 &Vector4::operator+=(float scalar)
{
	x += scalar;
	y += scalar;
	z += scalar;
	w += scalar;
	return *this;
}

inline Vector4 &Vector4::operator+=(const Vector4 &other)
{
	x += other.x;
	y += other.y;
	z += other.z;
	w += other.w;
	return *this;
}

inline Vector4 &Vector4::operator-=(float scalar)
{
	x -= scalar;
	y -= scalar;
	z -= scalar;
	w -= scalar;
	return *this;
}

inline Vector4 &Vector4::operator-=(const Vector4 &other)
{
	x -= other.x;
	y -= other.y;
	z -= other.z;
	w -= other.w;
	return *this;
}

inline Vector4 &Vector4::operator*=(float scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
	w *= scalar;
	return *this;
}

inline Vector4 &Vector4::operator*=(const Vector4 &other)
{
	x *= other.x;
	y *= other.y;
	z *= other.z;
	w *= other.w;
	return *this;
}

inline Vector4 &Vector4::operator/=(float scalar)
{
	float inver = 1.0f / scalar;
	x *= inver;
	y *= inver;
	z *= inver;
	w *= inver;
	return *this;
}

inline Vector4 &Vector4::operator/=(const Vector4 &other)
{
	x *= 1.0f / other.x;
	y *= 1.0f / other.y;
	z *= 1.0f / other.z;
	w *= 1.0f / other.w;
	return *this;
}

inline Vector4::operator XMVECTOR()
{
	return XMVectorSet(x, y, z, w);
}

inline float Vector4::length()
{
	return (float)sqrt(x * x + y * y + z * z + w * w);
}

inline Vector4 Vector4::normalize()
{
	return Vector4(x / length(), y / length(), z / length(), w / length());
}

inline float Vector4::dot(const Vector4 &other)
{
	return x * other.x + y * other.y + z * other.z * w + other.w;
}

#endif