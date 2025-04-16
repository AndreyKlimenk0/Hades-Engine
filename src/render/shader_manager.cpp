#include <assert.h>
#include <string.h>
#include <windows.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include "shader_manager.h"
#include "../sys/sys.h"
#include "../sys/utils.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
//#include "../render/render_api.h"

using Microsoft::WRL::ComPtr;

const u32 COMPILE_AS_VERTEX_SHADER = 0x1;
const u32 COMPILE_AS_GEOMETRY_SHADER = 0x2;
const u32 COMPILE_AS_COMPUTE_SHADER = 0x4;
const u32 COMPILE_AS_HULL_SHADER = 0x8;
const u32 COMPILE_AS_DOMAIN_SHADER = 0x10;
const u32 COMPILE_AS_PIXEL_SHADER = 0x20;

struct Shader_Table_Entiry {
	const char *name = NULL;
	Shader *shader = NULL;
};

static const String HLSL_FILE_EXTENSION = "cso";
static const u32 SHADERS_COUNT = sizeof(Shader_Manager::Shader_List) / sizeof(Shader);
static Shader_Table_Entiry shader_table[SHADERS_COUNT];

inline Shader *find_shader_in_shader_table(const char *shader_name)
{
	for (u32 i = 0; i < SHADERS_COUNT; i++) {
		if ((shader_table[i].name == NULL) || (shader_table[i].shader == NULL)) {
			continue;
		}
		if (!strcmp(shader_table[i].name, shader_name)) {
			return shader_table[i].shader;
		}
	}
	return NULL;
}

inline bool include_shader(const char *shader_file_name)
{
	assert(shader_file_name);

	const u32 COUNT = 5;
	String include_shaders[COUNT] = { "light.hlsl", "utils.hlsl", "vertex.hlsl", "globals.hlsl", "cascaded_shadow.hlsl" };

	for (u32 i = 0; i < COUNT; i++) {
		if (include_shaders[i] == shader_file_name) {
			return true;
		}
	}
	return false;
}

inline void get_shader_name_from_file(const char *file_name, String &name)
{
	assert(name.is_empty());

	String f_name = file_name;

	Array<String> buffer;
	if (split(&f_name, "_", &buffer)) {
		for (u32 i = 0; i < (buffer.count - 1); i++) {
			if (i != 0) {
				name.append("_");
			}
			name.append(buffer[i]);
		}
	}
	name.append(".hlsl");
}

inline bool get_shader_type_from_file_name(const char *file_name, Shader_Type *shader_type)
{
	String name;
	String file_extension;

	extract_file_extension(file_name, file_extension);
	if (file_extension != HLSL_FILE_EXTENSION) {
		print("get_shader_type_from_file: {} has wrong file extension.", file_name);
		return false;
	}

	extract_base_file_name(file_name, name);

	Array<String> strings;
	bool result = split(&name, "_", &strings);
	if (!result) {
		print("get_shader_type_from_file: can not extract shader type from {}.", file_name);
		return false;
	}

	String type = strings.last();

	if (type == "vs") {
		*shader_type = VERTEX_SHADER;
	} else if (type == "gs") {
		*shader_type = GEOMETRY_SHADER;
	} else if (type == "cs") {
		*shader_type = COMPUTE_SHADER;
	} else if (type == "hs") {
		*shader_type = HULL_SHADER;
	} else if (type == "ds") {
		*shader_type = DOMAIN_SHADER;
	} else if (type == "ps") {
		*shader_type = PIXEL_SHADER;
	} else {
		print("get_shader_type_from_file: can not extract shader type from {}.", file_name);
		return false;
	}
	return true;
}

inline void make_output_shader_file_name(const char *shader_base_file_name, Shader_Type shader_type, String &output_shader_file_name)
{
	String compiled_shader_file_prefix;
	switch (shader_type) {
		case VERTEX_SHADER: {
			compiled_shader_file_prefix = "_vs";
			break;
		}
		case GEOMETRY_SHADER: {
			compiled_shader_file_prefix = "_gs";
			break;
		}
		case COMPUTE_SHADER: {
			compiled_shader_file_prefix = "_cs";
			break;
		}
		case HULL_SHADER: {
			compiled_shader_file_prefix = "_hs";
			break;
		}
		case DOMAIN_SHADER: {
			compiled_shader_file_prefix = "_ds";
			break;
		}
		case PIXEL_SHADER: {
			compiled_shader_file_prefix = "_ps";
			break;
		}
		default: {
			assert(false);
		}
	}
	output_shader_file_name = String(shader_base_file_name) + compiled_shader_file_prefix + ".cso";
}

inline bool compile_shader(const char *path_to_shader, Shader_Type shader_type, ID3DBlob **shader_bytecode)
{
	String profile;
	String entry_point;
	u32 complation_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
	switch (shader_type) {
		case VERTEX_SHADER: {
			entry_point = "vs_main";
			profile = "vs_5_0";
			break;
		}
		case GEOMETRY_SHADER: {
			entry_point = "gs_main";
			profile = "gs_5_0";
			break;
		}
		case COMPUTE_SHADER: {
			entry_point = "cs_main";
			profile = "cs_5_0";
			break;
		}
		case HULL_SHADER: {
			entry_point = "hs_main";
			profile = "hs_5_0";
			break;
		}
		case DOMAIN_SHADER: {
			entry_point = "ds_main";
			profile = "ds_5_0";
			break;
		}
		case PIXEL_SHADER: {
			entry_point = "ps_main";
			profile = "ps_5_0";
			break;
		}
		default: {
			assert(false);
		}
	}
	ComPtr<ID3DBlob> error_message;
	wchar_t *temp_path_to_shader = to_wstring(path_to_shader);
	defer(free_string(temp_path_to_shader));

	bool result = SUCCEEDED(D3DCompileFromFile(temp_path_to_shader, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry_point, profile, complation_flags, 0, shader_bytecode, error_message.ReleaseAndGetAddressOf()));

	if (!result && (error_message->GetBufferSize() > 0)) {
		const char *str_error_message = (const char *)error_message->GetBufferPointer();
		// Get rid of the new line character.
		String temp = String(str_error_message, 0, (u32)strlen(str_error_message) - 1);
		print("compile_shader:", temp);
	}

	return result;
}

Shader_Manager::Shader_Manager()
{
}

Shader_Manager::~Shader_Manager()
{
	shutdown();
}

void Shader_Manager::init()
{
	u32 shader_count = 0;
	shader_table[shader_count++] = { "debug_cascaded_shadows.hlsl", &shaders.debug_cascaded_shadows };
	shader_table[shader_count++] = { "depth_map.hlsl", &shaders.depth_map };
	shader_table[shader_count++] = { "draw_vertices.hlsl", &shaders.draw_vertices };
	shader_table[shader_count++] = { "forward_light.hlsl", &shaders.forward_light };
	shader_table[shader_count++] = { "outlining.hlsl", &shaders.outlining };
	shader_table[shader_count++] = { "render_2d.hlsl", &shaders.render_2d };
	shader_table[shader_count++] = { "silhouette.hlsl", &shaders.silhouette };
	shader_table[shader_count++] = { "voxelization.hlsl", &shaders.voxelization };
	shader_table[shader_count++] = { "draw_box.hlsl", &shaders.draw_box };
	shader_table[shader_count++] = { "generate_mips_linear.hlsl", &shaders.generate_mips_linear };
	shader_table[shader_count++] = { "generate_mips_linear_odd.hlsl", &shaders.generate_mips_linear_odd };
	shader_table[shader_count++] = { "generate_mips_linear_oddx.hlsl", &shaders.generate_mips_linear_oddx };
	shader_table[shader_count++] = { "generate_mips_linear_oddy.hlsl", &shaders.generate_mips_linear_oddy };

	for (u32 i = 0; i < shader_count; i++) {
		shader_table[i].shader->file_name = shader_table[i].name;
	}

	if ((SHADERS_COUNT - 1) > shader_count) {
		print("Shader_Manager::init: Number of shaders in Shader_List struct is more than number of shader entries in the shader table. Maybe some shader was not added to the shader table.");
	}

	String path_to_shader_dir;
	build_full_path_to_data_directory("shaders", path_to_shader_dir);

	Array<String> file_names;
	get_file_names_from_dir(path_to_shader_dir, &file_names);
	if (file_names.is_empty()) {
		print("Shader_Manager::init: Shader Manager has not found compiled shader files.");
	} else {
		print("Shader_Manager::init: Load and create shaders.");
	}

	for (u32 i = 0; i < file_names.count; i++) {
		String path_to_shader_file;
		build_full_path_to_shader_file(file_names[i], path_to_shader_file);

		String shader_name;
		get_shader_name_from_file(file_names[i].c_str(), shader_name);

		Shader *shader = find_shader_in_shader_table(shader_name);
		if (shader) {
			u8 *bytecode = NULL;
			s32 bytecode_size = 0;
			bytecode = (u8 *)read_entire_file(path_to_shader_file, "rb", &bytecode_size);
			if (!bytecode || (bytecode_size == 0)) {
				print("Shader_Manager::init: Failed to read shader byte code from {}.", &path_to_shader_file);
				continue;
			}
			Shader_Type shader_type;
			if (!get_shader_type_from_file_name(file_names[i].c_str(), &shader_type)) {
				print("Shader_Manager::init: The shader manager can get a shader type from {}.", file_names[i].c_str());
				continue;
			}
			switch (shader_type) {
				case VERTEX_SHADER: {
					shader->vs_bytecode.move(bytecode, (u32)bytecode_size);
					shader->types.push(VERTEX_SHADER);
					break;
				}
				case GEOMETRY_SHADER: {
					shader->gs_bytecode.move(bytecode, (u32)bytecode_size);
					shader->types.push(GEOMETRY_SHADER);
					break;
				}
				case COMPUTE_SHADER: {
					shader->cs_bytecode.move(bytecode, (u32)bytecode_size);
					shader->types.push(COMPUTE_SHADER);
					break;
				}
				case HULL_SHADER: {
					shader->hs_bytecode.move(bytecode, (u32)bytecode_size);
					shader->types.push(HULL_SHADER);
					break;
				}
				case DOMAIN_SHADER: {
					shader->ds_bytecode.move(bytecode, (u32)bytecode_size);
					shader->types.push(DOMAIN_SHADER);
					break;
				}
				case PIXEL_SHADER: {
					shader->ps_bytecode.move(bytecode, (u32)bytecode_size);
					shader->types.push(PIXEL_SHADER);
					break;
				}
				default: {
					assert(false);
				}
			}
			loop_print("  {} was loaded.", shader_name);
		} else {
			print("Shader_Manager::init: The shader table doesn't have a shader entiry with name {}.", &shader_name);
		}
	}
}

void Shader_Manager::reload(void *arg)
{
	const char *shader_file_name = (const char *)arg;

	Array<Shader *> shaders;
	if (include_shader(shader_file_name)) {
		for (u32 i = 0; i < SHADERS_COUNT; i++) {
			shaders.push(shader_table[i].shader);
		}
	} else {
		Shader *shader = find_shader_in_shader_table(shader_file_name);
		if (shader) {
			shaders.push(shader);
		} else {
			print("Shader_Manager::reload: {} was not found in the shader table. The shader can't be compiled and reloaded.", shader_file_name);
		}
	}
	recompile_and_reload_shaders(shaders);
}

void Shader_Manager::recompile_and_reload_shaders(Array<Shader *> &shaders)
{
	for (u32 shader_index = 0; shader_index < shaders.count; shader_index++) {
		Shader *shader = shaders[shader_index];

		String full_path_to_source_shader;
		build_full_path_to_source_shader_file(shader->file_name, full_path_to_source_shader);

		Array<ComPtr<ID3DBlob>> compiled_shaders;
		for (u32 i = 0; i < shader->types.count; i++) {
			ComPtr<ID3DBlob> shader_bytecode;
			if (!compile_shader(full_path_to_source_shader.c_str(), shader->types[i], shader_bytecode.ReleaseAndGetAddressOf())) {
				break;
			}
			compiled_shaders.push(shader_bytecode);
		}

		if ((!shader->types.is_empty() && !compiled_shaders.is_empty()) && (shader->types.count == compiled_shaders.count)) {
			Array<Shader_Type> shader_types = shader->types;
			String shader_file_name = shader->file_name;
			shader->free();
			shader->file_name = shader_file_name;

			String base_file_name;
			extract_base_file_name(shader->file_name, base_file_name);

			for (u32 i = 0; i < shader_types.count; i++) {
				String output_shader_file_name;
				make_output_shader_file_name(base_file_name.c_str(), shader_types[i], output_shader_file_name);

				String full_path_to_shader_file;
				build_full_path_to_shader_file(output_shader_file_name, full_path_to_shader_file);

				File shader_file;
				shader_file.open(full_path_to_shader_file, FILE_MODE_WRITE, FILE_CREATE_ALWAYS);
				shader_file.write(compiled_shaders[i]->GetBufferPointer(), (u32)compiled_shaders[i]->GetBufferSize());
			}
			print("Shader_Manager::recompile_and_reload_shaders: {} was successfully recompiled and reloaded.", shader->file_name);
		}
	}
}

void Shader_Manager::shutdown()
{
	Shader *shader = (Shader *)&shaders;
	for (u32 i = 0; i < SHADERS_COUNT; i++) {
		shader->free();
		shader++;
	}
}

Shader::Shader()
{
}

Shader::~Shader()
{
	free();
}

void Shader::free()
{
	file_name.free();
	types.clear();
	vs_bytecode.free();
	gs_bytecode.free();
	cs_bytecode.free();
	hs_bytecode.free();
	ds_bytecode.free();
	ps_bytecode.free();
}

Bytecode::Bytecode()
{
}

Bytecode::~Bytecode()
{
	free();
}

void Bytecode::free()
{
	DELETE_PTR(data);
	size = 0;
}

void Bytecode::move(u8 *bytecode, u32 bytecode_size)
{
	data = bytecode;
	size = bytecode_size;
}

D3D12_SHADER_BYTECODE Bytecode::d3d12_shader_bytecode()
{
	return { (void *)data, (SIZE_T)size };
}
