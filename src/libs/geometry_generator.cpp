#include "math/vector.h"
#include "geometry_generator.h"
#include "../render/vertex.h"

void generate_grid(float width, float depth, int m, int n, Triangle_Mesh *mesh)
{
	int vertex_count = m * n;
	int index_count = (m - 1)*(n - 1) * 2;

	float half_width = 0.5f*width;
	float half_depth = 0.5f*depth;

	float dx = width / (n - 1);
	float dz = depth / (m - 1);

	float du = 1.0f / (n - 1);
	float dv = 1.0f / (m - 1);

	mesh->allocate_vertices(vertex_count);
	for (int i = 0; i < m; ++i) {
		float z = half_depth - i * dz;
		for (int j = 0; j < n; ++j) {
			float x = -half_width + j * dx;

			mesh->vertices[i*n + j].position = Vector3(x, 0.0f, z);
			mesh->vertices[i*n + j].normal = Vector3(0.0f, 1.0f, 0.0f);
			mesh->vertices[i*n + j].uv.x = j;
			mesh->vertices[i*n + j].uv.y = i;
		}
	}

	mesh->allocate_indices(index_count * 3);

	int k = 0;
	for (int i = 0; i < m - 1; ++i) {
		for (int j = 0; j < n - 1; ++j) {
			mesh->indices[k] = i * n + j;
			mesh->indices[k + 1] = i * n + j + 1;
			mesh->indices[k + 2] = (i + 1)*n + j;

			mesh->indices[k + 3] = (i + 1)*n + j;
			mesh->indices[k + 4] = i * n + j + 1;
			mesh->indices[k + 5] = (i + 1)*n + j + 1;

			k += 6; // next quad
		}
	}
}

void generate_box(float width, float height, float depth, Triangle_Mesh *mesh)
{
	float w = 0.5f * width;
	float h = 0.5f * height;
	float d = 0.5f * depth;

	mesh->allocate_vertices(24);

	mesh->vertices[0]  = Vertex(Vector3(-w, -h, -d), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f));
	mesh->vertices[1]  = Vertex(Vector3(-w, +h, -d), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f));
	mesh->vertices[2]  = Vertex(Vector3(+w, +h, -d), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f));
	mesh->vertices[3]  = Vertex(Vector3(+w, -h, -d), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f));
	
	mesh->vertices[4]  = Vertex(Vector3(-w, -h, +d), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f));
	mesh->vertices[5]  = Vertex(Vector3(+w, -h, +d), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f));
	mesh->vertices[6]  = Vertex(Vector3(+w, +h, +d), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f));
	mesh->vertices[7]  = Vertex(Vector3(-w, +h, +d), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f));

	mesh->vertices[8]  = Vertex(Vector3(-w, +h, -d), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f));
	mesh->vertices[9]  = Vertex(Vector3(-w, +h, +d), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f));
	mesh->vertices[10] = Vertex(Vector3(+w, +h, +d), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f));
	mesh->vertices[11] = Vertex(Vector3(+w, +h, -d), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f));

	mesh->vertices[12] = Vertex(Vector3(-w, -h, -d), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f));
	mesh->vertices[13] = Vertex(Vector3(+w, -h, -d), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f));
	mesh->vertices[14] = Vertex(Vector3(+w, -h, +d), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f));
	mesh->vertices[15] = Vertex(Vector3(-w, -h, +d), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f));

	mesh->vertices[16] = Vertex(Vector3(-w, -h, +d), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f));
	mesh->vertices[17] = Vertex(Vector3(-w, +h, +d), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f));
	mesh->vertices[18] = Vertex(Vector3(-w, +h, -d), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f));
	mesh->vertices[19] = Vertex(Vector3(-w, -h, -d), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f));
	
	mesh->vertices[20] = Vertex(Vector3(+w, -h, -d), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f));
	mesh->vertices[21] = Vertex(Vector3(+w, +h, -d), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f));
	mesh->vertices[22] = Vertex(Vector3(+w, +h, +d), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f));
	mesh->vertices[23] = Vertex(Vector3(+w, -h, +d), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f));

	mesh->allocate_indices(36);
	u32 *i = mesh->indices;
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