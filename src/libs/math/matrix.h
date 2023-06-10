#ifndef MATRIX_H
#define MATRIX_H

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <DirectXMath.h>

#include "vector.h"
#include "../../sys/sys_local.h"

using namespace DirectX;

struct Matrix3 {
	Vector3 matrix[3];
	Matrix3() {};
	Matrix3(const Vector3 &first_row, const Vector3 &second_row, const Vector3 &third_row);

	Vector3 &operator[](int index);
	const Vector3 &operator[](int index) const;

	Matrix3 operator*(float scalar);
	Vector3 operator*(const Vector3 &vec);
	Matrix3 operator*(const Matrix3 &other);
	Matrix3 operator+(const Matrix3 &other);
	Matrix3 operator-(const Matrix3 &other);

	Matrix3 &operator*=(float scalar);
	Matrix3 &operator*=(const Matrix3 &other);
	Matrix3 &operator+=(const Matrix3 &other);
	Matrix3 &operator-=(const Matrix3 &other);

	Matrix3 transpose();
	Matrix3 inverse();

	void zero();
	void indentity();

	bool compare(const Matrix3 &other);
	bool is_indentity();
};

extern Matrix3 mat3_indentity;

void print_mat(Matrix3 &mat);

inline Matrix3::Matrix3(const Vector3 &first_row, const Vector3 &second_row, const Vector3 &thrid_row)
{
	matrix[0].x = first_row.x;  matrix[0].y = first_row.y;  matrix[0].z = first_row.z;
	matrix[1].x = second_row.x; matrix[1].y = second_row.y; matrix[1].z = second_row.z;
	matrix[2].x = thrid_row.x;  matrix[2].y = thrid_row.y;  matrix[2].z = thrid_row.z;
}

inline Vector3 &Matrix3::operator[](int index)
{
	assert(index < 3);
	return matrix[index];
}

inline const Vector3 &Matrix3::operator[](int index) const
{
	assert(index < 3);
	return matrix[index];
}

inline Matrix3 Matrix3::operator*(float scalar)
{
	return Matrix3(
		Vector3(matrix[0].x * scalar, matrix[0].y * scalar, matrix[0].z * scalar),
		Vector3(matrix[1].x * scalar, matrix[1].y * scalar, matrix[1].z * scalar),
		Vector3(matrix[2].x * scalar, matrix[2].y * scalar, matrix[2].z * scalar));
}

inline Vector3 Matrix3::operator*(const Vector3 &vec)
{
	return Vector3(
		vec.x * matrix[0].x + vec.y * matrix[1].x + vec.z * matrix[2].x,
		vec.x * matrix[0].y + vec.y * matrix[1].y + vec.z * matrix[2].y,
		vec.x * matrix[0].z + vec.y * matrix[1].z + vec.z * matrix[2].z);
}

inline Matrix3 Matrix3::operator*(const Matrix3 &other)
{
	Matrix3 mat;
	mat[0].x = matrix[0].x * other[0].x + matrix[0].y * other[1].x + matrix[0].z * other[2].x;
	mat[0].y = matrix[0].x * other[0].y + matrix[0].y * other[1].y + matrix[0].z * other[2].y;
	mat[0].z = matrix[0].x * other[0].z + matrix[0].y * other[1].z + matrix[0].z * other[2].z;

	mat[1].x = matrix[1].x * other[0].x + matrix[1].y * other[1].x + matrix[1].z * other[2].x;
	mat[1].y = matrix[1].x * other[0].y + matrix[1].y * other[1].y + matrix[1].z * other[2].y;
	mat[1].z = matrix[1].x * other[0].z + matrix[1].y * other[1].z + matrix[1].z * other[2].z;

	mat[2].x = matrix[2].x * other[0].x + matrix[2].y * other[1].x + matrix[2].z * other[2].x;
	mat[2].y = matrix[2].x * other[0].y + matrix[2].y * other[1].y + matrix[2].z * other[2].y;
	mat[2].z = matrix[2].x * other[0].z + matrix[2].y * other[1].z + matrix[2].z * other[2].z;

	return mat;
}

inline Matrix3 Matrix3::operator+(const Matrix3 &other)
{
	return Matrix3(
		Vector3(matrix[0].x + other[0].x, matrix[0].y + other[0].y, matrix[0].z + other[0].z),
		Vector3(matrix[1].x + other[1].x, matrix[1].y + other[1].y, matrix[1].z + other[1].z),
		Vector3(matrix[2].x + other[2].x, matrix[2].y + other[2].y, matrix[2].z + other[2].z));
}

inline Matrix3 Matrix3::operator-(const Matrix3 &other)
{
	return Matrix3(
		Vector3(matrix[0].x - other[0].x, matrix[0].y - other[0].y, matrix[0].z - other[0].z),
		Vector3(matrix[1].x - other[1].x, matrix[1].y - other[1].y, matrix[1].z - other[1].z),
		Vector3(matrix[2].x - other[2].x, matrix[2].y - other[2].y, matrix[2].z - other[2].z));
}

inline Matrix3 &Matrix3::operator*=(float scalar)
{

	matrix[0] = Vector3(matrix[0].x * scalar, matrix[0].y * scalar, matrix[0].z * scalar);
	matrix[1] = Vector3(matrix[1].x * scalar, matrix[1].y * scalar, matrix[1].z * scalar);
	matrix[2] = Vector3(matrix[2].x * scalar, matrix[2].y * scalar, matrix[2].z * scalar);
	return *this;
}

inline Matrix3 &Matrix3::operator*=(const Matrix3 &other)
{
	Matrix3 mat;
	mat[0].x = matrix[0].x * other[0].x + matrix[0].y * other[1].x + matrix[0].z * other[2].x;
	mat[0].y = matrix[0].x * other[0].y + matrix[0].y * other[1].y + matrix[0].z * other[2].y;
	mat[0].z = matrix[0].x * other[0].z + matrix[0].y * other[1].z + matrix[0].z * other[2].z;

	mat[1].x = matrix[1].x * other[0].x + matrix[1].y * other[1].x + matrix[1].z * other[2].x;
	mat[1].y = matrix[1].x * other[0].y + matrix[1].y * other[1].y + matrix[1].z * other[2].y;
	mat[1].z = matrix[1].x * other[0].z + matrix[1].y * other[1].z + matrix[1].z * other[2].z;

	mat[2].x = matrix[2].x * other[0].x + matrix[2].y * other[1].x + matrix[2].z * other[2].x;
	mat[2].y = matrix[2].x * other[0].y + matrix[2].y * other[1].y + matrix[2].z * other[2].y;
	mat[2].z = matrix[2].x * other[0].z + matrix[2].y * other[1].z + matrix[2].z * other[2].z;

	matrix[0] = mat[0];
	matrix[1] = mat[1];
	matrix[2] = mat[2];
	return *this;
}

inline Matrix3 &Matrix3::operator+=(const Matrix3 &other)
{
	matrix[0] = Vector3(matrix[0].x + other[0].x, matrix[0].y + other[0].y, matrix[0].z + other[0].z);
	matrix[1] = Vector3(matrix[1].x + other[1].x, matrix[1].y + other[1].y, matrix[1].z + other[1].z);
	matrix[2] = Vector3(matrix[2].x + other[2].x, matrix[2].y + other[2].y, matrix[2].z + other[2].z);
	return *this;
}

inline Matrix3 &Matrix3::operator-=(const Matrix3 &other)
{
	matrix[0] = Vector3(matrix[0].x - other[0].x, matrix[0].y - other[0].y, matrix[0].z - other[0].z);
	matrix[1] = Vector3(matrix[1].x - other[1].x, matrix[1].y - other[1].y, matrix[1].z - other[1].z);
	matrix[2] = Vector3(matrix[2].x - other[2].x, matrix[2].y - other[2].y, matrix[2].z - other[2].z);
	return *this;
}

inline void Matrix3::zero()
{
	memset(&matrix, 0, sizeof(Matrix3));
}

inline void Matrix3::indentity()
{
	*this = mat3_indentity;
}

inline bool Matrix3::is_indentity()
{
	return compare(mat3_indentity);
}

inline bool Matrix3::compare(const Matrix3 &other)
{
	float *first = (float *)&other;
	float *second = (float *)&matrix;

	for (int i = 0; i < 3 * 3; i++) {
		if (first[i] != second[i]) {
			return false;
		}
	}
	return true;
}

inline Matrix3 Matrix3::transpose()
{
	Matrix3 transpose;
	for (int r = 0; r < 3; r++) {
		for (int c = 0; c < 3; c++) {
			transpose[c][r] = matrix[r][c];
		}
	}
	return transpose;
}

inline Matrix3 Matrix3::inverse()
{
	float determinant = matrix[2].dot(matrix[0].cross(matrix[1]));

	// calculate determinants for matrix 3x3, save result in matrix and transpose this matrix;
	Matrix3 adjoin;
	adjoin[0].x = +(matrix[1].y * matrix[2].z - matrix[1].z * matrix[2].y);
	adjoin[1].x = -(matrix[1].x * matrix[2].z - matrix[1].z * matrix[2].x);
	adjoin[2].x = +(matrix[1].x * matrix[2].y - matrix[1].y * matrix[2].x);

	adjoin[0].y = -(matrix[0].y * matrix[2].z - matrix[0].z * matrix[2].y);
	adjoin[1].y = +(matrix[0].x * matrix[2].z - matrix[0].z * matrix[2].x);
	adjoin[2].y = -(matrix[0].x * matrix[2].y - matrix[0].y * matrix[2].x);

	adjoin[0].z = +(matrix[0].y * matrix[1].z - matrix[0].z * matrix[1].y);
	adjoin[1].z = -(matrix[0].x * matrix[1].z - matrix[0].z * matrix[1].x);
	adjoin[2].z = +(matrix[0].x * matrix[1].y - matrix[0].y * matrix[1].x);

	adjoin *= (1.0f / determinant);
	return adjoin;
}

struct Matrix4 {
	Vector4 matrix[4];
	Matrix4() {};
	Matrix4(const XMMATRIX &xmatrix);
	Matrix4(const Vector4 &first_row, const Vector4 &second_row, const Vector4 &third_row, const Vector4 &four_row);

	Vector4 &operator[](int index);
	const Vector4 &operator[](int index) const;

	Matrix4 operator*(float scalar);
	Vector3 operator*(const Vector3 &vec);
	Vector4 operator*(const Vector4 &vec);
	Matrix4 operator*(const Matrix4 &other);
	Matrix4 operator+(const Matrix4 &other);
	Matrix4 operator-(const Matrix4 &other);

	Matrix4 &operator*=(float scalar);
	Matrix4 &operator*=(const Matrix4 &other);
	Matrix4 &operator+=(const Matrix4 &other);
	Matrix4 &operator-=(const Matrix4 &other);

	operator XMMATRIX();
	operator float *();
	
	void zero();
	void indentity();
	void translate(Vector2 *vector);
	void translate(Vector3 *vector);
	void translate(float x = 0.0f, float y = 0.0f, float z = 0.0f);
	void rotate_about_z(float radians);

	bool is_indentity();
	bool compare(const Matrix4 &other);
	
	Matrix4 transpose();
	Matrix4 inverse();
};

extern Matrix4 matrix4_indentity;

void print_mat(Matrix4 &mat);

inline Matrix4::Matrix4(const XMMATRIX &xmatrix)
{
	XMFLOAT4X4 temp;
	XMStoreFloat4x4(&temp, xmatrix);
	matrix[0] = Vector4(temp._11, temp._12, temp._13, temp._14);
	matrix[1] = Vector4(temp._21, temp._22, temp._23, temp._24);
	matrix[2] = Vector4(temp._31, temp._32, temp._33, temp._34);
	matrix[3] = Vector4(temp._41, temp._42, temp._43, temp._44);
}

inline Matrix4::Matrix4(const Vector4 &first_row, const Vector4 &second_row, const Vector4 &thrid_row, const Vector4 &four_row)
{
	matrix[0] = first_row;
	matrix[1] = second_row;
	matrix[2] = thrid_row;
	matrix[3] = four_row;
}

inline Vector4 &Matrix4::operator[](int index)
{
	assert(index < 4);
	return matrix[index];
}

inline const Vector4 &Matrix4::operator[](int index) const
{
	assert(index < 4);
	return matrix[index];
}

inline Matrix4 Matrix4::operator*(float scalar)
{
	return Matrix4(
		Vector4(matrix[0].x * scalar, matrix[0].y * scalar, matrix[0].z * scalar, matrix[0].w * scalar),
		Vector4(matrix[1].x * scalar, matrix[1].y * scalar, matrix[1].z * scalar, matrix[1].w * scalar),
		Vector4(matrix[2].x * scalar, matrix[2].y * scalar, matrix[2].z * scalar, matrix[2].w * scalar),
		Vector4(matrix[3].x * scalar, matrix[3].y * scalar, matrix[3].z * scalar, matrix[3].w * scalar));
}

inline Vector3 Matrix4::operator*(const Vector3 &vec)
{
	return Vector3(
		vec.x * matrix[0].x + vec.y * matrix[1].x + vec.z * matrix[2].x,
		vec.x * matrix[0].y + vec.y * matrix[1].y + vec.z * matrix[2].y,
		vec.x * matrix[0].z + vec.y * matrix[1].z + vec.z * matrix[2].z);
}

inline Vector4 Matrix4::operator*(const Vector4 &vec)
{
	return Vector4(
		vec.x * matrix[0].x + vec.y * matrix[1].x + vec.z * matrix[2].x + vec.w * matrix[3].x,
		vec.x * matrix[0].y + vec.y * matrix[1].y + vec.z * matrix[2].y + vec.w * matrix[3].y,
		vec.x * matrix[0].z + vec.y * matrix[1].z + vec.z * matrix[2].z + vec.w * matrix[3].z,
		vec.x * matrix[0].w + vec.y * matrix[1].w + vec.z * matrix[2].w + vec.w * matrix[3].w);
}

inline Matrix4 Matrix4::operator*(const Matrix4 &other)
{
	int i, j;
	const float *m1Ptr, *m2Ptr;
	float *dstPtr;
	Matrix4 dst;

	m1Ptr = reinterpret_cast<const float *>(this);
	m2Ptr = reinterpret_cast<const float *>(&other);
	dstPtr = reinterpret_cast<float *>(&dst);

	for ( i = 0; i < 4; i++ ) {
		for ( j = 0; j < 4; j++ ) {
			*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 4 + j ] + m1Ptr[1] * m2Ptr[ 1 * 4 + j ]+ m1Ptr[2] * m2Ptr[ 2 * 4 + j ] + m1Ptr[3] * m2Ptr[ 3 * 4 + j ];
			dstPtr++;
		}
		m1Ptr += 4;
	}
	return dst;
}

inline Matrix4& Matrix4::operator*=(const Matrix4 &other)
{
	Matrix4 mat;
	mat[0].x = matrix[0].x * other[0].x + matrix[0].y * other[1].x + matrix[0].z * other[2].x + matrix[0].w * other[3].x;
	mat[0].y = matrix[0].x * other[0].y + matrix[0].y * other[1].y + matrix[0].z * other[2].y + matrix[0].w * other[3].y;
	mat[0].z = matrix[0].x * other[0].z + matrix[0].y * other[1].z + matrix[0].z * other[2].z + matrix[0].w * other[3].z;
	mat[0].w = matrix[0].x * other[0].w + matrix[0].y * other[1].w + matrix[0].z * other[2].w + matrix[0].w * other[3].w;

	mat[1].x = matrix[1].x * other[0].x + matrix[1].y * other[1].x + matrix[1].z * other[2].x + matrix[1].w * other[3].x;
	mat[1].y = matrix[1].x * other[0].y + matrix[1].y * other[1].y + matrix[1].z * other[2].y + matrix[1].w * other[3].y;
	mat[1].z = matrix[1].x * other[0].z + matrix[1].y * other[1].z + matrix[1].z * other[2].z + matrix[1].w * other[3].z;
	mat[1].w = matrix[1].x * other[0].w + matrix[1].y * other[1].w + matrix[1].z * other[2].w + matrix[1].w * other[3].w;

	mat[2].x = matrix[2].x * other[0].x + matrix[2].y * other[1].x + matrix[2].z * other[2].x + matrix[2].w * other[3].x;
	mat[2].y = matrix[2].x * other[0].y + matrix[2].y * other[1].y + matrix[2].z * other[2].y + matrix[2].w * other[3].y;
	mat[2].z = matrix[2].x * other[0].z + matrix[2].y * other[1].z + matrix[2].z * other[2].z + matrix[2].w * other[3].z;
	mat[2].w = matrix[2].x * other[0].w + matrix[2].y * other[1].w + matrix[2].z * other[2].w + matrix[2].w * other[3].w;

	mat[3].x = matrix[3].x * other[0].x + matrix[3].y * other[1].x + matrix[3].z * other[2].x + matrix[3].w * other[3].x;
	mat[3].y = matrix[3].x * other[0].y + matrix[3].y * other[1].y + matrix[3].z * other[2].y + matrix[3].w * other[3].y;
	mat[3].z = matrix[3].x * other[0].z + matrix[3].y * other[1].z + matrix[3].z * other[2].z + matrix[3].w * other[3].z;
	mat[3].w = matrix[3].x * other[0].w + matrix[3].y * other[1].w + matrix[3].z * other[2].w + matrix[3].w * other[3].w;

	matrix[0] = mat[0];
	matrix[1] = mat[1];
	matrix[2] = mat[2];
	matrix[3] = mat[3];

	return *this;
}

inline Matrix4::operator XMMATRIX()
{
	 return XMMatrixSet(
		 matrix[0].x, matrix[0].y, matrix[0].z, matrix[0].w,
		 matrix[1].x, matrix[1].y, matrix[1].z, matrix[1].w,
		 matrix[2].x, matrix[2].y, matrix[2].z, matrix[2].w,
		 matrix[3].x, matrix[3].y, matrix[3].z, matrix[3].w);
}

inline Matrix4::operator float *()
{
	return &matrix[0].x;
}

inline bool Matrix4::is_indentity()
{
	return compare(matrix4_indentity);
}

inline bool Matrix4::compare(const Matrix4 & other)
{
	float *first = (float *)&other;
	float *second = (float *)&matrix;

	for (int i = 0; i < 4 * 5; i++) {
		if (first[i] != second[i]) {
			return false;
		}
	}
	return true;
}

inline void Matrix4::indentity()
{
	*this = matrix4_indentity;
}

inline void Matrix4::translate(Vector2 *vector)
{
	if (!is_indentity()) {
		indentity();
	}

	matrix[3].x = vector->x;
	matrix[3].y = vector->y;
}

inline void Matrix4::translate(Vector3 *vector)
{
	if (!is_indentity()) {
		indentity();
	}

	matrix[3].x = vector->x;
	matrix[3].y = vector->y;
	matrix[3].z = vector->z;
}

inline void Matrix4::translate(float x, float y, float z)
{
	if (!is_indentity()) {
		indentity();
	}

	matrix[3].x = x;
	matrix[3].y = y;
	matrix[3].z = z;
}

inline void Matrix4::rotate_about_z(float radians)
{
	float c = math::cos(radians);
	float s = math::sin(radians);

	matrix[0] = Vector4(c, -s, 0.0f, 0.0f);
	matrix[1] = Vector4(s,  c, 0.0f, 0.0f);
	matrix[2] = Vector4(0.0f, 0.0f, 1.0f, 0.0f);
	matrix[3] = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
}

inline Matrix4 Matrix4::transpose()
{
	Matrix4 transpose;
	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4; c++) {
			transpose[c][r] = matrix[r][c];
		}
	}
	return transpose;
}

inline Matrix4 Matrix4::inverse()
{
	Matrix4 m;
	m[0] = matrix[0];
	m[1] = matrix[1];
	m[2] = matrix[2];
	m[3] = matrix[3];

	float determinant_1 = (m[0][0] * (
		m[1][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) + 
		m[1][2] * (m[2][3] * m[3][1] - m[2][1] * m[3][3]) + 
		m[1][3] * (m[2][1] * m[3][2] - m[2][2] * m[3][1])));
	
	float determinant_2 = (m[0][1] * (
		m[1][0] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) + 
		m[1][2] * (m[2][3] * m[3][0] - m[2][0] * m[2][3]) + 
		m[1][3] * (m[2][0] * m[3][2] - m[2][2] * m[3][0])));
	
	float determinant_3 = (m[0][2] * (
		m[1][0] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) + 
		m[1][1] * (m[2][3] * m[3][0] - m[2][0] * m[2][3]) + 
		m[1][3] * (m[2][0] * m[3][1] - m[2][1] * m[3][0])));
	
	float determinant_4 = (m[0][3] * (
		m[1][0] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]) + 
		m[1][1] * (m[2][2] * m[3][0] - m[2][0] * m[3][2]) + 
		m[1][2] * (m[2][0] * m[3][1] - m[2][1] * m[3][0])));

	float determinant = determinant_1 - determinant_2 + determinant_3 - determinant_4;

	Matrix4 adjoin;
	adjoin[0][0] =  m[1][1] * m[2][2] * m[3][3] + m[1][2] * m[2][3] * m[3][1] + m[1][3] * m[2][1] * m[3][2] - m[1][3] * m[2][2] * m[3][1] - m[1][2] * m[2][1] * m[3][3] - m[1][1] * m[2][3] * m[3][2];
	adjoin[0][1] = -m[0][1] * m[2][2] * m[3][3] - m[0][2] * m[2][3] * m[3][1] - m[0][3] * m[2][1] * m[3][2] + m[0][3] * m[2][2] * m[3][1] + m[0][2] * m[2][1] * m[3][3] + m[0][1] * m[2][3] * m[3][2];
	adjoin[0][2] =  m[0][1] * m[1][2] * m[3][3] + m[0][2] * m[1][3] * m[3][1] + m[0][3] * m[1][1] * m[3][2] - m[0][3] * m[1][2] * m[3][1] - m[0][2] * m[1][1] * m[3][3] - m[0][1] * m[1][3] * m[3][2];
	adjoin[0][3] = -m[0][1] * m[1][2] * m[2][3] - m[1][2] * m[1][3] * m[2][1] - m[0][3] * m[1][1] * m[2][2] + m[0][3] * m[1][2] * m[2][1] + m[1][2] * m[1][1] * m[2][3] + m[0][1] * m[1][3] * m[2][2];

	adjoin[1][0] = -m[1][0] * m[2][2] * m[3][3] - m[1][2] * m[2][3] * m[3][0] - m[1][3] * m[2][0] * m[3][2] + m[1][3] * m[2][2] * m[3][0] + m[1][2] * m[2][0] * m[3][3] + m[1][0] * m[2][3] * m[3][2];
	adjoin[1][1] =  m[0][0] * m[2][2] * m[3][3] + m[0][2] * m[2][3] * m[3][0] + m[0][3] * m[2][0] * m[3][2] - m[0][3] * m[2][2] * m[3][0] - m[0][2] * m[2][0] * m[3][3] - m[0][0] * m[2][3] * m[3][2];
	adjoin[1][2] = -m[0][0] * m[1][2] * m[3][3] - m[0][2] * m[1][3] * m[3][0] - m[0][3] * m[1][0] * m[3][2] + m[0][3] * m[1][2] * m[3][0] + m[0][2] * m[1][0] * m[3][3] + m[0][0] * m[1][3] * m[3][2];
	adjoin[1][3] =  m[0][0] * m[1][2] * m[2][3] + m[0][2] * m[1][3] * m[2][0] + m[0][3] * m[1][0] * m[2][2] - m[0][3] * m[1][2] * m[2][0] - m[0][2] * m[1][0] * m[2][3] - m[0][0] * m[1][3] * m[2][2];

	adjoin[2][0] =  m[1][0] * m[2][1] * m[3][3] + m[1][1] * m[2][3] * m[3][0] + m[1][3] * m[2][0] * m[3][1] - m[1][3] * m[2][1] * m[3][0] - m[1][1] * m[2][0] * m[3][3] - m[1][0] * m[2][3] * m[3][1];
	adjoin[2][1] = -m[0][0] * m[2][1] * m[3][3] - m[0][1] * m[2][3] * m[3][0] - m[0][3] * m[2][0] * m[3][1] + m[0][3] * m[2][1] * m[3][0] + m[0][1] * m[2][0] * m[3][3] + m[0][0] * m[2][3] * m[3][1];
	adjoin[2][2] =  m[0][0] * m[1][1] * m[3][3] + m[0][1] * m[1][3] * m[3][0] + m[0][3] * m[1][0] * m[3][1] - m[0][3] * m[1][1] * m[3][0] - m[0][1] * m[1][0] * m[3][3] - m[0][0] * m[1][3] * m[3][1];
	adjoin[2][3] = -m[0][0] * m[1][1] * m[2][3] - m[0][1] * m[1][3] * m[2][0] - m[0][3] * m[1][0] * m[2][1] + m[0][3] * m[1][1] * m[2][0] + m[0][1] * m[1][0] * m[2][3] + m[0][0] * m[1][3] * m[2][1];

	adjoin[3][0] = -m[1][0] * m[2][1] * m[3][2] - m[1][1] * m[2][2] * m[3][0] - m[1][2] * m[2][0] * m[3][1] + m[1][2] * m[2][1] * m[3][0] + m[1][1] * m[2][0] * m[3][2] + m[1][0] * m[2][2] * m[3][1];
	adjoin[3][1] =  m[0][0] * m[2][1] * m[3][2] + m[0][1] * m[2][2] * m[3][0] + m[0][2] * m[2][0] * m[3][1] - m[0][2] * m[2][1] * m[3][0] - m[0][1] * m[2][0] * m[3][2] - m[0][0] * m[2][2] * m[3][1];
	adjoin[3][2] = -m[0][0] * m[1][1] * m[3][2] - m[0][1] * m[1][2] * m[3][0] - m[0][2] * m[2][0] * m[3][1] + m[0][2] * m[1][1] * m[3][0] + m[0][1] * m[1][0] * m[3][2] + m[0][0] * m[1][2] * m[3][1];
	adjoin[3][3] =  m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] + m[0][2] * m[1][0] * m[2][1] - m[0][2] * m[1][1] * m[2][0] - m[0][1] * m[1][0] * m[2][2] - m[0][0] * m[1][2] * m[2][1];
	
	Matrix4 result = adjoin * (1 / determinant);
	return result;
}

#endif