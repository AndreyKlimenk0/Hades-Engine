#include <DirectXMath.h>

#include "camera.h"
#include "input.h"
#include "../math/vector.h"
#include "../../sys/sys_local.h"
#include "../../win32/win_local.h"

using namespace DirectX;


void Free_Camera::init(float _near_z, float _far_z)
{
	window_width = win32.window_width;
	window_hegith = win32.window_height;
	near_z = _near_z;
	far_z = _far_z;
}

void Free_Camera::update()
{
	static float camera_speed = 1.0f;
	//static float camera_speed = 0.02f;
	static int last_mouse_x = 0.0f;
	static int last_mouse_y = 0.0f;
	static float pitch = 0.0f;
	static float yam = 0.0f;

	float move_by_z = 0.0f;
	float move_by_x = 0.0f;
	
	if (Key_Input::is_key_down(VK_LBUTTON)) {
		float delta_x = XMConvertToRadians(0.50f*static_cast<float>(Mouse_Input::x - last_mouse_x));
		float delta_y = XMConvertToRadians(0.50f*static_cast<float>(Mouse_Input::y - last_mouse_y));

		yam += delta_x;
		pitch += delta_y;
	}

	last_mouse_x = Mouse_Input::x;
	last_mouse_y = Mouse_Input::y;

	if (Key_Input::is_key_down(Key_W)) {
		move_by_z += camera_speed;
	}

	if (Key_Input::is_key_down(Key_S)) {
		move_by_z -= camera_speed;
	}

	if (Key_Input::is_key_down(Key_A)) {
		move_by_x -= camera_speed;
	}

	if (Key_Input::is_key_down(Key_D)) {
		move_by_x += camera_speed;
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
