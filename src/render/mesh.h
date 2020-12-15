#ifndef MESH_H
#define MESH_H

#include <d3d11.h>

#include "vertex.h"
#include "../libs/math/vector.h"
#include "../win32/win_types.h"
#include "../libs/general.h"
#include "../win32/win_types.h"
#include "../libs/general.h"


#define CLEAR_MESH_DATA(x) if (x->vertices) delete[] x->vertices, x->vertex_count = 0, if (x->indices) delete[] x->indices, x->index_count = 0;

struct Triangle_Mesh {
	ID3D11Buffer *vertex_buffer = NULL;
	ID3D11Buffer *index_buffer = NULL;

	Vertex_Color *vertices = NULL;
	u32 *indices = NULL;
	u32 vertex_count;
	u32 index_count;
	
	~Triangle_Mesh();
	void allocate_vertices(int number);
	void allocate_indices(int number);
};

inline void Triangle_Mesh::allocate_vertices(int number)
{
	assert(vertices == NULL);
	vertices = new Vertex_Color[number];
	vertex_count = number;
}

inline void Triangle_Mesh::allocate_indices(int number)
{
	assert(indices == NULL);
	indices = new u32[number];
	index_count = number;
}

inline void create_default_buffer(ID3D11Device *device, Triangle_Mesh *mesh)
{
	D3D11_BUFFER_DESC vertex_buffer_desc;
	ZeroMemory(&vertex_buffer_desc, sizeof(D3D11_BUFFER_DESC));
	vertex_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertex_buffer_desc.ByteWidth = sizeof(Vertex_Color) * mesh->vertex_count;

	D3D11_SUBRESOURCE_DATA vertex_resource_data;
	ZeroMemory(&vertex_resource_data, sizeof(D3D11_SUBRESOURCE_DATA));
	vertex_resource_data.pSysMem = (void *)mesh->vertices;

	HR(device->CreateBuffer(&vertex_buffer_desc, &vertex_resource_data, &mesh->vertex_buffer));

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