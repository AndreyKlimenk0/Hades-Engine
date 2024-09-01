#ifndef MESH_H
#define MESH_H

#include "vertices.h"
#include "../libs/str.h"
#include "../libs/structures/array.h"

template <typename T>
struct Mesh {
	Array<T> vertices;
	Array<u32> indices;

	bool empty();
};

struct Triangle_Mesh : Mesh<Vertex_PNTUV> {
	String name;
	String file_name;

	String normal_texture_name;
	String diffuse_texture_name;
	String specular_texture_name;
	String displacement_texture_name;

	const char *get_name();
	String get_pretty_name();
};

typedef Mesh<Vector3> Line_Mesh;
typedef Mesh<Vector3> Vertex_Mesh;

template<typename T>
inline bool Mesh<T>::empty()
{
	return (vertices.is_empty() || indices.is_empty());
}

#endif
