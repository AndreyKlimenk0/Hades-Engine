#ifndef FBX_LOADER
#define FBX_LOADER

#include <fbxsdk.h>

#include "../render/mesh.h"

void copy_fbx_mesh_to_triangle_mesh_from_file(const char *file_path, Triangle_Mesh *mesh);
FbxScene *load_scene_from_fbx_file(const char *file_path, Triangle_Mesh *mesh);

#endif