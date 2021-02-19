#ifndef BASE_RENDER_H
#define BASE_RENDER_H

#include <d2d1.h>
#include <d3d11.h>
#include <DxErr.h>
#include <DirectXMath.h>

#include "../win32/win_local.h"
#include "../libs/math/matrix.h"

using namespace DirectX;

const float Pi = 3.1415926535f;

extern Vector4 White;
extern Vector4 Black;
extern Vector4 Red;
extern Vector4 Green;
extern Vector4 Blue;
extern Vector4 Yellow;
extern Vector4 Cyan;
extern Vector4 Magenta;
extern Vector4 Silver;
extern Vector4 LightSteelBlue;

struct Direct2D {
	~Direct2D();
	
	ID2D1Factory *factory = NULL;
	ID2D1HwndRenderTarget *render_target = NULL;
	ID2D1SolidColorBrush *gray_brush = NULL;
	ID2D1SolidColorBrush *blue_brush = NULL;

	void init();
};

struct Direct3D {
	ID3D11Device *device = NULL;
	ID3D11DeviceContext *device_context = NULL;
	IDXGISwapChain *swap_chain = NULL;
	
	ID3D11RenderTargetView *render_target_view = NULL;
	ID3D11DepthStencilView *depth_stencil_view = NULL;
	ID3D11Texture2D *depth_stencil_buffer = NULL;
	
	UINT quality_levels;

	Matrix4 perspective_matrix;

	void init(const Win32_State *win32);
	void shutdown();
	void resize(const Win32_State *win32);
};

extern Direct3D direct3d;

inline Matrix4 get_perspective_matrix(int window_width, int window_height, float near_z, float far_z)
{

	XMMATRIX m = XMMatrixPerspectiveFovLH(0.25f * Pi, (float)window_width / (float)window_height, near_z, far_z);
	return Matrix4(m);
}
#endif