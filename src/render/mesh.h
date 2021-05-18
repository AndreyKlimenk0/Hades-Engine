#ifndef MESH_H
#define MESH_H

#include <d3d11.h>

#include "base.h"
#include "vertex.h"
#include "../libs/math/vector.h"
#include "../sys/sys_local.h"
#include "../win32/win_types.h"



struct Triangle_Mesh {
	~Triangle_Mesh();

	ID3D11Buffer *vertex_buffer = NULL;
	ID3D11Buffer *index_buffer = NULL;

	Vertex *vertices = NULL;
	u32 *indices = NULL;
	
	u32 vertex_count;
	u32 index_count;

	bool is_indexed = true;

	void allocate_static_buffer();
	void allocate_vertices(int number);
	void allocate_indices(int number);
	void copy_vertices(Vertex *source, int vertex_count);
	void copy_indices(u32 *source, int index_count);
};

inline void Triangle_Mesh::allocate_vertices(int number)
{
	assert(vertices == NULL);
	vertices = new Vertex[number];
	vertex_count = number;
}

inline void Triangle_Mesh::allocate_indices(int number)
{
	assert(indices == NULL);
	indices = new u32[number];
	index_count = number;
}

inline void Triangle_Mesh::copy_vertices(Vertex *source, int vertex_count)
{
	allocate_vertices(vertex_count);
	memcpy(vertices, source, sizeof(Vertex) * vertex_count);
}

inline void Triangle_Mesh::copy_indices(u32 *source, int index_count)
{
	allocate_indices(index_count);
	memcpy(indices, source, sizeof(u32) *index_count);
}

inline void Triangle_Mesh::allocate_static_buffer()
{
	assert(index_buffer == NULL);
	assert(vertex_buffer == NULL);

	D3D11_BUFFER_DESC vertex_buffer_desc;
	ZeroMemory(&vertex_buffer_desc, sizeof(D3D11_BUFFER_DESC));
	vertex_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertex_buffer_desc.ByteWidth = sizeof(Vertex) * vertex_count;

	D3D11_SUBRESOURCE_DATA vertex_resource_data;
	ZeroMemory(&vertex_resource_data, sizeof(D3D11_SUBRESOURCE_DATA));
	vertex_resource_data.pSysMem = (void *)vertices;

	HR(direct3d.device->CreateBuffer(&vertex_buffer_desc, &vertex_resource_data, &vertex_buffer));

	if (!is_indexed) {
		return;
	}

	D3D11_BUFFER_DESC index_buffer_desc;
	ZeroMemory(&index_buffer_desc, sizeof(D3D11_BUFFER_DESC));
	index_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
	index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	index_buffer_desc.ByteWidth = sizeof(u32) * index_count;

	D3D11_SUBRESOURCE_DATA index_resource_data;
	ZeroMemory(&index_resource_data, sizeof(D3D11_SUBRESOURCE_DATA));
	index_resource_data.pSysMem = (void *)indices;

	HR(direct3d.device->CreateBuffer(&index_buffer_desc, &index_resource_data, &index_buffer));
}

inline void create_static_vertex_buffer(int vertex_count, int vertex_size, void *vertices, ID3D11Buffer **vertex_buffer)
{
	assert(vertices);

	D3D11_BUFFER_DESC vertex_buffer_desc;
	ZeroMemory(&vertex_buffer_desc, sizeof(D3D11_BUFFER_DESC));
	vertex_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertex_buffer_desc.ByteWidth = vertex_size * vertex_count;

	D3D11_SUBRESOURCE_DATA vertex_resource_data;
	ZeroMemory(&vertex_resource_data, sizeof(D3D11_SUBRESOURCE_DATA));
	vertex_resource_data.pSysMem = vertices;

	HR(direct3d.device->CreateBuffer(&vertex_buffer_desc, &vertex_resource_data, vertex_buffer));
}

inline void create_default_buffer(Triangle_Mesh *mesh)
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

	HR(direct3d.device->CreateBuffer(&vertex_buffer_desc, &vertex_resource_data, &mesh->vertex_buffer));

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

	HR(direct3d.device->CreateBuffer(&index_buffer_desc, &index_resource_data, &mesh->index_buffer));
}
#endif