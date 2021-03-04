#ifndef CAMERA_H
#define CAMERA_H

#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"

#include "../win32/win_local.h"

struct Free_Camera {
	//int camera_speed = 1.0;
	int window_width;
	int window_hegith;
	float far_z;
	float near_z;


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