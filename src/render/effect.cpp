#include <assert.h>

#include "base.h"
#include "effect.h"
#include "../sys/sys_local.h"
#include "../libs/str.h"
#include "../framework/file.h"

#include <string.h>


Hash_Table<const char *, ID3DX11Effect *> *get_fx_shaders(const Direct3D *direct3d)
{
	static Hash_Table<const char *, ID3DX11Effect *> *effects = NULL;

	if (!effects) {
		Array<char *> file_name;
		Array<char *> file_names;
		String path_to_shader_dir;

		effects = new Hash_Table<const char *, ID3DX11Effect *>();

		os_path.data_dir_paths.get("shader", path_to_shader_dir);
		
		bool success = get_file_names_from_dir(path_to_shader_dir + "\\", &file_names);
		assert(success);
		
		for (int i = 0; i < file_names.count; i++) {
			int file_size;
			
			String *path_to_shader_file = os_path.build_full_path_to_shader_file(&String(file_names[i]));
			defer(path_to_shader_file->free());

			char *compiled_shader = read_entire_file(*path_to_shader_file, "rb", &file_size);
			if (!compiled_shader)
				continue;

			ID3DX11Effect *fx = NULL;
			HR(D3DX11CreateEffectFromMemory(compiled_shader, sizeof(char) * file_size, 0, direct3d->device, &fx, NULL));

			split(file_names[i], ".", &file_name);
			effects->set(file_name[0], fx);

			DELETE_PTR(compiled_shader);
		}
	}
	return effects;
}