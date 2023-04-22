#ifndef MODEL_H
#define MODEL_H

#include "../libs/ds/array.h"
#include "../libs/str.h"
#include "../libs/math/vector.h"
#include "vertex.h"

template <typename T>
struct Mesh {
	Array<T> vertices;
	Array<u32> indices;
};

typedef Mesh<Vertex_XNUV> Triangle_Mesh;
typedef Mesh<Vector3> Line_Mesh;


struct Extended_Trinagle_Mesh {
	String name;
	Array<Vector3> positions;
	Triangle_Mesh mesh;
};

#endif 
