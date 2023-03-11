#ifndef MODEL_H
#define MODEL_H

#include <d3d11.h>
#include <stdlib.h>

#include "vertex.h"
#include "render_api.h"
#include "../libs/str.h"
#include "../libs/color.h"
#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"
#include "../libs/ds/hash_table.h"

#include "../sys/sys_local.h"


enum Render_Model_Surface {
	RENDER_MODEL_SURFACE_USE_TEXTURE,
	RENDER_MODEL_SURFACE_USE_COLOR,
};

struct Material {
	Vector4 ambient;
	Vector4 diffuse;
	Vector4 specular;
};

template <typename T>
struct Mesh {
	Array<T> vertices;
	Array<u32> indices;
};

typedef Mesh<Vertex_XNUV> Triangle_Mesh;
typedef Mesh<Vector3> Line_Mesh;

//struct Render_Mesh {
//	Render_Mesh() {}
//	~Render_Mesh() {};
//	
//	Texture2D *normal_texture = NULL;
//	Texture2D *diffuse_texture = NULL;
//	Texture2D *specular_texture = NULL;
//
//	Matrix4 position;
//	Matrix4 orientation;
//	Matrix4 scale;
//	Matrix4 transform;
//
//	Material material;
//	Triangle_Mesh mesh;
//
//
//	void operator=(const Render_Mesh &other);
//};

//inline void Render_Mesh::operator=(const Render_Mesh &other)
//{
//	normal_texture = NULL;
//	diffuse_texture = NULL;
//	specular_texture = NULL;
//
//	//mesh.vertex_buffer = NULL;
//	//mesh.index_buffer = NULL;
//	//mesh.vertices = NULL;
//	//mesh.indices = NULL;
//	//mesh.vertex_count = 0;
//	//mesh.index_count = 0;
//}

struct Render_Model {
	Render_Model();
	~Render_Model();

	String name;
	Array<Triangle_Mesh> render_meshes;

	void setup(const char *render_model_name, u32 render_mesh_count = 1);
	void init_from_file(const char *file_name);
	void load_fbx_model(const char *file_name);
	bool is_single_mesh_model();
	Triangle_Mesh *get_render_mesh(int index = 0);
	Triangle_Mesh *get_triangle_mesh(int index = 0);
};

inline bool Render_Model::is_single_mesh_model()
{
	return render_meshes.count == 1;
}

inline Triangle_Mesh *Render_Model::get_triangle_mesh(int index)
{
	//return &render_meshes[index].mesh;
}



inline Material make_default_material()
{
	Material material;
	material.ambient  = Vector4(0.1f, 0.1f, 0.1f, 0.1f);
	material.diffuse  = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	material.specular = Vector4(0.4f, 0.4f, 0.4f, 8.0f);
	return material;
}

struct Render_Model_Manager {
	Render_Model_Manager() {}
	~Render_Model_Manager() {}
	
	Hash_Table<String, Render_Model *> render_models;

	Render_Model *make_render_model(const char *name);
	Render_Model *get_render_model(const char *name);
};

extern Render_Model_Manager model_manager;

#endif 
