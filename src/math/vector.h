#ifndef VECTOR_H
#define VECTOR_H

#include <DirectXMath.h>

using namespace DirectX;

struct Vector2 {
	float x;
	float y;
	
	Vector2() {};
	Vector2(float _x, float _y) : x(_x), y(_y) {}

	Vector2 operator+(float scalar);
	Vector2 operator+(const Vector2 &other);
	Vector2 operator-(float scalar);
	Vector2 operator-(const Vector2 &other);
	Vector2 operator*(float scalar);
	Vector2 operator*(const Vector2 &other);
	Vector2 operator/(float scalar);
	Vector2 operator/(const Vector2 &other);
	
	operator XMVECTOR();

	float length();
	float normalize();

};

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
	return Vector2(x * scalar, x * scalar);
}
inline Vector2 Vector2::operator*(const Vector2 &other) 
{
	return Vector2(x * other.x, y * other.y);
}

inline Vector2 Vector2::operator/(float scalar) 
{
	float inver = 1.0f / scalar;
	return Vector2(x / scalar, y / scalar);
}

inline Vector2 Vector2::operator/(const Vector2 &other) 
{
	return Vector2(x / other.x, y / other.y);
}

Vector2::operator XMVECTOR()
{
	return XMVectorSet(x, y, 0.0f, 0.0f);
}

struct Vector3 {};
struct Vector4 {};

#endif