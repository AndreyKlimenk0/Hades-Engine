#ifndef MODEL_H
#define MODEL_H

#include <d3d11.h>
#include <stdlib.h>

#include "mesh.h"
#include "../libs/str.h"
#include "../libs/math/vector.h"
//#include "../game/entity.h"


enum Render_Model_Surface {
	RENDER_MODEL_SURFACE_USE_TEXTURE,
	RENDER_MODEL_SURFACE_USE_COLOR,
};

struct Material {
	Vector4 ambient;
	Vector4 diffuse;
	Vector4 specular;
};

struct Render_Model {
	~Render_Model();
	
	String name;
	Material material;
	Triangle_Mesh mesh;

	Vector3 *model_color = NULL;
	
	Render_Model_Surface render_surface_use = RENDER_MODEL_SURFACE_USE_TEXTURE;
	
	ID3D11ShaderResourceView *normal_texture = NULL;
	ID3D11ShaderResourceView *diffuse_texture = NULL;
	ID3D11ShaderResourceView *specular_texture = NULL;

	void init_from_file(const char *file_name);
	void set_model_color(const Vector3 &color);
};

inline void Render_Model::set_model_color(const Vector3 &color)
{
	model_color = new Vector3(color);
	render_surface_use = RENDER_MODEL_SURFACE_USE_COLOR;
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