#include <string.h>
#include <d3dx11.h>

#include "mesh.h"
#include "model.h"

#include "../sys/sys_local.h"

#include "../libs/str.h"
#include "../libs/os/file.h"
#include "../libs/os/path.h"
#include "../libs/fbx_loader.h"


Render_Model_Manager model_manager;


Render_Model::Render_Model()
{}

Render_Model::~Render_Model()
{}

void Render_Model::init(const char *render_model_name, u32 render_mesh_count)
{
	name = render_model_name;
	render_meshes.set_count(render_mesh_count);
	
	Render_Mesh *render_mesh = NULL;
	For(render_meshes, render_mesh) {
		render_mesh->position.indentity();
		render_mesh->scale.indentity();
		render_mesh->orientation.indentity();
		render_mesh->material = make_default_material();
		//render_mesh->diffuse_texture = &texture_manager.default_texture;
	}
}

void Render_Model::init_from_file(const char *file_name)
{
	assert(file_name != NULL);
	extract_file_name(file_name, name);
	
	String file_extension;
	extract_file_extension(file_name, file_extension);

	if (!strcmp(file_extension, "fbx")) {
		load_fbx_model(file_name);
	} else {
		print("Model::init_from_file: {} is unkown model type, now only supports fbx file type", file_extension);
		return;
	}
}


void Render_Model::load_fbx_model(const char *file_name)
{
	String path_to_model_file;
	build_full_path_to_model_file(file_name, path_to_model_file);
	FbxScene *scene = load_scene_from_fbx_file(&path_to_model_file);

	if (!scene) {
		print("The scene was not found in file {}", file_name);
		return;
	}

	set_fbx_file_name(file_name);

	Array<FbxNode *> fbx_mesh_nodes;
	bool result = find_fbx_mesh_nodes_in_scene(scene, &fbx_mesh_nodes);
	assert(result);

	render_meshes.set_count(fbx_mesh_nodes.count);
	
	int index = 0;
	FbxNode *fbx_mesh_node = NULL;
	For(fbx_mesh_nodes, fbx_mesh_node) {
		Render_Mesh *render_mesh = &render_meshes[index++];

		String normal_texture_name;
		String diffuse_texture_name;
		String specular_texture_name;

		//get_normal_texture_file_name(fbx_mesh_node, &normal_texture_name);
		//get_diffuse_texture_file_name(fbx_mesh_node, &diffuse_texture_name);
		//get_specular_texture_file_name(fbx_mesh_node, &specular_texture_name);

		//render_mesh->diffuse_texture = texture_manager.get_texture(diffuse_texture_name);

		find_and_copy_material(fbx_mesh_node, render_mesh);

		copy_fbx_mesh_to_triangle_mesh(fbx_mesh_node->GetMesh(), &render_mesh->mesh);
		
		get_position_rotation_scale_matrix(fbx_mesh_node, render_mesh);
		
		//render_mesh->mesh.allocate_static_buffer();
	}
}

Render_Model *Render_Model_Manager::make_render_model(const char *name)
{
	Render_Model *render_model = new Render_Model();
	render_model->init(name);
	render_models.set(name, render_model);
	return render_model;
}

Render_Model *Render_Model_Manager::get_render_model(const char *name)
{
	Render_Model *render_model = NULL;
	if (!render_models.get(name , &render_model)) {
		Render_Model *new_render_model = new Render_Model();
		new_render_model->init_from_file(name);

		render_models.set(name, render_model);
		return new_render_model;
	}
	return render_model;
}