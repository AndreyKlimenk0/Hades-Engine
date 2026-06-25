#ifndef MESH_H
#define MESH_H

#include "vertices.h"
#include "../libs/str.h"
#include "../libs/math/vector.h"
#include "../libs/number_types.h"
#include "../libs/structures/array.h"

template <typename T>
struct Mesh {
	Array<T> vertices;
	Array<u32> indices;

	bool empty();
	u32 vertex_count();
	u32 index_count();
};

template<typename T>
inline bool Mesh<T>::empty()
{
	return (vertices.is_empty() || indices.is_empty());
}

template<typename T>
inline u32 Mesh<T>::vertex_count()
{
	return vertices.count;
}

template<typename T>
inline u32 Mesh<T>::index_count()
{
	return indices.count;
}

typedef Mesh<Vertex_PNTUV> Triangle_Mesh;
typedef Mesh<Vector3> Line_Mesh;
typedef Mesh<Vector3> Vertex_Mesh;

#endif
