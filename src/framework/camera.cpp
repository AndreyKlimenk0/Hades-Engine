#include <DirectXMath.h>

#include "camera.h"
#include "input.h"

#include <stdio.h>

using namespace DirectX;

void Free_Camera::init(const Vector3 &_position, const Vector3 &_target, const Vector3 &_up)
{
	position = _position;
	target = _target;
	up = _up;
}

float clamp(float value, float min, float max)
{
	return value < min ? min : (value > max ? max : value);
}

void Free_Camera::update(Win32_State *win32)
{
	const float Pi = 3.1415926535f;
	if (Key_Input::is_key_down(VK_LBUTTON)) {

		//static float mPhi;
		//static float mTheta;
		float mPhi = XMConvertToRadians(Mouse_Input::y % 360);
		float mTheta = XMConvertToRadians(Mouse_Input::x % 360);
		
		//mPhi = clamp(mPhi, 0.1, Pi - 0.1f);
		
		float mRadius = 10.0f;

		float x = mRadius * sinf(mPhi)*cosf(mTheta);
		float z = mRadius * sinf(mPhi)*sinf(mTheta);
		float y = mRadius * cosf(mPhi);

		position.x = x;
		position.y = y;
		position.z = z;

		Mouse_Input::last_x = Mouse_Input::x;
		Mouse_Input::last_y = Mouse_Input::y;
	}
}

Matrix4 Free_Camera::get_view_matrix()
{
	XMMATRIX view = XMMatrixLookAtLH(position, target, up);
	return Matrix4(view);
}