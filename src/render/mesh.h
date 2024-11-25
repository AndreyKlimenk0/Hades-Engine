#ifndef MESH_H
#define MESH_H

#include "vertices.h"
#include "../libs/str.h"
#include "../libs/math/vector.h"
#include "../libs/number_types.h"
#include "../libs/structures/array.h"

#define DISALLOW_COPY_AND_ASSIGN(Type_Name)	\
    Type_Name(const Type_Name &other) = delete;       \
	Type_Name &operator=(const Type_Name &other) = delete  \


template <typename T>
struct Mesh {
	Array<T> vertices;
	Array<u32> indices;

	bool empty();
};

template<typename T>
inline bool Mesh<T>::empty()
{
	return (vertices.is_empty() || indices.is_empty());
}

typedef Mesh<Vertex_PNTUV> Triangle_Mesh;
typedef Mesh<Vector3> Line_Mesh;
typedef Mesh<Vector3> Vertex_Mesh;

struct Loading_Model {
	Loading_Model();
	Loading_Model(const String &name, const String &file_name);
	~Loading_Model();

	struct Transformation {
		Vector3 scaling = Vector3::one;
		Vector3 rotation = Vector3::zero; // stores angles in radians
		Vector3 translation = Vector3::zero;
	};
	
	String name;
	String file_name;
	
	String normal_texture_name;
	String diffuse_texture_name;
	String specular_texture_name;
	String displacement_texture_name;

	Triangle_Mesh mesh;
	Array<Transformation> instances;

	DISALLOW_COPY_AND_ASSIGN(Loading_Model);

	const char *get_name();
	String get_pretty_name();
};

#endif
