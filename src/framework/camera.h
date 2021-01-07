#ifndef CAMERA_H
#define CAMERA_H

#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"

#include "../win32/win_local.h"

struct Free_Camera {
	float near_z;
	float far_z;
	int window_width;
	int window_hegith;

	//Vector3 position = Vector3(0.0f, 300.0f, -210.0f);
	Vector3 position = Vector3(0.0f, 0.0f, -2.0f);
	Vector3 target = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
	Vector3 forward = Vector3(0.0f, 0.0f, 1.0f);

	void init(float _near_z = 1.0f, float _far_z = 10000.0f);
	void update();

	Matrix4 get_view_matrix();
	Matrix4 get_view_projection_matrix();
};
#endif