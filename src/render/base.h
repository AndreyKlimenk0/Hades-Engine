#ifndef BASE_RENDER_H
#define BASE_RENDER_H

#include <d3d11.h>
#include <DxErr.h>
#include <DirectXMath.h>

#include "../win32/win_local.h"
#include "../libs/math/matrix.h"

using namespace DirectX;

const float Pi = 3.1415926535f;

#if defined(DEBUG) | defined(_DEBUG)
	#ifndef HR
	#define HR(x)                                              \
	{                                                          \
		HRESULT hr = (x);                                      \
		if(FAILED(hr))                                         \
		{                                                      \
			DXTrace(__FILE__, (DWORD)__LINE__, hr, #x, true); \
		}                                                      \
	}
	#endif
#else 
	#ifndef HR
	#define HR(x) x
	#endif
#endif

#define RELEASE_COM(x) { if(x){ x->Release(); x = 0; } }

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

inline Matrix4 get_perspective_matrix(const Win32_State *win32)
{

	XMMATRIX m = XMMatrixPerspectiveFovLH(0.25f * Pi, (float)win32->window_width / (float)win32->window_height, 1.0f, 1000.0f);
	return Matrix4(m);
}
#endif