#include <DirectXMath.h>

#include "camera.h"
#include "input.h"
#include "../math/vector.h"
#include "../../sys/sys_local.h"
#include "../../win32/win_local.h"

using namespace DirectX;


void Free_Camera::handle_event(Event *event)
{
	static float camera_speed = 10.0f;
	static int last_mouse_x = 0.0f;
	static int last_mouse_y = 0.0f;
	
	static float pitch = 0.0f;
	static float yam = 0.0f;

	float move_by_z = 0.0f;
	float move_by_x = 0.0f;

	static bool is_left_mouse_button_down = false;

	if (event->is_mouse_event()) {

		if (is_left_mouse_button_down) {
			yam = XMConvertToRadians(static_cast<float>(event->mouse_info.x % 360));
			pitch = XMConvertToRadians(static_cast<float>(event->mouse_info.y % 360));
		}
		
		last_mouse_x = event->mouse_info.x;
		last_mouse_y = event->mouse_info.y;
	}

	if (event->is_key_event()) {
		if (event->is_key_down(VK_LBUTTON)) {
			is_left_mouse_button_down = true;
		}

		if (event->key_info.key == VK_LBUTTON && (!event->key_info.is_pressed)) {
			is_left_mouse_button_down = false;
		}

		if (event->is_key_down(Key_W)) {
			move_by_z += camera_speed;
		}

		if (event->is_key_down(Key_S)) {
			move_by_z -= camera_speed;
		}

		if (event->is_key_down(Key_A)) {
			move_by_x -= camera_speed;
		}

		if (event->is_key_down(Key_D)) {
			move_by_x += camera_speed;
		}
	}

	Matrix4 rotation = XMMatrixRotationRollPitchYaw(pitch, yam, 0);
	target = XMVector3TransformCoord(base_z_vec, rotation);
	target.normalize();

	Matrix4 rotation_about_y = XMMatrixRotationY(yam);	
	Matrix4 rotation_about_x = XMMatrixRotationX(pitch);

	Vector3 right = XMVector3TransformCoord(base_x_vec, rotation_about_y);
    Vector3 incline = XMVector3TransformCoord(base_z_vec, rotation_about_x);
	up = XMVector3TransformCoord(up, rotation_about_y);
	forward = XMVector3TransformCoord(base_z_vec, rotation_about_y);
	

	forward.y += incline.y;
	position += move_by_z * forward;
	position += move_by_x * right;

	target = position + target;
}

Matrix4 Free_Camera::get_view_matrix()
{
	XMMATRIX view = XMMatrixLookAtLH(position, target, up);
	return Matrix4(view);
}
