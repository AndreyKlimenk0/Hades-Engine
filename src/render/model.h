#ifndef MODEL_H
#define MODEL_H

#include <d3d11.h>
#include <stdlib.h>

#include "mesh.h"


struct Model {
	char *name = NULL;
	Triangle_Mesh mesh;
	ID3D11ShaderResourceView *texture = NULL;

	~Model();
	void init_from_file(const char *file_name);
};

#endif