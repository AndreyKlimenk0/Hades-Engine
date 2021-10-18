#ifndef MODEL_H
#define MODEL_H

#include <d3d11.h>
#include <stdlib.h>

#include "mesh.h"
#include "texture.h"
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

struct Render_Mesh {
	Render_Mesh() {}
	~Render_Mesh() {};
	
	Texture *normal_texture = NULL;
	Texture *diffuse_texture = NULL;
	Texture *specular_texture = NULL;

	Matrix4 position;
	Matrix4 orientation;
	Matrix4 scale;
	Matrix4 transform;

	Material material;
	Triangle_Mesh mesh;


	void operator=(const Render_Mesh &other);
};

inline void Render_Mesh::operator=(const Render_Mesh &other)
{
	//static int i = 0;
	//print("Call copy render mesh = {}", i++);

	normal_texture = NULL;
	diffuse_texture = NULL;
	specular_texture = NULL;

	mesh.vertex_buffer = NULL;
	mesh.index_buffer = NULL;
	mesh.vertices = NULL;
	mesh.indices = NULL;
	mesh.vertex_count = 0;
	mesh.index_count = 0;

	//other.normal_texture->Release();
	//other.diffuse_texture->Release();
	//other.specular_texture->Release();

	//position = other.position;
	//orientation = other.orientation;
	//scale = other.scale;

	//material = other.material;

	//if (other.mesh.vertices) {
	//	mesh.copy_vertices(other.mesh.vertices, other.mesh.vertex_count);
	//}

	//if (other.mesh.indices) {
	//	mesh.copy_indices(other.mesh.indices, other.mesh.index_count);
	//}
}



struct Render_Model {
	Render_Model();
	~Render_Model();

	String name;
	Array<Render_Mesh> render_meshes;

	void init_from_file(const char *file_name);
	bool is_single_mesh_model();
	Render_Mesh *get_render_mesh(int index = 0);
	Triangle_Mesh *get_triangle_mesh(int index = 0);
};

inline bool Render_Model::is_single_mesh_model()
{
	return render_meshes.count == 1;
}

inline Triangle_Mesh *Render_Model::get_triangle_mesh(int index)
{
	return &render_meshes[index].mesh;
}

inline Render_Mesh *Render_Model::get_render_mesh(int index)
{
	return &render_meshes[index];
}


inline Material make_default_material()
{
	Material material;
	material.ambient  = Vector4(0.1f, 0.1f, 0.1f, 0.1f);
	material.diffuse  = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	material.specular = Vector4(0.4f, 0.4f, 0.4f, 8.0f);
	return material;
}


void load_texture(String *file_name, ID3D11ShaderResourceView **texture);
void load_model_from_obj_file(const char *file_name, Triangle_Mesh *mesh);

//Render_Model *create_model_for_entity(Entity *entity);
Render_Model *generate_floor_model(float width, float depth, int m, int n);

#endif 
