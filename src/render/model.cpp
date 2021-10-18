#include <string.h>
#include <d3dx11.h>

#include "mesh.h"
#include "model.h"
#include "directx.h"

#include "../sys/sys_local.h"

#include "../libs/str.h"
#include "../libs/os/file.h"
#include "../libs/os/path.h"
#include "../libs/fbx_loader.h"

#include "../libs/ds/array.h"
#include "../libs/geometry_generator.h"


Render_Model::Render_Model()
{
	//meshes = Array<Render_Mesh>(1);
}

Render_Model::~Render_Model()
{
	//DELETE_PTR(model_color);
	//RELEASE_COM(normal_texture);
	//RELEASE_COM(diffuse_texture);
	//RELEASE_COM(specular_texture);
}

void Render_Model::init_from_file(const char *file_name)
{
	assert(file_name != NULL);
	name = extract_name_from_file(&String(file_name));
	
	char *file_extension = extract_file_extension(file_name);
	if (!strcmp(file_extension, "fbx")) {
		load_fbx_model(file_name, this);
	} else {
		print("Model::init_from_file: {} is unkown model type, now only supports fbx file type", file_extension);
		return;
	}

	Render_Mesh *render_mesh = NULL;
	For(render_meshes, render_mesh) {
		render_mesh->mesh.allocate_static_buffer();
	}
}

#include <windows.h>

void load_texture(String *file_name, ID3D11ShaderResourceView **texture)
{
	if (file_name->len == 0)
		return;

	String *full_path_to_texture = os_path.build_full_path_to_texture_file(file_name);
	defer(full_path_to_texture->free());

	HR(D3DX11CreateShaderResourceViewFromFile(directx11.device, (const char *)full_path_to_texture->data, NULL, NULL, texture, NULL));
}

//Render_Model *create_model_for_entity(Entity *entity)
//{
//
//}

Render_Model *generate_floor_model(float width, float depth, int m, int n)
{
	Render_Model *model = new Render_Model();
	Render_Mesh mm;
	model->render_meshes.push(mm);
	model->name = "floor";
	generate_grid(width, depth, m, n, model->get_triangle_mesh());
	load_texture(&String("floor.jpg"), &model->get_render_mesh()->diffuse_texture->shader_resource);

	model->get_render_mesh()->material.ambient = Vector4(0.1f, 0.1f, 0.1f, 0.1f);
	model->get_render_mesh()->material.diffuse = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	model->get_render_mesh()->material.specular = Vector4(0.1f, 0.1f, 0.1f, 8.0f);
	//model->render_meshes.count += 1;
	Triangle_Mesh *mesh = model->get_triangle_mesh();
	mesh->allocate_static_buffer();
	return model;
}

