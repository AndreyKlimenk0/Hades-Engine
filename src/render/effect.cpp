#include <assert.h>

#include "base.h"
#include "effect.h"
#include "../libs/general.h"
#include "../libs/ds/string.h"
#include "../framework/file.h"


Hash_Table<const char *, ID3DX11Effect *> *get_fx_shaders(const Direct3D *direct3d)
{
	static Hash_Table<const char *, ID3DX11Effect *> *effects = NULL;
	if (!effects) {
		Array<char *> file_name;
		Array<char *> file_names;
		const char *full_path_to_dir = "D:\\andrey\\dev\\Simple-Game-Engine-master\\Debug\\compiled_fx\\";
		effects = new Hash_Table<const char *, ID3DX11Effect *>();

		bool success = get_file_names_from_dir(full_path_to_dir, &file_names);
		assert(success);

		for (int i = 0; i < file_names.count; i++) {
			int file_size;
			char *compiled_shader = read_entire_file(concatenate_c_str(full_path_to_dir, file_names[i]), "rb", &file_size);
			if (!compiled_shader)
				continue;

			ID3DX11Effect *fx = NULL;
			HR(D3DX11CreateEffectFromMemory(compiled_shader, sizeof(char) * file_size, 0, direct3d->device, &fx, NULL));

			split(file_names[i], ".", &file_name);
			effects->set(_strdup(file_name[0]), fx);

			DELETE_PTR(compiled_shader);
		}
	}
	return effects;
}