#ifndef CAMERA_H
#define CAMERA_H

#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"

#include "../win32/win_local.h"

struct Free_Camera {
	Vector3 position;
	Vector3 target;
	Vector3 up;
	
	int last_mouse_x;
	int last_mouse_y;

	Matrix4 get_view_matrix();
	void init(const Vector3 &_position = Vector3(0.0f, 3.0f, -10.0f), 
		const Vector3 &_target = Vector3(0.0f , 0.0f, 0.0f), const Vector3 & _up = Vector3(0.0f, 1.0f, 0.0f));
	void update(Win32_State *win32);
};
#endif