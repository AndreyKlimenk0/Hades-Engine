#ifndef CAMERA_H
#define CAMERA_H

#include "../math/vector.h"
#include "../math/matrix.h"

#include "../../win32/win_local.h"
#include "event.h"

struct Free_Camera {
	//int camera_speed = 1.0;


	Vector3 position = Vector3(0.0f, 200.0f, 100.0f);
	Vector3 target = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
	Vector3 forward = Vector3(0.0f, 0.0f, 1.0f);

	
	//void init(float _near_z = 1.0f, float _far_z = 10000.0f);
	void handle_event(Event *event);

	Matrix4 get_view_matrix();
};
#endif