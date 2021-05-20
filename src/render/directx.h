#ifndef DIRECTX_RENDER_H
#define DIRECTX_RENDER_H


#include <stdio.h>
#include <d2d1.h>
#include <d3d11.h>

#include "../libs/math/matrix.h"
#include "../libs/color.h"


struct Direct2D {
	~Direct2D();

	ID2D1Factory *factory = NULL;
	ID2D1RenderTarget *render_target = NULL;

	void init(IDXGISwapChain *swap_chain);
	void shutdown();

	void test_draw();
	
	void begin_draw();
	void end_draw();

	void fill_rect(int x, int y, int width, int height, const Color &background_color);
	void draw_rect(int x, int y, int width, int height, const Color &stroke_color, ID2D1StrokeStyle *stroke_style = NULL, float stroke_width = 2.0f);
	void draw_rounded_rect(int x, int y, int width, int height, float radius_x, float radius_y, const Color &background_color);
	void draw_bitmap(const D2D1_RECT_F &rect, ID2D1Bitmap *bitmap, float scale = 1.0f);
};

inline void Direct2D::begin_draw()
{
	render_target->BeginDraw();
}

inline void Direct2D::end_draw()
{
	render_target->EndDraw();
}

extern Direct2D direct2d;

void load_bitmap_from_file(const char *file_path, int dest_width, int dest_height, ID2D1Bitmap **bitmap);

struct DirectX11 {
	~DirectX11();

	ID3D11Device *device = NULL;
	ID3D11DeviceContext *device_context = NULL;
	IDXGISwapChain *swap_chain = NULL;

	ID3D11RenderTargetView *render_target_view = NULL;
	ID3D11DepthStencilView *depth_stencil_view = NULL;
	ID3D11Texture2D *depth_stencil_buffer = NULL;

	UINT quality_levels;

	void init();
	void shutdown();
	void resize(Direct2D *direct2d = NULL);

	void begin_draw();
	void end_draw();
};

inline void DirectX11::begin_draw()
{
	assert(render_target_view);
	assert(depth_stencil_view);

	device_context->ClearRenderTargetView(render_target_view, (float *)&Color::LightSteelBlue);
	device_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

inline void DirectX11::end_draw()
{
	HR(swap_chain->Present(0, 0));
}

extern DirectX11 directx11;

#endif