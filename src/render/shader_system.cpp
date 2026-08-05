#include <stdlib.h>

#include <Windows.h>
#include <dxcapi.h>
#include <wrl/client.h>

#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../sys/sys.h"
#include "shader_system.h"

#pragma comment(lib, "dxcompiler.lib")

using Microsoft::WRL::ComPtr;

static const char *SHADER_INFO_FILE_NAME = "shader_compilation_info.txt";

struct Shader_Entry {
	const char *shader_file_name = NULL;
	const char *arguments = NULL;
};

static const Shader_Entry SHADER_LIST[] = {
	{ "culling.hlsl", "-DDEBUG_SHADOWS" },
	{ "empty.hlsl", "-DDEBUG_SHADOWS" },
	{ "mesh_culling.hlsl", "-DDEBUG_SHADOWS" },
};

#define ARRAY_SIZE(list) (sizeof(list) / sizeof(list[0]))

void find_entry_points(const char *code, u32 code_size)
{
	for (u32 i = 0; i < code_size; i++) {

	}
}

void Shader_System::init()
{
	//String path_to_data_directory;
	//const char *full_path_to_data_dir = get_full_path_to_data_directory();

 //   String full_path_to_variable_file = join_paths(get_full_path_to_data_directory(), file_name);
 //   if (!file_exists(full_path_to_variable_file)) {
 //       print("Variable_Service: File {} doesn't exist in the data directory.", file_name);
 //       return;
 //   }

 //   char *data = read_entire_file(full_path_to_variable_file, "rb");
 //   if (data) {
 //       parse(data);
 //       free_string(data);
 //   }

	ComPtr<IDxcUtils> utils;
	ComPtr<IDxcCompiler3> compiler;

	for (u32 i = 0; i < ARRAY_SIZE(SHADER_LIST); i++) {
		Shader_Entry shader_entry = SHADER_LIST[i];

		String full_path_to_shader_file = join_paths(join_paths(get_base_path(), "hlsl"), shader_entry.shader_file_name);

		if (!file_exists(full_path_to_shader_file)) {
			print("Shader_System::init: Shader compilcation failed, {} was found.", shader_entry.shader_file_name);
			continue;
		}

		s32 shader_file_size = 0;
		u8 *shader_code = (u8 *)read_entire_file(full_path_to_shader_file, "rb", &shader_file_size);
		if (!shader_code || (shader_file_size == 0)) {
			print("Shader_System::init: Faield to read {}", shader_entry.shader_file_name);
			DELETE_ARRAY(shader_code);
			continue;
		}

		String temp;
		temp.move((char *)shader_code);
		if (temp.find("cs_main")) {
			int x = 0;
		}
		int z = 0;
		
		//DELETE_ARRAY(shader_code);
	}
}