#ifndef MESH_H
#define MESH_H

#include <d3d11.h>

#include "vertex.h"
#include "../libs/math/vector.h"
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
};

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

inline void generate_grid(int x, int z, Triangle_Mesh *mesh)
{
	if (mesh->vertices) delete[] mesh->vertices;

	mesh->vertex_count = (x * z) * 4;
	mesh->vertices = new Vertex_Color[x * z];
	for (int i = 0; i < z; i++) {
		for (int j = 0; j < x; j++) {
			Vertex_Color vertex;
			vertex.position = Vector3(j, 0.0f, i);
			vertex.color = Vector4(0.0f, 1.0f, 0.4f, 0.0f);
			mesh->vertices[j + (z * i)] = vertex;
		}
	}

	if (mesh->indices) delete[] mesh->indices;

	mesh->index_count = (x * z) * 6;
	mesh->indices = new u32[mesh->index_count];
	int k = 0;
	for (int i = 0; i < z; i++) {
		for (int j = 0; j < x; j++) {
			mesh->indices[k] = j;
			mesh->indices[k + 1] = j + 1;
			mesh->indices[k + 2] = j + 2;
			
			mesh->indices[k + 3] = j;
			mesh->indices[k + 4] = j + 2;
			mesh->indices[k + 5] = j + 3;
			
			k += 6;
		}
	}
}

inline void generate_box(Triangle_Mesh *mesh)
{
	mesh->vertices = new Vertex_Color[8];
	mesh->vertex_count = 8;

	mesh->vertices[0] = Vertex_Color(Vector3(-1.0f, -1.0f, -1.0f), White);
	mesh->vertices[1] = Vertex_Color(Vector3(-1.0f, +1.0f, -1.0f), Black);
	mesh->vertices[2] = Vertex_Color(Vector3(+1.0f, +1.0f, -1.0f), Red);
	mesh->vertices[3] = Vertex_Color(Vector3(+1.0f, -1.0f, -1.0f), Green);
	mesh->vertices[4] = Vertex_Color(Vector3(-1.0f, -1.0f, +1.0f), Blue);
	mesh->vertices[5] = Vertex_Color(Vector3(-1.0f, +1.0f, +1.0f), Yellow);
	mesh->vertices[6] = Vertex_Color(Vector3(+1.0f, +1.0f, +1.0f), Cyan);
	mesh->vertices[7] = Vertex_Color(Vector3(+1.0f, -1.0f, +1.0f), Magenta);

	UINT indices[] = {
		0, 1, 2,
		0, 2, 3,

		4, 6, 5,
		4, 7, 6,

		4, 5, 1,
		4, 1, 0,

		3, 2, 6,
		3, 6, 7,

		1, 5, 6,
		1, 6, 2,

		4, 0, 3,
		4, 3, 7
	};

	mesh->index_count = 36;
	mesh->indices = new u32[36];
	for (int i = 0; i < 36; i++) {
		mesh->indices[i] = indices[i];
	}
}

#endif