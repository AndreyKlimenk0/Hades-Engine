#include "mesh.h"


Triangle_Mesh::~Triangle_Mesh()
{
	RELEASE_COM(vertex_buffer);
	RELEASE_COM(index_buffer);

	delete[] vertices;
	delete[] indices;
}