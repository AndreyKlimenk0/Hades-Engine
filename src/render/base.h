#ifndef BASE_RENDER_H
#define BASE_RENDER_H

#include <d3d11.h>

#include "../win32/win_local.h"

#if defined(DEBUG) | defined(_DEBUG)
	#ifndef HR
	#define HR(x) x
	#endif 
#else 
	#ifndef HR
	#define HR(x) x
	#endif
#endif

#define RELEASE_COM(x) { if(x){ x->Release(); x = 0; } }

struct Direct3D_State {
	ID3D11Device *device = NULL;
	ID3D11DeviceContext *device_context = NULL;
	IDXGISwapChain *swap_chain = NULL;
	
	ID3D11RenderTargetView *render_target_view = NULL;
	ID3D11DepthStencilView *depth_stencil_view = NULL;
	ID3D11Texture2D *depth_stencil_buffer = NULL;
	
	UINT quality_levels;
	
	void init(const Win32_State *win32);
	void shutdown();
	void resize(const Win32_State *win32);
};

inline void create_default_buffer(ID3D11Device *device, int vertex_size, void *vertex_data, int index_size, void *index_data, ID3D11Buffer **_vertex_buffer, ID3D11Buffer **_index_buffer)
{
	D3D11_BUFFER_DESC vertex_buffer_desc;
	ZeroMemory(&vertex_buffer_desc, sizeof(D3D11_BUFFER_DESC));
	vertex_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertex_buffer_desc.ByteWidth = vertex_size;

	D3D11_SUBRESOURCE_DATA vertex_resource_data;
	ZeroMemory(&vertex_resource_data, sizeof(D3D11_SUBRESOURCE_DATA));
	vertex_resource_data.pSysMem = vertex_data;

	HR(device->CreateBuffer(&vertex_buffer_desc, &vertex_resource_data, &(*_vertex_buffer)));

	D3D11_BUFFER_DESC index_buffer_desc;
	ZeroMemory(&index_buffer_desc, sizeof(D3D11_BUFFER_DESC));
	index_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
	index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	index_buffer_desc.ByteWidth = sizeof(UINT) * index_size;

	D3D11_SUBRESOURCE_DATA index_resource_data;
	ZeroMemory(&index_resource_data, sizeof(D3D11_SUBRESOURCE_DATA));
	index_resource_data.pSysMem = index_data;

	HR(device->CreateBuffer(&index_buffer_desc, &index_resource_data, &(*_index_buffer)));
}

void render_frame(Direct3D_State *direct3d);

#endif