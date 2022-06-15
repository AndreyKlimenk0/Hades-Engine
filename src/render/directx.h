#ifndef DIRECTX_RENDER_H
#define DIRECTX_RENDER_H


#include <stdlib.h>
#include <d2d1.h>
#include <d3d11.h>
#include <dwrite.h>

#include "../libs/math/matrix.h"
#include "../libs/color.h"
#include "../libs/ds/hash_table.h"


//struct Rect {
//	int x;
//	int y;
//	int width;
//	int height;
//	Rect() : x(0), y(0), width(0), height(0) {}
//	Rect(int x, int y) : x(x), y(y), width(0), height(0) {}
//	Rect(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) {}
//};

struct Shader;

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
	void resize();

	void begin_draw();
	void end_draw();
};

void set_vertex_shader(Shader *shader);
void set_pixel_shader(Shader *shader);

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
	//HRESULT result = swap_chain->Present(0, 0);
	//if (FAILED(result)) {
	//	HRESULT error = device->GetDeviceRemovedReason();
	//	char *str = get_str_error_message_from_hresult_description(error);
	//	print("Faield:", str);
	//}
}

typedef ID3D11DepthStencilState Stencil_Test;

typedef ID3D11VertexShader      Vertex_Shader;
typedef ID3D11GeometryShader    Geometry_Shader;
typedef ID3D11ComputeShader     Compute_Shader;
typedef ID3D11HullShader        Hull_Shader;
typedef ID3D11DomainShader      Domain_Shader;
typedef ID3D11PixelShader       Pixel_Shader;

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
