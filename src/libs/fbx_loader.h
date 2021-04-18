#ifndef FBX_LOADER
#define FBX_LOADER

#include <fbxsdk.h>

#include "../render/model.h"

void load_fbx_model(const char *file_name, Model *model);

#endif