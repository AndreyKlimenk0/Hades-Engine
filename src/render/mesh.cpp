#include "mesh.h"

Triangle_Mesh::Triangle_Mesh() : vertex_buffer(NULL), index_buffer(NULL), vertices(NULL), indices(NULL), vertex_count(0), index_count(0), is_indexed(true) {}

Triangle_Mesh::~Triangle_Mesh()
{
	RELEASE_COM(vertex_buffer);
	RELEASE_COM(index_buffer);

	delete[] vertices;
	delete[] indices;
}