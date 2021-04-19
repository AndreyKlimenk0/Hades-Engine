#ifndef BASE_RENDER_H
#define BASE_RENDER_H

#include <d2d1.h>
#include <d3d11.h>
#include <d2d1_1.h>
#include <d3d11_1.h>

#include "../libs/color.h"
#include "../libs/math/matrix.h"
#include "../libs/math/vector.h"
#include "../win32/win_local.h"

using namespace DirectX;

const float Pi = 3.1415926535f;

struct Direct2D {
	~Direct2D();
	ID2D1Factory *factory = NULL;
	ID2D1RenderTarget *render_target = NULL;
	
	void init();
	void draw();
	void shutdown();
};

struct Direct3D {
	~Direct3D();
	ID3D11Device *device = NULL;
	ID3D11DeviceContext *device_context = NULL;
	IDXGISwapChain *swap_chain = NULL;
	
	ID3D11RenderTargetView *render_target_view = NULL;
	ID3D11DepthStencilView *depth_stencil_view = NULL;
	ID3D11Texture2D *depth_stencil_buffer = NULL;
	
	UINT quality_levels;

	Matrix4 perspective_matrix;

	void init();
	void shutdown();
	void resize();
};

struct DirectX_Render {
	~DirectX_Render();
	Direct3D direct3d;
	Direct2D direct2d;

	void init();
	void shutdown();
	void resize_buffer();
	void test_draw();
	
	void fill_rect(int x, int y, int width, int height, const Color &background_color);
	void draw_rect(int x, int y, int width, int height, const Color &stroke_color, ID2D1StrokeStyle *stroke_style = NULL, float stroke_width = 2.0f);
	void draw_rounded_rect(int x, int y, int width, int height, float radius_x, float radius_y, const Color &background_color);
	void draw_bitmap(const D2D1_RECT_F &rect, ID2D1Bitmap *bitmap, float scale = 1.0f);
	void load_bitmap_from_file(const char *file_path, int dest_width, int dest_height, ID2D1Bitmap **bitmap);
};

extern DirectX_Render directx_render;

inline Matrix4 get_perspective_matrix(int window_width, int window_height, float near_z, float far_z)
{
	XMMATRIX m = XMMatrixPerspectiveFovLH(0.25f * Pi, (float)window_width / (float)window_height, near_z, far_z);
	return Matrix4(m);
}
#endif