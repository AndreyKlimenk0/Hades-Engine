#include "vector.h"

Vector3 base_x_vec = Vector3(1.0f, 0.0f, 0.0f);
Vector3 base_y_vec = Vector3(0.0f, 1.0f, 0.0f);
Vector3 base_z_vec = Vector3(0.0f, 0.0f, 1.0f);

Vector2 base_x_vec2 = Vector2(1.0f, 0.0f);
Vector2 base_y_vec2 = Vector2(0.0f, 1.0f);

Vector3::Vector3(const Vector4 & vec)
{
	x = vec.x;
	y = vec.y;
	z = vec.z;
}


Vector3 & Vector3::operator=(const Vector4 & vec4)
{
	x = vec4.x;
	y = vec4.y;
	z = vec4.z;
	return *this;
}