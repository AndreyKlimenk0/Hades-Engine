#ifndef DIRECTX_RENDER_H
#define DIRECTX_RENDER_H


#include <stdlib.h>
#include <d2d1.h>
#include <d3d11.h>
#include <dwrite.h>

#include "../libs/math/matrix.h"
#include "../libs/color.h"
#include "../libs/ds/hash_table.h"

struct Direct_Character {
	float width;
	float height;
};

struct Direct_Write {
	Direct_Write() : characters(128) {}
	~Direct_Write();

	wchar_t *font_name = NULL;
	IDWriteFactory *write_factory = NULL;
	IDWriteTextFormat *text_format = NULL;
	IDWriteFontFace *font_face = NULL;

	int glyph_height;
	int glyph_width;
	int font_size;
	
	float max_glyph_height = 0.0f;
	float max_glyph_width = 0.0f;
	float avarage_glyph_height = 0.0f;
	
	Color text_color;

	Hash_Table<char, Direct_Character> characters;

	void init(const char *_font_name, int _font_size, const Color &color);
	void init_characters();
	void shutdown();

	D2D1_SIZE_F get_text_size_in_pixels(const char *text);
	int get_text_width(const char *text);
	int get_max_glyph_height(const char *text);
};

inline int Direct_Write::get_text_width(const char *text)
{
	D2D1_SIZE_F size = get_text_size_in_pixels(text);
	return static_cast<int>(size.width);
}

inline int Direct_Write::get_max_glyph_height(const char *text)
{
	int max_glyph_height = 0;
	int str_len = strlen(text);
	
	for (int i = 0; i < str_len; i++) {
		Direct_Character c = characters[text[i]];
		if (c.height > max_glyph_height) {
			max_glyph_height = c.height;
		}
	}

	return max_glyph_height;
}

extern Direct_Write direct_write;


struct Rect {
	int x;
	int y;
	int width;
	int height;
	Rect() : x(0), y(0), width(0), height(0) {}
	Rect(int x, int y) : x(x), y(y), width(0), height(0) {}
	Rect(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) {}
};

struct Direct2D {
	~Direct2D();

	ID2D1Factory *factory = NULL;
	ID2D1RenderTarget *render_target = NULL;
	ID2D1SolidColorBrush *color = NULL;
	bool started_draw = false;
	void init(IDXGISwapChain *swap_chain);
	void shutdown();

	void test_draw();

	void begin_draw();
	void end_draw();

	void fill_rect(Rect *rect, const Color &color);
	void fill_rect(int x, int y, int width, int height, const Color &color);
	void fill_rect(float x, float y, float width, float height, const Color &color);
	void draw_rect(int x, int y, int width, int height, const Color &stroke_color, float stroke_width = 2.0f);
	void draw_rounded_rect(Rect *rect, float radius, const Color &color);
	void draw_rounded_rect(int x, int y, int width, int height, float radius_x, float radius_y, const Color &color);
	void draw_bitmap(int x, int y, int width, int height, ID2D1Bitmap *bitmap, float scale = 1.0f);
	void draw_text(int x, int y, const char *text);
};

inline void Direct2D::fill_rect(Rect *rect, const Color &color)
{
	fill_rect(rect->x, rect->y, rect->width, rect->height, color);
}

inline void Direct2D::draw_rounded_rect(Rect *rect, float radius, const Color &color)
{
	draw_rounded_rect(rect->x, rect->y, rect->width, rect->height, radius, radius, color);
}

inline void Direct2D::begin_draw()
{
	render_target->BeginDraw();
	started_draw = true;
}

inline void Direct2D::end_draw()
{
	if (started_draw) {
		HR(render_target->EndDraw());
		started_draw = false;
	}
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

extern DirectX11 directx11;

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

typedef ID3D11DepthStencilState Stencil_Test;

inline Stencil_Test *make_stecil_test(D3D11_STENCIL_OP stencil_failed, D3D11_STENCIL_OP depth_failed, D3D11_STENCIL_OP pass, D3D11_COMPARISON_FUNC compare_func, u32 write_mask = 0xff, u32 read_mask = 0xff, bool enable_depth_test = true)
{
	ID3D11DepthStencilState *stencil_state = NULL;

	D3D11_DEPTH_STENCIL_DESC depth_stencil_desc;
	ZeroMemory(&depth_stencil_desc, sizeof(D3D11_DEPTH_STENCILOP_DESC));

	if (enable_depth_test) {
		depth_stencil_desc.DepthEnable = true;
		depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;
	} else {
		depth_stencil_desc.DepthEnable = false;
		depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depth_stencil_desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	}

	depth_stencil_desc.StencilEnable = true;
	depth_stencil_desc.StencilReadMask = read_mask;
	depth_stencil_desc.StencilWriteMask = write_mask;

	depth_stencil_desc.FrontFace.StencilFailOp = stencil_failed;
	depth_stencil_desc.FrontFace.StencilDepthFailOp = depth_failed;
	depth_stencil_desc.FrontFace.StencilPassOp = pass;
	depth_stencil_desc.FrontFace.StencilFunc = compare_func;

	depth_stencil_desc.BackFace.StencilFailOp = stencil_failed;
	depth_stencil_desc.BackFace.StencilDepthFailOp = depth_failed;
	depth_stencil_desc.BackFace.StencilPassOp = pass;
	depth_stencil_desc.BackFace.StencilFunc = compare_func;

	HR(directx11.device->CreateDepthStencilState(&depth_stencil_desc, &stencil_state));
	return stencil_state;
}

inline void enable_stencil_test(Stencil_Test *stencil_test, u32 ref_value)
{
	directx11.device_context->OMSetDepthStencilState(stencil_test, ref_value);
}

inline void disable_stencil_test()
{
	directx11.device_context->OMSetDepthStencilState(NULL, 0);
}
#endif