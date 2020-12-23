#include <string.h>
#include <d3dx11.h>

#include "base.h"
#include "model.h"
#include "../framework/file.h"
#include "../libs/general.h"
#include "../libs/fbx_loader.h"
#include "../libs/ds/string.h"


Model::~Model()
{
	DELETE_PTR(name);
	RELEASE_COM(texture);
}

void load_texture(const char *name, ID3D11ShaderResourceView *texture)
{
	char texture_dir[] = "data\\texture\\";
	char *texture_path = build_full_path(concatenate_c_str(texture_dir, name));
	HR(D3DX11CreateShaderResourceViewFromFile(direct3d.device, texture_path, NULL, NULL, &texture, NULL));
	DELETE_PTR(texture_path);
}

void Model::init_from_file(const char *file_name)
{
	assert(file_name != NULL);
	name = get_file_name(file_name);
	
	char *file_extension = get_file_extension(file_name);
	if (!strcmp(file_extension, "fbx")) {
		Fbx_Binary_File fbx_file;
		fbx_file.read(file_name);
		fbx_file.fill_out_mesh(&mesh);
		fbx_file.get_texture_name();
	} else {
		printf("Model::init_from_file: %s is unkown model type, now only supports fbx file type\n", file_extension);
		return;
	}
}