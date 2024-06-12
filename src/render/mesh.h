#ifndef MESH_H
#define MESH_H

#include "vertices.h"
#include "../libs/str.h"
#include "../libs/structures/array.h"

template <typename T>
struct Mesh {
	String name;
	String normal_texture_name;
	String diffuse_texture_name;
	String specular_texture_name;
	String displacement_texture_name;

	Array<T> vertices;
	Array<u32> indices;
};

typedef Mesh<Vertex_PNTUV> Triangle_Mesh;
typedef Mesh<Vector3> Line_Mesh;

#endif

