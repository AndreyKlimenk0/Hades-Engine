#ifndef DIRECTX_RENDER_H
#define DIRECTX_RENDER_H



#include <stdio.h>
#include <d2d1.h>
#include <d3d11.h>

#include "mesh.h"
#include "effect.h"
#include "render.h"

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


struct DirectX11 : Render {
	~DirectX11();

	ID3D11Device *device = NULL;
	ID3D11DeviceContext *device_context = NULL;
	IDXGISwapChain *swap_chain = NULL;

	ID3D11RenderTargetView *render_target_view = NULL;
	ID3D11DepthStencilView *depth_stencil_view = NULL;
	ID3D11Texture2D *depth_stencil_buffer = NULL;

	UINT quality_levels;
	
	Direct2D direct2d;

	void init();
	void shutdown();
	void resize();

	void begin_draw();
	void end_draw();

	void draw_mesh(Triangle_Mesh *mesh);
	void draw_indexed_mesh(Triangle_Mesh *mesh);
	void draw_not_indexed_mesh(Triangle_Mesh *mesh);
	void draw_normals(Entity *entity, float line_len);
	void draw_shadow(Entity *entity, Fx_Shader *fx_shader_light, Light *light, Matrix4 &view, Matrix4 &perspective);

	void create_default_buffer(Triangle_Mesh *mesh);
};


inline void DirectX11::begin_draw()
{
	assert(render_target_view);
	assert(depth_stencil_view);

	device_context->ClearRenderTargetView(render_target_view, (float *)&Color::LightSteelBlue);
	device_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	direct2d.begin_draw();
}

inline void DirectX11::end_draw()
{
	direct2d.end_draw();
	HR(swap_chain->Present(0, 0));
}

inline void DirectX11::create_default_buffer(Triangle_Mesh *mesh)
{
	assert(mesh->index_buffer == NULL);
	assert(mesh->vertex_buffer == NULL);

	D3D11_BUFFER_DESC vertex_buffer_desc;
	ZeroMemory(&vertex_buffer_desc, sizeof(D3D11_BUFFER_DESC));
	vertex_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertex_buffer_desc.ByteWidth = sizeof(Vertex) * mesh->vertex_count;

	D3D11_SUBRESOURCE_DATA vertex_resource_data;
	ZeroMemory(&vertex_resource_data, sizeof(D3D11_SUBRESOURCE_DATA));
	vertex_resource_data.pSysMem = (void *)mesh->vertices;

	HR(device->CreateBuffer(&vertex_buffer_desc, &vertex_resource_data, &mesh->vertex_buffer));

	if (!mesh->is_indexed) {
		return;
	}

	D3D11_BUFFER_DESC index_buffer_desc;
	ZeroMemory(&index_buffer_desc, sizeof(D3D11_BUFFER_DESC));
	index_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
	index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	index_buffer_desc.ByteWidth = sizeof(u32) * mesh->index_count;

	D3D11_SUBRESOURCE_DATA index_resource_data;
	ZeroMemory(&index_resource_data, sizeof(D3D11_SUBRESOURCE_DATA));
	index_resource_data.pSysMem = (void *)mesh->indices;

	HR(device->CreateBuffer(&index_buffer_desc, &index_resource_data, &mesh->index_buffer));
}
#endif