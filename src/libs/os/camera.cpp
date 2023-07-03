#include <DirectXMath.h>

#include "camera.h"
#include "input.h"
#include "../math/vector.h"
#include "../math/matrix.h"
#include "../../sys/sys_local.h"
#include "../../win32/win_local.h"

using namespace DirectX;


Camera::Camera()
{
	position = Vector3(0.0f, 20.0f, -250.0f);
	target = Vector3(0.0f, 0.0f, -1.0f);
	up = Vector3(0.0f, 1.0f, 0.0f);
	forward = Vector3(0.0f, 0.0f, 1.0f);
}

void Camera::handle_event(Event *event)
{
	static int last_mouse_x = 0;
	static int last_mouse_y = 0;

	static s32 sum = 0;
	if (event->type == EVENT_TYPE_KEY) {
		if (event->key_info.key == VK_LBUTTON) {
			if (!captured && event->key_info.is_pressed) {
				last_mouse_x = Mouse_Input::x;
				last_mouse_y = Mouse_Input::y;
				captured = true;
			}

			if (captured && !event->key_info.is_pressed) {
				captured = false;
			}
		}
	}
	if (Key_Input::is_key_down(Key_W)) {
		Vector3 normalized_target = (target - position);
		normalized_target = normalize(&normalized_target) * 2.0f;
		position += normalized_target;
		target += normalized_target;
	}

	if (Key_Input::is_key_down(Key_S)) {
		Vector3 normalized_target = (target - position);
		normalized_target = normalize(&normalized_target) * 2.0f;
		position -= normalized_target;
		target -= normalized_target;
	}
	
	if ((event->type == EVENT_TYPE_MOUSE) && captured) {
		s32 mouse_x_delta = event->mouse_info.x - last_mouse_x;
		s32 mouse_y_delta = event->mouse_info.y - last_mouse_y;

		float x_angle = degress_to_radians(mouse_x_delta);
		float y_angle = -degress_to_radians(mouse_y_delta);

		Vector3 normalized_target = (target - position);
		normalized_target = normalize(&normalized_target);
		
		rotation_matrix = rotate_about_x(0.5f * y_angle) * rotate_about_y(0.5f * x_angle);
		target = normalized_target * rotation_matrix;
		target += position;

		last_mouse_x = event->mouse_info.x;
		last_mouse_y = event->mouse_info.y;
	}
}

Matrix4 Camera::get_view_matrix()
{
	return make_view_matrix(&position, &target, &up);
}
