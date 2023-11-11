#include "math/common.h"
#include "math/vector.h"
#include "geometry_helper.h"
#include "../render/vertex.h"
#include "../libs/ds/array.h"
#include "../win32/win_types.h"


void make_grid_mesh(Grid *grid, Triangle_Mesh *mesh)
{
	u32 vertex_count = grid->rows_count * grid->columns_count;
	u32 index_count = (grid->rows_count - 1) * (grid->columns_count - 1) * 2;

	mesh->vertices.reserve(vertex_count);
	mesh->indices.reserve(index_count * 3);

	float half_width = 0.5f * grid->width;
	float half_depth = 0.5f * grid->depth;

	float dx = grid->width / (grid->columns_count - 1);
	float dz = grid->depth / (grid->rows_count - 1);

	float du = 1.0f / (grid->columns_count - 1);
	float dv = 1.0f / (grid->rows_count - 1);

	for (u32 i = 0; i < grid->rows_count; ++i) {
		float z = half_depth - i * dz;
		for (u32 j = 0; j < grid->columns_count; ++j) {
			float x = -half_width + j * dx;

			mesh->vertices[i * grid->columns_count + j].position = Vector3(x, 0.0f, z);
			mesh->vertices[i * grid->columns_count + j].normal = Vector3(0.0f, 1.0f, 0.0f);
			mesh->vertices[i * grid->columns_count + j].uv.x = (float)j;
			mesh->vertices[i * grid->columns_count + j].uv.y = (float)i;
		}
	}

	u32 k = 0;
	for (u32 i = 0; i < grid->rows_count - 1; ++i) {
		for (u32 j = 0; j < grid->columns_count - 1; ++j) {
			mesh->indices[k] = i * grid->columns_count + j;
			mesh->indices[k + 1] = i * grid->columns_count + j + 1;
			mesh->indices[k + 2] = (i + 1) * grid->columns_count + j;

			mesh->indices[k + 3] = (i + 1) * grid->columns_count + j;
			mesh->indices[k + 4] = i * grid->columns_count + j + 1;
			mesh->indices[k + 5] = (i + 1) * grid->columns_count + j + 1;

			k += 6; // next quad
		}
	}
}

void make_box_mesh(Box *box, Triangle_Mesh *mesh)
{
	float w = 0.5f * box->width;
	float h = 0.5f * box->height;
	float d = 0.5f * box->depth;

	mesh->vertices.reserve(24);

	mesh->vertices[0]  = Vertex_XNUV(Vector3(-w, -h, -d), Vector3(0.0f, 0.0f, -1.0f), Vector2(0.0f, 1.0f));
	mesh->vertices[1]  = Vertex_XNUV(Vector3(-w, +h, -d), Vector3(0.0f, 0.0f, -1.0f), Vector2(0.0f, 0.0f));
	mesh->vertices[2]  = Vertex_XNUV(Vector3(+w, +h, -d), Vector3(0.0f, 0.0f, -1.0f), Vector2(1.0f, 0.0f));
	mesh->vertices[3]  = Vertex_XNUV(Vector3(+w, -h, -d), Vector3(0.0f, 0.0f, -1.0f), Vector2(1.0f, 1.0f));
	
	mesh->vertices[4]  = Vertex_XNUV(Vector3(-w, -h, +d), Vector3(0.0f, 0.0f, 1.0f), Vector2(1.0f, 1.0f));
	mesh->vertices[5]  = Vertex_XNUV(Vector3(+w, -h, +d), Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 1.0f));
	mesh->vertices[6]  = Vertex_XNUV(Vector3(+w, +h, +d), Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 0.0f));
	mesh->vertices[7]  = Vertex_XNUV(Vector3(-w, +h, +d), Vector3(0.0f, 0.0f, 1.0f), Vector2(1.0f, 0.0f));

	mesh->vertices[8]  = Vertex_XNUV(Vector3(-w, +h, -d), Vector3(0.0f, 1.0f, 0.0f), Vector2(0.0f, 1.0f));
	mesh->vertices[9]  = Vertex_XNUV(Vector3(-w, +h, +d), Vector3(0.0f, 1.0f, 0.0f), Vector2(0.0f, 0.0f));
	mesh->vertices[10] = Vertex_XNUV(Vector3(+w, +h, +d), Vector3(0.0f, 1.0f, 0.0f), Vector2(1.0f, 0.0f));
	mesh->vertices[11] = Vertex_XNUV(Vector3(+w, +h, -d), Vector3(0.0f, 1.0f, 0.0f), Vector2(1.0f, 1.0f));

	mesh->vertices[12] = Vertex_XNUV(Vector3(-w, -h, -d), Vector3(0.0f, -1.0f, 0.0f), Vector2(1.0f, 1.0f));
	mesh->vertices[13] = Vertex_XNUV(Vector3(+w, -h, -d), Vector3(0.0f, -1.0f, 0.0f), Vector2(0.0f, 1.0f));
	mesh->vertices[14] = Vertex_XNUV(Vector3(+w, -h, +d), Vector3(0.0f, -1.0f, 0.0f), Vector2(0.0f, 0.0f));
	mesh->vertices[15] = Vertex_XNUV(Vector3(-w, -h, +d), Vector3(0.0f, -1.0f, 0.0f), Vector2(1.0f, 0.0f));

	mesh->vertices[16] = Vertex_XNUV(Vector3(-w, -h, +d), Vector3(-1.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f));
	mesh->vertices[17] = Vertex_XNUV(Vector3(-w, +h, +d), Vector3(-1.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f));
	mesh->vertices[18] = Vertex_XNUV(Vector3(-w, +h, -d), Vector3(-1.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f));
	mesh->vertices[19] = Vertex_XNUV(Vector3(-w, -h, -d), Vector3(-1.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f));
	
	mesh->vertices[20] = Vertex_XNUV(Vector3(+w, -h, -d), Vector3(1.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f));
	mesh->vertices[21] = Vertex_XNUV(Vector3(+w, +h, -d), Vector3(1.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f));
	mesh->vertices[22] = Vertex_XNUV(Vector3(+w, +h, +d), Vector3(1.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f));
	mesh->vertices[23] = Vertex_XNUV(Vector3(+w, -h, +d), Vector3(1.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f));

	mesh->indices.reserve(36);
	u32 *i = mesh->indices.items;
	i[0] = 0;   i[1] = 1;   i[2] = 2;
	i[3] = 0;   i[4] = 2;   i[5] = 3;

	i[6] = 4;   i[7] = 5;   i[8] = 6;
	i[9] = 4;   i[10] = 6;  i[11] = 7;

	i[12] = 8;  i[13] = 9;  i[14] = 10;
	i[15] = 8;  i[16] = 10; i[17] = 11;

	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;
}


void make_sphere_mesh(Sphere *sphere, Triangle_Mesh *mesh)
{
	Vertex_XNUV topVertex(0.0f, +sphere->radius, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, 0.0f);
	Vertex_XNUV bottomVertex(0.0f, -sphere->radius, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);

	mesh->vertices.push(topVertex);

	float phiStep = PI / sphere->stack_count;
	float thetaStep = 2.0f * PI / sphere->slice_count;

	for (UINT i = 1; i <= sphere->stack_count - 1; ++i) {
		float phi = i * phiStep;

		for (UINT j = 0; j <= sphere->slice_count; ++j) {
			float theta = j * thetaStep;

			Vertex_XNUV v;
			v.position.x = sphere->radius * sinf(phi) * cosf(theta);
			v.position.y = sphere->radius * cosf(phi);
			v.position.z = sphere->radius * sinf(phi) * sinf(theta);

			Vector3 n = v.position;
			v.normal = normalize(&n);

			v.uv.x = theta / XM_2PI;
			v.uv.y = phi / XM_PI;

			mesh->vertices.push(v);
		}
	}
	mesh->vertices.push(bottomVertex);

	for (UINT i = 1; i <= sphere->slice_count; ++i) {
		mesh->indices.push(0);
		mesh->indices.push(i + 1);
		mesh->indices.push(i);
	}

	UINT baseIndex = 1;
	UINT ringVertexCount = sphere->slice_count + 1;
	for (UINT i = 0; i < sphere->stack_count - 2; ++i) {
		for (UINT j = 0; j < sphere->slice_count; ++j) {
			mesh->indices.push(baseIndex + i * ringVertexCount + j);
			mesh->indices.push(baseIndex + i * ringVertexCount + j + 1);
			mesh->indices.push(baseIndex + (i + 1)*ringVertexCount + j);

			mesh->indices.push(baseIndex + (i + 1)*ringVertexCount + j);
			mesh->indices.push(baseIndex + i * ringVertexCount + j + 1);
			mesh->indices.push(baseIndex + (i + 1)*ringVertexCount + j + 1);
		}
	}
	UINT southPoleIndex = (UINT)mesh->vertices.count - 1;
	baseIndex = southPoleIndex - ringVertexCount;

	for (UINT i = 0; i < sphere->slice_count; ++i) {
		mesh->indices.push(southPoleIndex);
		mesh->indices.push(baseIndex + i);
		mesh->indices.push(baseIndex + i + 1);
	}
}

void make_AABB_mesh(Vector3 *min, Vector3 *max, Line_Mesh *mesh)
{
	// Go  for clockwise.
	// Back cube side.
	mesh->vertices.push(*min);
	mesh->vertices.push(Vector3(min->x, max->y, min->z));
	mesh->vertices.push(Vector3(max->x, max->y, min->z));
	mesh->vertices.push(Vector3(max->x, min->y, min->z));

	// Front cuve side.
	mesh->vertices.push(Vector3(min->x, min->y, max->z));
	mesh->vertices.push(Vector3(min->x, max->y, max->z));
	mesh->vertices.push(*max);
	mesh->vertices.push(Vector3(max->x, min->y, max->z));

	mesh->indices.push(0);
	mesh->indices.push(1);

	mesh->indices.push(1);
	mesh->indices.push(2);

	mesh->indices.push(2);
	mesh->indices.push(3);

	mesh->indices.push(3);
	mesh->indices.push(0);

	mesh->indices.push(0 + 4);
	mesh->indices.push(1 + 4);

	mesh->indices.push(1 + 4);
	mesh->indices.push(2 + 4);

	mesh->indices.push(2 + 4);
	mesh->indices.push(3 + 4);

	mesh->indices.push(3 + 4);
	mesh->indices.push(0 + 4);

	mesh->indices.push(0);
	mesh->indices.push(0 + 4);

	mesh->indices.push(1);
	mesh->indices.push(1 + 4);

	mesh->indices.push(2);
	mesh->indices.push(2 + 4);

	mesh->indices.push(3);
	mesh->indices.push(3 + 4);
}

void make_frustum_mesh(float fov, float aspect_ratio, float near_plane, float far_plane, Line_Mesh *mesh)
{
	// Near plane
	//@Note: Hard code
	//float w = 32.5f;
	//float h = 18.3f;
	float w = 0.0f;
	float h = 0.0f;
	mesh->vertices.push(Vector3(w, h, 0.0f));
	mesh->vertices.push(Vector3(w, -h, 0.0f));
	mesh->vertices.push(Vector3(-w, -h, 0.0f));
	mesh->vertices.push(Vector3(-w, h, 0.0f));

	float frustum_depth = far_plane - near_plane;
	float half_height = frustum_depth * math::tan(fov * 0.5f);
	float half_width = half_height * aspect_ratio;
	mesh->vertices.push(Vector3(half_width, half_height, frustum_depth));
	mesh->vertices.push(Vector3(half_width, -half_height, frustum_depth));
	mesh->vertices.push(Vector3(-half_width, -half_height, frustum_depth));
	mesh->vertices.push(Vector3(-half_width, half_height, frustum_depth));

	// Near plane
	mesh->indices.push(0);
	mesh->indices.push(1);
	
	mesh->indices.push(1);
	mesh->indices.push(2);
	
	mesh->indices.push(2);
	mesh->indices.push(3);

	mesh->indices.push(3);
	mesh->indices.push(0);

	// Far plane
	mesh->indices.push(0 + 4);
	mesh->indices.push(1 + 4);
	
	mesh->indices.push(1 + 4);
	mesh->indices.push(2 + 4);
	
	mesh->indices.push(2 + 4);
	mesh->indices.push(3 + 4);

	mesh->indices.push(3 + 4);
	mesh->indices.push(0 + 4);

	// Right, bottom, left, top sides
	mesh->indices.push(0);
	mesh->indices.push(0 + 4);

	mesh->indices.push(1);
	mesh->indices.push(1 + 4);

	mesh->indices.push(2);
	mesh->indices.push(2 + 4);

	mesh->indices.push(3);
	mesh->indices.push(3 + 4);
}
