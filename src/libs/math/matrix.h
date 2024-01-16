#ifndef MATRIX_H
#define MATRIX_H

#include "vector.h"
#include <DirectXMath.h>

using namespace DirectX;

struct Matrix3 : XMFLOAT3X3 {
	Matrix3() : XMFLOAT3X3(0.0f, 0.0f, 0.0f,
						   0.0f, 0.0f, 0.0f,
						   0.0f, 0.0f, 0.0f) {}
	Matrix3(XMMATRIX matrix)
	{
		XMStoreFloat3x3(this, matrix);
	}

	Matrix3(float m00, float m01, float m02,
			float m10, float m11, float m12,
			float m20, float m21, float m22) :
			XMFLOAT3X3(m00, m01, m02,
					   m10, m11, m12,
					   m20, m21, m22) {}

	void set_row_0(const Vector3 &vector);
	void set_row_1(const Vector3 &vector);
	void set_row_2(const Vector3 &vector);
};

struct Matrix4 : XMFLOAT4X4 {
	Matrix4() : XMFLOAT4X4(0.0f, 0.0f, 0.0f, 0.0f,
						   0.0f, 0.0f, 0.0f, 0.0f,
						   0.0f, 0.0f, 0.0f, 0.0f,
						   0.0f, 0.0f, 0.0f, 0.0f) {}
	Matrix4(XMMATRIX matrix)
	{
		XMStoreFloat4x4(this, matrix);
	}

	Matrix4(float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23,
			float m30, float m31, float m32, float m33) : 
			XMFLOAT4X4(m00, m01, m02, m03, 
					   m10, m11, m12, m13, 
					   m20, m21, m22, m23,
					   m30, m31, m32, m33) {}

	void set_row_0(const Vector4 &vector);
	void set_row_1(const Vector4 &vector);
	void set_row_2(const Vector4 &vector);
	void set_row_3(const Vector4 &vector);

	Matrix3 to_matrix3();
};

inline Vector2 transform(Vector2 *vector, Matrix4 *transform_matrix);
inline Vector3 transform(Vector3 *vector, Matrix4 *transform_matrix);
inline Vector4 transform(Vector4 *vector, Matrix4 *transform_matrix);

inline Matrix4 rotate_about_x(float angle);
inline Matrix4 rotate_about_y(float angle);
inline Matrix4 rotate_about_z(float angle);
inline Matrix4 rotate(Vector3 *xyz_angles);
inline Matrix4 rotate(float x_angle, float y_angle, float z_angle);

inline Matrix4 inverse(const Matrix4 &matrix);
inline Matrix4 inverse(Matrix4 *matrix);
inline Matrix4 transpose(Matrix4 *matrix);

inline Matrix3 make_identity_matrix3();
inline Matrix4 make_identity_matrix();
inline Matrix4 make_scale_matrix(float scaling_value);
inline Matrix4 make_scale_matrix(Vector3 *scaling);
inline Matrix4 make_scale_matrix(float scale_x, float scale_y, float scale_z);
inline Matrix4 make_translation_matrix(Vector2 *vector);
inline Matrix4 make_translation_matrix(Vector3 *vector);

inline Matrix4 make_look_at_matrix(const Vector3 &view_position, const Vector3 &view_direction, const Vector3 &up = Vector3(0.0f, 1.0f, 0.0f));
inline Matrix4 make_look_to_matrix(const Vector3 &view_position, const Vector3 &view_direction, const Vector3 &up = Vector3(0.0f, 1.0f, 0.0f));
inline Matrix4 make_perspective_matrix(float fov, float aspect_ratio, float near_plane, float far_plane);
inline Matrix4 make_orthographic_matrix(float width, float height, float near_plane, float far_plane);

inline Matrix4 operator*(const Matrix3 &first_matrix, const Matrix4 second_matrix);
inline Matrix4 operator*(const Matrix4 &first_matrix, const Matrix4 &second_matrix);
inline Vector2 operator*(const Vector2 &vector, const Matrix4 &second_matrix);
inline Vector3 operator*(const Vector3 &vector, const Matrix4 &second_matrix);
inline Vector4 operator*(const Vector4 &vector, const Matrix4 &second_matrix);

inline void Matrix3::set_row_0(const Vector3 &vector)
{
	m[0][0] = vector.x;
	m[0][1] = vector.y;
	m[0][2] = vector.z;
}

inline void Matrix3::set_row_1(const Vector3 &vector)
{
	m[1][0] = vector.x;
	m[1][1] = vector.y;
	m[1][2] = vector.z;
}

inline void Matrix3::set_row_2(const Vector3 &vector)
{
	m[2][0] = vector.x;
	m[2][1] = vector.y;
	m[2][2] = vector.z;
}

inline Matrix3 Matrix4::to_matrix3()
{
	return Matrix3(_11, _12, _13,
			       _21, _22, _23,
				   _31, _32, _33);
}

inline void Matrix4::set_row_0(const Vector4 &vector)
{
	m[0][0] = vector.x;
	m[0][1] = vector.y;
	m[0][2] = vector.z;
	m[0][3] = vector.w;
}

inline void Matrix4::set_row_1(const Vector4 &vector)
{
	m[1][0] = vector.x;
	m[1][1] = vector.y;
	m[1][2] = vector.z;
	m[1][3] = vector.w;
}

inline void Matrix4::set_row_2(const Vector4 &vector)
{
	m[2][0] = vector.x;
	m[2][1] = vector.y;
	m[2][2] = vector.z;
	m[2][3] = vector.w;
}

inline void Matrix4::set_row_3(const Vector4 &vector)
{
	m[3][0] = vector.x;
	m[3][1] = vector.y;
	m[3][2] = vector.z;
	m[3][3] = vector.w;
}

inline Matrix4 rotate_about_x(float angle)
{
	return XMMatrixRotationX(angle);
}

inline Matrix4 rotate_about_y(float angle)
{
	return XMMatrixRotationY(angle);
}

inline Matrix4 rotate_about_z(float angle)
{
	return XMMatrixRotationZ(angle);
}

inline Matrix4 rotate(Vector3 *xyz_angles)
{
	return rotate(xyz_angles->x, xyz_angles->y, xyz_angles->z);
}

inline Matrix4 rotate(float x_angle, float y_angle, float z_angle)
{
	return XMMatrixRotationRollPitchYaw(x_angle, y_angle, z_angle);
}

inline Matrix4 inverse(const Matrix4 &matrix)
{
	XMMATRIX temp = XMLoadFloat4x4(&matrix);
	return XMMatrixInverse(NULL, temp);
}

inline Matrix4 inverse(Matrix4 *matrix)
{
	XMMATRIX temp = XMLoadFloat4x4(matrix);
	return XMMatrixInverse(NULL, temp);
}

inline Matrix4 transpose(Matrix4 *matrix)
{
	XMMATRIX temp = XMLoadFloat4x4(matrix);
	return XMMatrixTranspose(temp);
}

inline Matrix3 make_identity_matrix3()
{
	return XMMatrixIdentity();
}

inline Matrix4 make_identity_matrix()
{
	return XMMatrixIdentity();
}

inline Matrix4 make_scale_matrix(float scaling_value)
{
	return make_scale_matrix(scaling_value, scaling_value, scaling_value);
}

inline Matrix4 make_scale_matrix(Vector3 *scale_xyz)
{
	return make_scale_matrix(scale_xyz->x, scale_xyz->y, scale_xyz->z);
}

inline Matrix4 make_scale_matrix(float scale_x, float scale_y, float scale_z)
{
	return XMMatrixScaling(scale_x, scale_y, scale_z);
}

inline Matrix4 make_translation_matrix(Vector2 *vector)
{
	Vector3 temp = Vector3(vector->x, vector->y, 0.0f);
	return make_translation_matrix(&temp);
}

Matrix4 make_translation_matrix(Vector3 *vector)
{
	return XMMatrixTranslation(vector->x, vector->y, vector->z);
}

inline Matrix4 make_look_at_matrix(const Vector3 &view_position, const Vector3 &view_direction, const Vector3 &up)
{
	return XMMatrixLookAtLH(XMLoadFloat3(&view_position), XMLoadFloat3(&view_direction), XMLoadFloat3(&up));
}

inline Matrix4 make_look_to_matrix(const Vector3 &view_position, const Vector3 &view_direction, const Vector3 &up)
{
	return XMMatrixLookToLH(XMLoadFloat3(&view_position), XMLoadFloat3(&view_direction), XMLoadFloat3(&up));
}

inline Matrix4 make_perspective_matrix(float fov, float aspect_ratio, float near_plane, float far_plane)
{
	return XMMatrixPerspectiveFovLH(fov, aspect_ratio, near_plane, far_plane);
}

inline Matrix4 make_orthographic_matrix(float width, float height, float near_plane, float far_plane)
{
	return XMMatrixOrthographicLH(width, height, near_plane, far_plane);
}

inline Matrix4 operator*(const Matrix3 &first_matrix, const Matrix4 second_matrix)
{
	XMMATRIX first = XMLoadFloat3x3(&first_matrix);
	XMMATRIX second = XMLoadFloat4x4(&second_matrix);
	return XMMatrixMultiply(first, second);
}

inline Matrix4 operator*(const Matrix4 &first_matrix, const Matrix4 &second_matrix)
{
	XMMATRIX first = XMLoadFloat4x4(&first_matrix);
	XMMATRIX second = XMLoadFloat4x4(&second_matrix);
	return XMMatrixMultiply(first, second);
}

inline Vector2 operator*(const Vector2 &vector, const Matrix4 &second_matrix)
{
	XMVECTOR v = XMLoadFloat2(&vector);
	XMMATRIX m = XMLoadFloat4x4(&second_matrix);
	return XMVector2Transform(v, m);
}

inline Vector3 operator*(const Vector3 &vector, const Matrix4 &second_matrix)
{
	XMVECTOR v = XMLoadFloat3(&vector);
	XMMATRIX m = XMLoadFloat4x4(&second_matrix);
	return XMVector3Transform(v, m);
}

inline Vector4 operator*(const Vector4 &vector, const Matrix4 &second_matrix)
{
	XMVECTOR v = XMLoadFloat4(&vector);
	XMMATRIX m = XMLoadFloat4x4(&second_matrix);
	return XMVector4Transform(v, m);
}

inline Vector2 transform(Vector2 *vector, Matrix4 *transform_matrix)
{
	XMVECTOR v = XMLoadFloat2(vector);
	XMMATRIX m = XMLoadFloat4x4(transform_matrix);
	return XMVector2Transform(v, m);
}

inline Vector3 transform(Vector3 *vector, Matrix4 *transform_matrix)
{
	Vector4 t = Vector4(*vector, 1.0f);
	XMVECTOR v = XMLoadFloat4(&t);
	XMMATRIX m = XMLoadFloat4x4(transform_matrix);
	return XMVector3Transform(v, m);
}

inline Vector4 transform(Vector4 *vector, Matrix4 *transform_matrix)
{
	XMVECTOR v = XMLoadFloat4(vector);
	XMMATRIX m = XMLoadFloat4x4(transform_matrix);
	return XMVector4Transform(v, m);
}

#endif