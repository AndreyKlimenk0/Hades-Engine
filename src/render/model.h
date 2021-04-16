#ifndef MODEL_H
#define MODEL_H

#include <d3d11.h>
#include <stdlib.h>

#include "mesh.h"
#include "../libs/str.h"

struct Model {
	~Model();
	
	String name;
	Triangle_Mesh mesh;
	
	ID3D11ShaderResourceView *normal_texture = NULL;
	ID3D11ShaderResourceView *diffuse_texture = NULL;
	ID3D11ShaderResourceView *specular_texture = NULL;

	void init_from_file(const char *file_name);
};

void load_model_from_obj_file(const char *file_name, Triangle_Mesh *mesh);
#endif