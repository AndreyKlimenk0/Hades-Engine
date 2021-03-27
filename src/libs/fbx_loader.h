#ifndef FBX_LOADER
#define FBX_LOADER

#include <fbxsdk.h>

#include "../render/mesh.h"

void loat_fbx_model(const char *file_path, Triangle_Mesh *mesh);
FbxScene *load_scene_from_fbx_file(const char *file_path, Triangle_Mesh *mesh);

#endif