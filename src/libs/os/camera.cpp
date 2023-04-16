#include <DirectXMath.h>

#include "camera.h"
#include "input.h"
#include "../math/vector.h"
#include "../../sys/sys_local.h"
#include "../../win32/win_local.h"

using namespace DirectX;


Camera::Camera()
{
	position = Vector3(0.0f, 20.0f, 250.0f);
	target = Vector3(0.0f, 10.0f, 1.0f);
	up = Vector3(0.0f, 1.0f, 0.0f);
	forward = Vector3(0.0f, 0.0f, 1.0f);
}

void Camera::handle_event(Event *event)
{
	static int last_mouse_x = 0.0f;
	static int last_mouse_y = 0.0f;

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
		normalized_target.normalize();
		normalized_target *= 2;
		position += normalized_target;
		target += normalized_target;
	}

	if (Key_Input::is_key_down(Key_S)) {
		Vector3 normalized_target = (target - position);
		normalized_target.normalize();
		normalized_target *= 2;
		position -= normalized_target;
	}
	
	if ((event->type == EVENT_TYPE_MOUSE) && captured) {
		s32 mouse_x_delta = event->mouse_info.x - last_mouse_x;
		s32 mouse_y_delta = event->mouse_info.y - last_mouse_y;

		float x_angle = degress_to_radians(mouse_x_delta);
		float y_angle = -degress_to_radians(mouse_y_delta);

		Matrix4 rotate_about_y = XMMatrixRotationY(0.5 * x_angle);
		Matrix4 rotate_about_x = XMMatrixRotationX(0.5 * y_angle);

		Vector3 normalized_target = (target - position);
		normalized_target.normalize();
		
		Vector4 result = rotate_about_x * rotate_about_y * Vector4(normalized_target, 1.0);
		
		result += Vector4(position, 1.0f);
		target = result;

		last_mouse_x = event->mouse_info.x;
		last_mouse_y = event->mouse_info.y;
	}
}

Matrix4 Camera::get_view_matrix()
{
	XMMATRIX view = XMMatrixLookAtLH(position, target, up);
	return Matrix4(view);
}
