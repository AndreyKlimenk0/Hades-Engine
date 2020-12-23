#include "math/vector.h"
#include "geometry_generator.h"
#include "../render/vertex.h"

//void generate_grid(int x, int z, Triangle_Mesh *mesh)
//{
//	// x and z tell how many vertices need to be !!! not square
//
//	float half_x = 0.5f * x;
//	float half_z = 0.5f * z;
//
//	if (mesh->vertices) delete[] mesh->vertices;
//	mesh->vertex_count = (x * z);
//	mesh->vertices = new Vertex_Color[mesh->vertex_count];
//	for (int i = 0; i < z; i++) {
//
//		float p1 = half_z - i;
//		for (int j = 0; j < x; j++) {
//			float p2 = -half_x + j;
//
//			Vertex_Color vertex;
//			vertex.position = Vector3(p2, 0.0f, p1);
//			vertex.color = Red;
//			mesh->vertices[i * z + j] = vertex;
//		}
//	}
//
//	if (mesh->indices) delete[] mesh->indices;
//
//	mesh->index_count = (x * z) * 6;
//	mesh->indices = new u32[mesh->index_count];
//	int k = 0;
//	for (int i = 0; i < z - 1; i++) {
//		for (int j = 0; j < x - 1; j++) {
//			mesh->indices[k] = i * x + j;
//			mesh->indices[k + 1] = i * x + j + 1;
//			mesh->indices[k + 2] = (i + 1) * x + j;
//			mesh->indices[k + 3] = (i + 1) * x + j;
//			mesh->indices[k + 4] = i * x + j + 1;
//			mesh->indices[k + 5] = (i + 1) * x + j + 1;
//
//			k += 6;
//		}
//	}
//}
//
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