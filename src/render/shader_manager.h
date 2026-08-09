#ifndef SHADER_MANAGER
#define SHADER_MANAGER

#include <windows.h>
#include <dxcapi.h>
#include <wrl/client.h>

#include "shader_manager.h"
#include "render_api\render.h"
#include "../libs/str.h"
#include "../libs/number_types.h"
#include "../libs/structures/array.h"
#include "../libs/structures/hash_table.h"

using Microsoft::WRL::ComPtr;

enum Shader_Type {
	VERTEX_SHADER,
	GEOMETRY_SHADER,
	COMPUTE_SHADER,
	HULL_SHADER,
	DOMAIN_SHADER,
	PIXEL_SHADER,
};

struct Shader_Entry {
	const char *shader_file_name = NULL;
	const char *shader_alias = NULL;
	const char *arguments = NULL;
};

struct Shader_Compilation_Result {
	bool compiled = false;
	u64 last_write_time = 0;
	Shader_Bytecode bytecode;
};

struct Shader_Compiler {
	Shader_Compiler();
	~Shader_Compiler();

	String cso_directory;
	String shader_pdb_directory;
	String shader_source_directory;

	ComPtr<IDxcUtils> utils;
	ComPtr<IDxcCompiler3> compiler;
	ComPtr<IDxcIncludeHandler> include_handler;

	Shader_Compilation_Result compile(void *shader_code, u32 code_size, const char *path_to_shader_file, const char *file_name, Shader_Type shader_type);
};

struct Shader_Data {
	String source_file;

	Shader_Bytecode vs_bytecode;
	Shader_Bytecode gs_bytecode;
	Shader_Bytecode cs_bytecode;
	Shader_Bytecode hs_bytecode;
	Shader_Bytecode ds_bytecode;
	Shader_Bytecode ps_bytecode;

	void set_bytecode(Shader_Type shader_type, Shader_Bytecode bytecode);
	Shader_Bytecode get_bytecode(Shader_Type shader_type);
};

struct Shader_Compilation_Info {
	String shader_file_name;
	String shader_alias;
	String cso_file_name;
	u64 cso_file_last_write_time;
	String shader_compilation_args;
};

struct Shader_Manager {
	Shader_Compiler shader_compiler;

	Array<Shader_Compilation_Info> shader_info_list;
	Hash_Table<String, Shader_Data *> shader_table;

	void init();
	void shutdown();

	void load_shader_compilation_info();
	void update_shader_compilation_info(u64 cso_file_last_write_time, Shader_Entry *shader_entry, Shader_Type shader_type);

	Shader_Bytecode get_shader_bytecode(const char *shader_alias, Shader_Type shader_type);
};

#endif