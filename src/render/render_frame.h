#ifndef RENDER_FRAME_H
#define RENDER_FRAME_H

#include "base.h"
#include <DirectXMath.h>
using namespace DirectX;
const float Pi = 3.1415926535f;

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

inline Matrix4 get_perspective_matrix(const Win32_State *win32)
{
	XMMATRIX m = XMMatrixPerspectiveFovLH(0.25f * Pi, (float)(win32->window_width / win32->window_height), 1.0f, 1000.0f);
	return Matrix4(m);
}

void render_frame(Direct3D *direct3d, Win32_State *win32);

#endif