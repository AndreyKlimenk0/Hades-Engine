#include <assert.h>

#include "vector.h"
#include "matrix.h"
#include "functions.h"

Vector2 from_raster_to_screen_space(u32 x, u32 y, u32 screen_width, u32 screen_height)
{
	assert(screen_width > 0);
	assert(screen_height > 0);

	float ndc_x = (((float)x / (float)screen_width) * 2.0f) - 1.0f;
	float ndc_y = 1.0f - (((float)y / (float)screen_height) * 2.0f);
	return Vector2(ndc_x, ndc_y);
}

Matrix4 make_rotation_matrix_v2(Vector3 *direction, Vector3 *up_direction)
{
	assert(direction);

	Vector3 z_axis = normalize(direction);
	Vector3 up = { 0.0f, 1.0f, 0.0f };
	if (up_direction) {
		up = *up_direction;
	}
	Vector3 x_axis = normalize(cross(up, z_axis));
	Vector3 y_axis = normalize(cross(z_axis, x_axis));

	Matrix4 rotation_matrix = make_identity_matrix();
	rotation_matrix.set_row_0(Vector4(x_axis, 0.0f));
	rotation_matrix.set_row_1(Vector4(y_axis, 0.0f));
	rotation_matrix.set_row_2(Vector4(z_axis, 0.0f));
	return rotation_matrix;
}
