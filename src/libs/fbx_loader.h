#ifndef FBX_LOADER
#define FBX_LOADER

#include <fbxsdk.h>

#include "../render/model.h"

void set_fbx_file_name(const char *file_name);
void load_fbx_model(const char *file_name, Render_Model *model);
void find_and_copy_material(FbxNode *mesh_node, Render_Mesh *sub_model);
void copy_fbx_mesh_to_triangle_mesh(FbxMesh *fbx_mesh, Triangle_Mesh *mesh);
void get_position_rotation_scale_matrix(FbxNode *fbx_node, Render_Mesh *render_mesh);

bool find_fbx_mesh_nodes_in_scene(FbxScene *scene, Array<FbxNode *> *fbx_nodes);
bool get_texture_file_name(FbxNode *mesh_node, const char *texture_type, String *file_name);

FbxScene *load_scene_from_fbx_file(String *file_path);

inline bool get_specular_texture_file_name(FbxNode *mesh_node, String *file_name)
{
	return get_texture_file_name(mesh_node, FbxSurfaceMaterial::sSpecular, file_name);
}

inline bool get_diffuse_texture_file_name(FbxNode *mesh_node, String *file_name)
{
	return get_texture_file_name(mesh_node, FbxSurfaceMaterial::sDiffuse, file_name);
}

inline bool get_normal_texture_file_name(FbxNode *mesh_node, String *file_name)
{
	return get_texture_file_name(mesh_node, FbxSurfaceMaterial::sNormalMap, file_name);
}

#endif