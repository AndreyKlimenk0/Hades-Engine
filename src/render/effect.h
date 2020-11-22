#ifndef EFFECT_H
#define EFFECT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <d3dx11effect.h>

#include "../render/base.h"
#include "../framework/file.h"
#include "../elib/ds/string.h"


struct Fx_Shader {
	char *name = NULL;
	ID3DX11Effect *fx = NULL;
};


void init_fx_shaders(Direct3D_State *direct3d)
{
	const char *full_path_to_dir = "E:\\andrey\\dev\\hades\\Debug\\compiled_fx\\";
	Array<char *> *file_names = get_file_names_from_dir(full_path_to_dir);
	for (int i = 0; i < file_names->count; i++) {
		char *compiled_shader = read_entire_file(concatenate_c_str(full_path_to_dir, file_names->at(i)));
		if (!compiled_shader) 
			continue;
		
		Fx_Shader * fx = new Fx_Shader();
		fx->name = file_names->at(i);
		
		D3DX11CreateEffectFromMemory(compiled_shader, sizeof(char) * strlen(compiled_shader), 0, direct3d->device, &fx->fx, NULL);
	}
}
#endif