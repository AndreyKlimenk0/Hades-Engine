#ifndef EFFECT_H
#define EFFECT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "../framework/file.h"

struct Fx_Shader {
	char *name = NULL;
	char *path = NULL;
};


void init_fx_shaders()
{
	const char *full_path = "E:\\andrey\\dev\\hades\\Debug\\compiled_fx\\*";
	Array<char *> *file_names = get_files_name_from_dir(full_path);
	for (int i = 0; i < file_names->count; i++) {

	}
}
#endif