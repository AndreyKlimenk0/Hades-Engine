#include "math/vector.h"
#include "geometry_generator.h"
#include "../render/vertex.h"

void generate_grid(int x, int z, Triangle_Mesh *mesh)
{
	// x and z tell how many vertices need to be !!! not square

	float half_x = 0.5f * x;
	float half_z = 0.5f * z;

	if (mesh->vertices) delete[] mesh->vertices;
	mesh->vertex_count = (x * z);
	mesh->vertices = new Vertex_Color[mesh->vertex_count];
	for (int i = 0; i < z; i++) {

		float p1 = half_z - i;
		for (int j = 0; j < x; j++) {
			float p2 = -half_x + j;

			Vertex_Color vertex;
			vertex.position = Vector3(p2, 0.0f, p1);
			vertex.color = Red;
			mesh->vertices[i * z + j] = vertex;
		}
	}

	if (mesh->indices) delete[] mesh->indices;

	mesh->index_count = (x * z) * 6;
	mesh->indices = new u32[mesh->index_count];
	int k = 0;
	for (int i = 0; i < z - 1; i++) {
		for (int j = 0; j < x - 1; j++) {
			mesh->indices[k] = i * x + j;
			mesh->indices[k + 1] = i * x + j + 1;
			mesh->indices[k + 2] = (i + 1) * x + j;
			mesh->indices[k + 3] = (i + 1) * x + j;
			mesh->indices[k + 4] = i * x + j + 1;
			mesh->indices[k + 5] = (i + 1) * x + j + 1;

			k += 6;
		}
	}
}

void generate_box(Triangle_Mesh *mesh)
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