#ifndef FBX_LOADER
#define FBX_LOADER

#include <fbxsdk.h>

#include "../render/mesh.h"

void load_fbx_model(const char *file_path, Triangle_Mesh *mesh);

#endif