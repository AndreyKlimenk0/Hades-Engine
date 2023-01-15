#ifndef CAMERA_H
#define CAMERA_H

#include "../math/vector.h"
#include "../math/matrix.h"

#include "../../win32/win_local.h"
#include "event.h"

struct Free_Camera {
	Vector3 position = Vector3(0.0f, 00.0f, 40.0f);
	Vector3 target = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
	Vector3 forward = Vector3(0.0f, 0.0f, 1.0f);

	void handle_event(Event *event);

	Matrix4 get_view_matrix();
};
#endif