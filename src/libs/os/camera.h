#ifndef CAMERA_H
#define CAMERA_H

#include "../math/vector.h"
#include "../math/matrix.h"

#include "../../win32/win_local.h"
#include "event.h"

struct Camera {
	Camera();

	bool captured = false;

	Vector3 position;
	Vector3 target;
	Vector3 up;
	Vector3 forward;

	void handle_event(Event *event);

	Matrix4 get_view_matrix();
};
#endif