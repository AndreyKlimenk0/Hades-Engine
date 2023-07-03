#ifndef VECTOR_H
#define VECTOR_H

#include "../math/common.h"
#include <assert.h>
#include <DirectXMath.h>

using namespace DirectX;

struct Vector2 : XMFLOAT2 {
	Vector2() {}
	Vector2(XMVECTOR vector);
	Vector2(const Vector2 &other);
	Vector2(float x, float y) : XMFLOAT2(x, y) {}

	static Vector2 base_x;
	static Vector2 base_y;

	Vector2 &operator=(XMVECTOR vector);
	Vector2 &operator=(const Vector2 &other);

	Vector2 &operator+=(float value);
	Vector2 &operator-=(float value);
	Vector2 &operator*=(float value);
	Vector2 &operator/=(float value);

	Vector2 &operator+=(const Vector2 &other);
	Vector2 &operator-=(const Vector2 &other);
	Vector2 &operator*=(const Vector2 &other);
	Vector2 &operator/=(const Vector2 &other);

	operator float *();
	operator XMVECTOR();
};

inline float get_length(Vector2 *vector2);
inline float get_angle(Vector2 *first_vector2, Vector2 *second_vector2);
inline float get_distance(Vector2 *first_vector2, Vector2 *second_vector2);
inline Vector2 negate(Vector2 *vector);
inline Vector2 normalize(Vector2 *vector);
inline Vector2 dot(Vector2 *first_vector2, Vector2 *second_vector2);
inline Vector2 cross(Vector2 *first_vector2, Vector2 *second_vector2);

inline Vector2 operator+(const Vector2 &first_vector, const Vector2 &second_vector);
inline Vector2 operator-(const Vector2 &first_vector, const Vector2 &second_vector);
inline Vector2 operator*(const Vector2 &first_vector, const Vector2 &second_vector);
inline Vector2 operator/(const Vector2 &first_vector, const Vector2 &second_vector);

struct Vector3 : XMFLOAT3 {
	Vector3() {}
	Vector3(XMVECTOR vector);
	Vector3(const Vector3 &other);
	Vector3(float x, float y, float z) : XMFLOAT3(x, y, z) {}

	static Vector3 base_x;
	static Vector3 base_y;
	static Vector3 base_z;

	Vector3 &operator=(XMVECTOR vector);
	Vector3 &operator=(const Vector3 &other);

	Vector3 &operator+=(float value);
	Vector3 &operator-=(float value);
	Vector3 &operator*=(float value);
	Vector3 &operator/=(float value);

	Vector3 &operator+=(const Vector3 &other);
	Vector3 &operator-=(const Vector3 &other);
	Vector3 &operator*=(const Vector3 &other);
	Vector3 &operator/=(const Vector3 &other);

	operator float *();
	operator XMVECTOR();
};

inline float get_length(Vector3 *vector3);
inline float get_angle(Vector3 *first_vector3, Vector3 *second_vector3);
inline float get_distance(Vector3 *first_vector3, Vector3 *second_vector3);
inline Vector3 negate(Vector3 *vector);
inline Vector3 normalize(Vector3 *vector);
inline Vector3 dot(Vector3 *first_vector3, Vector3 *second_vector3);
inline Vector3 cross(Vector3 *first_vector3, Vector3 *second_vector3);

inline Vector3 operator+(const Vector3 &first_vector, const Vector3 &second_vector);
inline Vector3 operator-(const Vector3 &first_vector, const Vector3 &second_vector);
inline Vector3 operator*(const Vector3 &first_vector, const Vector3 &second_vector);
inline Vector3 operator/(const Vector3 &first_vector, const Vector3 &second_vector);

struct Vector4 : XMFLOAT4 {
	Vector4() {}
	Vector4(XMVECTOR vector);
	Vector4(const Vector4 &other);
	Vector4(float x, float y, float z, float w) : XMFLOAT4(x, y, z, w) {}
	Vector4(const Vector3 &vector, float w) : XMFLOAT4(vector.x, vector.y, vector.z, w) {}

	Vector4 &operator=(XMVECTOR vector);
	Vector4 &operator=(const Vector3 &vector);
	Vector4 &operator=(const Vector4 &other);

	Vector4 &operator+=(float value);
	Vector4 &operator-=(float value);
	Vector4 &operator*=(float value);
	Vector4 &operator/=(float value);

	Vector4 &operator+=(const Vector4 &other);
	Vector4 &operator-=(const Vector4 &other);
	Vector4 &operator*=(const Vector4 &other);
	Vector4 &operator/=(const Vector4 &other);

	operator float *();
	operator XMVECTOR();
};

inline float get_length(Vector4 *vector4);
inline float get_angle(Vector4 *first_vector4, Vector4 *second_vector4);
inline float get_distance(Vector4 *first_vector4, Vector4 *second_vector4);
inline Vector4 negate(Vector4 *vector);
inline Vector4 normalize(Vector4 *vector);
inline Vector4 dot(Vector4 *first_vector4, Vector4 *second_vector4);
inline Vector4 cross(Vector4 *first_vector4, Vector4 *second_vector4, Vector4 *third_vector4);

inline Vector4 operator+(const Vector4 &first_vector, const Vector4 &second_vector);
inline Vector4 operator-(const Vector4 &first_vector, const Vector4 &second_vector);
inline Vector4 operator*(const Vector4 &first_vector, const Vector4 &second_vector);
inline Vector4 operator/(const Vector4 &first_vector, const Vector4 &second_vector);

///////////////////////////////
//          Vector2          //
///////////////////////////////

inline Vector2::Vector2(XMVECTOR vector)
{
	XMStoreFloat2(this, vector);
}

inline Vector2::Vector2(const Vector2 &other)
{
	x = other.x;
	y = other.y;
}

inline Vector2 &Vector2::operator=(XMVECTOR vector)
{
	XMStoreFloat2(this, vector);
	return *this;
}

inline Vector2 &Vector2::operator=(const Vector2 &other)
{
	if (this != &other) {
		x = other.x;
		y = other.y;
	}
	return *this;
}

inline Vector2 &Vector2::operator+=(float value)
{
	Vector2 temp = Vector2(value, value);
	XMVECTOR first = XMLoadFloat2(this);
	XMVECTOR second = XMLoadFloat2(&temp);
	*this = XMVectorAdd(first, second);
	return *this;
}

inline Vector2 &Vector2::operator-=(float value)
{
	Vector2 temp = Vector2(value, value);
	XMVECTOR first = XMLoadFloat2(this);
	XMVECTOR second = XMLoadFloat2(&temp);
	*this = XMVectorSubtract(first, second);
	return *this;
}

inline Vector2 &Vector2::operator*=(float value)
{
	Vector2 temp = Vector2(value, value);
	XMVECTOR first = XMLoadFloat2(this);
	XMVECTOR second = XMLoadFloat2(&temp);
	*this = XMVectorMultiply(first, second);
	return *this;
}

inline Vector2 &Vector2::operator/=(float value)
{
	Vector2 temp = Vector2(value, value);
	XMVECTOR first = XMLoadFloat2(this);
	XMVECTOR second = XMLoadFloat2(&temp);
	*this = XMVectorDivide(first, second);
	return *this;
}

inline Vector2 &Vector2::operator+=(const Vector2 &other)
{
	XMVECTOR first = XMLoadFloat2(this);
	XMVECTOR second = XMLoadFloat2(&other);
	*this = XMVectorAdd(first, second);
	return *this;
}

inline Vector2 &Vector2::operator-=(const Vector2 &other)
{
	XMVECTOR first = XMLoadFloat2(this);
	XMVECTOR second = XMLoadFloat2(&other);
	*this = XMVectorSubtract(first, second);
	return *this;
}

inline Vector2 &Vector2::operator*=(const Vector2 &other)
{
	XMVECTOR first = XMLoadFloat2(this);
	XMVECTOR second = XMLoadFloat2(&other);
	*this = XMVectorMultiply(first, second);
	return *this;
}

inline Vector2 &Vector2::operator/=(const Vector2 &other)
{
	XMVECTOR first = XMLoadFloat2(this);
	XMVECTOR second = XMLoadFloat2(&other);
	*this = XMVectorDivide(first, second);
	return *this;
}

inline Vector2::operator float*()
{
	return &x;
}

inline Vector2::operator XMVECTOR()
{
	return XMVectorSet(x, y, 0.0f, 0.0f);
}

inline float get_length(Vector2 *vector2)
{
	XMVECTOR vector = XMLoadFloat2(vector2);
	return XMVectorGetX(XMVector2Length(vector));
}

inline float get_angle(Vector2 *first_vector2, Vector2 *second_vector2)
{
	XMVECTOR first = XMLoadFloat2(first_vector2);
	XMVECTOR second = XMLoadFloat2(second_vector2);
	return XMVectorGetX(XMVector2AngleBetweenVectors(first, second));
}
inline float get_distance(Vector2 *first_vector2, Vector2 *second_vector2)
{
	XMVECTOR first = XMLoadFloat2(first_vector2);
	XMVECTOR second = XMLoadFloat2(second_vector2);
	XMVECTOR temp = XMVectorSubtract(first, second);
	return XMVectorGetX(XMVector2Length(temp));
}

inline Vector2 negate(Vector2 *vector2)
{
	XMVECTOR vector = XMLoadFloat2(vector2);
	return XMVectorNegate(vector);
}

inline Vector2 normalize(Vector2 *vector2)
{
	XMVECTOR vector = XMLoadFloat2(vector2);
	return XMVector2Normalize(vector);
}

inline Vector2 dot(Vector2 *first_vector2, Vector2 *second_vector2)
{
	XMVECTOR first = XMLoadFloat2(first_vector2);
	XMVECTOR second = XMLoadFloat2(second_vector2);
	return XMVector2Dot(first, second);
}
inline Vector2 cross(Vector2 *first_vector2, Vector2 *second_vector2)
{
	XMVECTOR first = XMLoadFloat2(first_vector2);
	XMVECTOR second = XMLoadFloat2(second_vector2);
	return XMVector2Cross(first, second);
}

inline Vector2 operator+(const Vector2 &first_vector, const Vector2 &second_vector)
{
	XMVECTOR first = XMLoadFloat2(&first_vector);
	XMVECTOR second = XMLoadFloat2(&second_vector);
	return XMVectorAdd(first, second);
}

inline Vector2 operator-(const Vector2 &first_vector, const Vector2 &second_vector)
{
	XMVECTOR first = XMLoadFloat2(&first_vector);
	XMVECTOR second = XMLoadFloat2(&second_vector);
	return XMVectorSubtract(first, second);
}

inline Vector2 operator*(const Vector2 &first_vector, const Vector2 &second_vector)
{
	XMVECTOR first = XMLoadFloat2(&first_vector);
	XMVECTOR second = XMLoadFloat2(&second_vector);
	return XMVectorMultiply(first, second);
}

inline Vector2 operator/(const Vector2 &first_vector, const Vector2 &second_vector)
{
	XMVECTOR first = XMLoadFloat2(&first_vector);
	XMVECTOR second = XMLoadFloat2(&second_vector);
	return XMVectorDivide(first, second);
}

///////////////////////////////
//          Vector3          //
///////////////////////////////

inline Vector3::Vector3(XMVECTOR vector)
{
	XMStoreFloat3(this, vector);
}

inline Vector3::Vector3(const Vector3 &other)
{
	x = other.x;
	y = other.y;
	z = other.z;
}

inline Vector3 &Vector3::operator=(XMVECTOR vector)
{
	XMStoreFloat3(this, vector);
	return *this;
}

inline Vector3 &Vector3::operator=(const Vector3 &other)
{
	if (this != &other) {
		x = other.x;
		y = other.y;
		z = other.z;
	}
	return *this;
}

inline Vector3 &Vector3::operator+=(float value)
{
	Vector3 temp = Vector3(value, value, value);
	XMVECTOR first = XMLoadFloat3(this);
	XMVECTOR second = XMLoadFloat3(&temp);
	*this = XMVectorAdd(first, second);
	return *this;
}

inline Vector3 &Vector3::operator-=(float value)
{
	Vector3 temp = Vector3(value, value, value);
	XMVECTOR first = XMLoadFloat3(this);
	XMVECTOR second = XMLoadFloat3(&temp);
	*this = XMVectorSubtract(first, second);
	return *this;
}

inline Vector3 &Vector3::operator*=(float value)
{
	Vector3 temp = Vector3(value, value, value);
	XMVECTOR first = XMLoadFloat3(this);
	XMVECTOR second = XMLoadFloat3(&temp);
	*this = XMVectorMultiply(first, second);
	return *this;
}

inline Vector3 &Vector3::operator/=(float value)
{
	Vector3 temp = Vector3(value, value, value);
	XMVECTOR first = XMLoadFloat3(this);
	XMVECTOR second = XMLoadFloat3(&temp);
	*this = XMVectorDivide(first, second);
	return *this;
}

inline Vector3 &Vector3::operator+=(const Vector3 &other)
{
	XMVECTOR first = XMLoadFloat3(this);
	XMVECTOR second = XMLoadFloat3(&other);
	*this = XMVectorAdd(first, second);
	return *this;
}

inline Vector3 &Vector3::operator-=(const Vector3 &other)
{
	XMVECTOR first = XMLoadFloat3(this);
	XMVECTOR second = XMLoadFloat3(&other);
	*this = XMVectorSubtract(first, second);
	return *this;
}

inline Vector3 &Vector3::operator*=(const Vector3 &other)
{
	XMVECTOR first = XMLoadFloat3(this);
	XMVECTOR second = XMLoadFloat3(&other);
	*this = XMVectorMultiply(first, second);
	return *this;
}

inline Vector3 &Vector3::operator/=(const Vector3 &other)
{
	XMVECTOR first = XMLoadFloat3(this);
	XMVECTOR second = XMLoadFloat3(&other);
	*this = XMVectorDivide(first, second);
	return *this;
}

inline Vector3::operator float*()
{
	return &x;
}

inline Vector3::operator XMVECTOR()
{
	return XMVectorSet(x, y, z, 0.0f);
}

inline float get_length(Vector3 *vector3)
{
	XMVECTOR vector = XMLoadFloat3(vector3);
	return XMVectorGetX(XMVector3Length(vector));
}

inline float get_angle(Vector3 *first_vector3, Vector3 *second_vector3)
{
	XMVECTOR first = XMLoadFloat3(first_vector3);
	XMVECTOR second = XMLoadFloat3(second_vector3);
	return XMVectorGetX(XMVector3AngleBetweenVectors(first, second));
}
inline float get_distance(Vector3 *first_vector3, Vector3 *second_vector3)
{
	XMVECTOR first = XMLoadFloat3(first_vector3);
	XMVECTOR second = XMLoadFloat3(second_vector3);
	XMVECTOR temp = XMVectorSubtract(first, second);
	return XMVectorGetX(XMVector3Length(temp));
}

inline Vector3 negate(Vector3 *vector3)
{
	XMVECTOR vector = XMLoadFloat3(vector3);
	return XMVectorNegate(vector);
}

inline Vector3 normalize(Vector3 *vector3)
{
	XMVECTOR vector = XMLoadFloat3(vector3);
	return XMVector3Normalize(vector);
}

inline Vector3 dot(Vector3 *first_vector3, Vector3 *second_vector3)
{
	XMVECTOR first = XMLoadFloat3(first_vector3);
	XMVECTOR second = XMLoadFloat3(second_vector3);
	return XMVector3Dot(first, second);
}
inline Vector3 cross(Vector3 *first_vector3, Vector3 *second_vector3)
{
	XMVECTOR first = XMLoadFloat3(first_vector3);
	XMVECTOR second = XMLoadFloat3(second_vector3);
	return XMVector3Cross(first, second);
}

inline Vector3 operator+(const Vector3 &first_vector, const Vector3 &second_vector)
{
	XMVECTOR first = XMLoadFloat3(&first_vector);
	XMVECTOR second = XMLoadFloat3(&second_vector);
	return XMVectorAdd(first, second);
}

inline Vector3 operator-(const Vector3 &first_vector, const Vector3 &second_vector)
{
	XMVECTOR first = XMLoadFloat3(&first_vector);
	XMVECTOR second = XMLoadFloat3(&second_vector);
	return XMVectorSubtract(first, second);
}

inline Vector3 operator*(const Vector3 &first_vector, const Vector3 &second_vector)
{
	XMVECTOR first = XMLoadFloat3(&first_vector);
	XMVECTOR second = XMLoadFloat3(&second_vector);
	return XMVectorMultiply(first, second);
}

inline Vector3 operator/(const Vector3 &first_vector, const Vector3 &second_vector)
{
	XMVECTOR first = XMLoadFloat3(&first_vector);
	XMVECTOR second = XMLoadFloat3(&second_vector);
	return XMVectorDivide(first, second);
}

///////////////////////////////
//          Vector4          //
///////////////////////////////

inline Vector4::Vector4(XMVECTOR vector)
{
	XMStoreFloat4(this, vector);
}

inline Vector4::Vector4(const Vector4 &other)
{
	x = other.x;
	y = other.y;
	z = other.z;
	w = other.w;
}

inline Vector4 &Vector4::operator=(XMVECTOR vector)
{
	XMStoreFloat4(this, vector);
	return *this;
}

inline Vector4 &Vector4::operator=(const Vector3 &vector)
{
	x = vector.x;
	y = vector.y;
	z = vector.z;
	w = 0.0f;
	return *this;
}

inline Vector4 &Vector4::operator=(const Vector4 &other)
{
	if (this != &other) {
		x = other.x;
		y = other.y;
		z = other.z;
		w = other.w;
	}
	return *this;
}

inline Vector4 &Vector4::operator+=(float value)
{
	Vector4 temp = Vector4(value, value, value, value);
	XMVECTOR first = XMLoadFloat4(this);
	XMVECTOR second = XMLoadFloat4(&temp);
	*this = XMVectorAdd(first, second);
	return *this;
}

inline Vector4 &Vector4::operator-=(float value)
{
	Vector4 temp = Vector4(value, value, value, value);
	XMVECTOR first = XMLoadFloat4(this);
	XMVECTOR second = XMLoadFloat4(&temp);
	*this = XMVectorSubtract(first, second);
	return *this;
}

inline Vector4 &Vector4::operator*=(float value)
{
	Vector4 temp = Vector4(value, value, value, value);
	XMVECTOR first = XMLoadFloat4(this);
	XMVECTOR second = XMLoadFloat4(&temp);
	*this = XMVectorMultiply(first, second);
	return *this;
}

inline Vector4 &Vector4::operator/=(float value)
{
	Vector4 temp = Vector4(value, value, value, value);
	XMVECTOR first = XMLoadFloat4(this);
	XMVECTOR second = XMLoadFloat4(&temp);
	*this = XMVectorDivide(first, second);
	return *this;
}

inline Vector4 &Vector4::operator+=(const Vector4 &other)
{
	XMVECTOR first = XMLoadFloat4(this);
	XMVECTOR second = XMLoadFloat4(&other);
	*this = XMVectorAdd(first, second);
	return *this;
}

inline Vector4 &Vector4::operator-=(const Vector4 &other)
{
	XMVECTOR first = XMLoadFloat4(this);
	XMVECTOR second = XMLoadFloat4(&other);
	*this = XMVectorSubtract(first, second);
	return *this;
}

inline Vector4 &Vector4::operator*=(const Vector4 &other)
{
	XMVECTOR first = XMLoadFloat4(this);
	XMVECTOR second = XMLoadFloat4(&other);
	*this = XMVectorMultiply(first, second);
	return *this;
}

inline Vector4 &Vector4::operator/=(const Vector4 &other)
{
	XMVECTOR first = XMLoadFloat4(this);
	XMVECTOR second = XMLoadFloat4(&other);
	*this = XMVectorDivide(first, second);
	return *this;
}

inline Vector4::operator float*()
{
	return &x;
}

inline Vector4::operator XMVECTOR()
{
	return XMVectorSet(x, y, z, w);
}

inline float get_length(Vector4 *vector4)
{
	XMVECTOR vector = XMLoadFloat4(vector4);
	return XMVectorGetX(XMVector4Length(vector));
}

inline float get_angle(Vector4 *first_vector4, Vector4 *second_vector4)
{
	XMVECTOR first = XMLoadFloat4(first_vector4);
	XMVECTOR second = XMLoadFloat4(second_vector4);
	return XMVectorGetX(XMVector4AngleBetweenVectors(first, second));
}
inline float get_distance(Vector4 *first_vector4, Vector4 *second_vector4)
{
	XMVECTOR first = XMLoadFloat4(first_vector4);
	XMVECTOR second = XMLoadFloat4(second_vector4);
	XMVECTOR temp = XMVectorSubtract(first, second);
	return XMVectorGetX(XMVector4Length(temp));
}

inline Vector4 negate(Vector4 *vector4)
{
	XMVECTOR vector = XMLoadFloat4(vector4);
	return XMVectorNegate(vector);
}

inline Vector4 normalize(Vector4 *vector4)
{
	XMVECTOR vector = XMLoadFloat4(vector4);
	return XMVector4Normalize(vector);
}

inline Vector4 dot(Vector4 *first_vector4, Vector4 *second_vector4)
{
	XMVECTOR first = XMLoadFloat4(first_vector4);
	XMVECTOR second = XMLoadFloat4(second_vector4);
	return XMVector4Dot(first, second);
}
inline Vector4 cross(Vector4 *first_vector4, Vector4 *second_vector4, Vector4 *third_vector4)
{
	XMVECTOR first = XMLoadFloat4(first_vector4);
	XMVECTOR second = XMLoadFloat4(second_vector4);
	XMVECTOR third = XMLoadFloat4(third_vector4);
	return XMVector4Cross(first, second, third);
}

inline Vector4 operator+(const Vector4 &first_vector, const Vector4 &second_vector)
{
	XMVECTOR first = XMLoadFloat4(&first_vector);
	XMVECTOR second = XMLoadFloat4(&second_vector);
	return XMVectorAdd(first, second);
}

inline Vector4 operator-(const Vector4 &first_vector, const Vector4 &second_vector)
{
	XMVECTOR first = XMLoadFloat4(&first_vector);
	XMVECTOR second = XMLoadFloat4(&second_vector);
	return XMVectorSubtract(first, second);
}

inline Vector4 operator*(const Vector4 &first_vector, const Vector4 &second_vector)
{
	XMVECTOR first = XMLoadFloat4(&first_vector);
	XMVECTOR second = XMLoadFloat4(&second_vector);
	return XMVectorMultiply(first, second);
}

inline Vector4 operator/(const Vector4 &first_vector, const Vector4 &second_vector)
{
	XMVECTOR first = XMLoadFloat4(&first_vector);
	XMVECTOR second = XMLoadFloat4(&second_vector);
	return XMVectorDivide(first, second);
}
#endif