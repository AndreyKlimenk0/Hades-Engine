#include <assert.h>
#include <string.h>

#include "shader_manager.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../sys/sys_local.h"
#include "../render/render_api.h"
#include "../libs/str.h"

const u32 COMPILE_AS_VERTEX_SHADER   = 0x1;
const u32 COMPILE_AS_GEOMETRY_SHADER = 0x2;
const u32 COMPILE_AS_COMPUTE_SHADER  = 0x4;
const u32 COMPILE_AS_HULL_SHADER     = 0x8;
const u32 COMPILE_AS_DOMAIN_SHADER   = 0x10;
const u32 COMPILE_AS_PIXEL_SHADER    = 0x20;

enum Shader_Type {
	VERTEX_SHADER,
	GEOMETRY_SHADER,
	COMPUTE_SHADER,
	HULL_SHADER,
	DOMAIN_SHADER,
	PIXEL_SHADER,
};

bool is_valid(Shader *shader, u32 flags)
{
	if (flags & VALIDATE_VERTEX_SHADER) {
		if (!shader->vertex_shader) {
			return false;
		}
	}
	if (flags & VALIDATE_GEOMETRY_SHADER) {
		assert(false);
	}
	if (flags & VALIDATE_COMPUTE_SHADER) {
		assert(false);
	}
	if (flags & VALIDATE_HULL_SHADER) {
		assert(false);
	}
	if (flags & VALIDATE_DOMAIN_SHADER) {
		assert(false);
	}
	if (flags & VALIDATE_PIXEL_SHADER) {
		if (!shader->pixel_shader) {
			return false;
		}
	}
	return true;
}

struct Shader_Table_Entiry {
	const char *name = NULL;
	Extend_Shader *shader = NULL;
};

static const String HLSL_FILE_EXTENSION = "cso";
static const u32 SHADER_COUNT = sizeof(Shader_Manager::Shader_List) / sizeof(Extend_Shader);
static Shader_Table_Entiry shader_table[SHADER_COUNT];

inline  void get_shader_name_from_file(const char *file_name, String &name)
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

	extract_file_name(file_name, name);

	Array<String> strings;
	bool result = split(&name, "_", &strings);
	if (!result) {
		print("get_shader_type_from_file: can not extract shader type from {}.", file_name);
		return false;
	}

	String type = strings.last_item();

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

Shader_Manager::Shader_Manager()
{
}

Shader_Manager::~Shader_Manager()
{
	shutdown();
}

inline Extend_Shader *find_shader_in_shader_table(const char *shader_name)
{
	for (u32 i = 0; i < SHADER_COUNT; i++) {
		if ((shader_table[i].name == NULL) || (shader_table[i].shader == NULL)) {
			continue;
		}
		if (!strcmp(shader_table[i].name, shader_name)) {
			return shader_table[i].shader;
		}
	}
	return NULL;
}

void Shader_Manager::init(Gpu_Device *_gpu_device)
{
	assert(_gpu_device);
	print("------------------------- Initializing Shader Manager -------------------------");

	gpu_device = _gpu_device;

	u32 shader_index = 0;
	shader_table[shader_index++] = { "cascaded_shadow.hlsl", &shaders.cascaded_shadow };
	shader_table[shader_index++] = { "debug_cascaded_shadows.hlsl", &shaders.debug_cascaded_shadows };
	shader_table[shader_index++] = { "depth_map.hlsl", &shaders.depth_map };
	shader_table[shader_index++] = { "draw_lines.hlsl", &shaders.draw_lines };
	shader_table[shader_index++] = { "draw_vertices.hlsl", &shaders.draw_vertices };
	shader_table[shader_index++] = { "forward_light.hlsl", &shaders.forward_light };
	shader_table[shader_index++] = { "outlining.hlsl", &shaders.outlining };
	shader_table[shader_index++] = { "render_2d.hlsl", &shaders.render_2d };

	if ((SHADER_COUNT - 1) > shader_index) {
		print("Shader_Manager::init: Number of shaders in Shader_List struct is more than number of shader entries in the shader table. Maybe some shader was not added to the shader table.");
	}

	String path_to_shader_dir;
	get_path_to_data_dir("shader", path_to_shader_dir);

	Array<String> file_names;
	bool success = get_file_names_from_dir(path_to_shader_dir, &file_names);

	if (!success) {
		error("Shader_Manager::init: has not found compiled shader files.");
	}

	for (u32 i = 0; i < file_names.count; i++) {
		String path_to_shader_file;
		build_full_path_to_shader_file(file_names[i], path_to_shader_file);

		String shader_name;
		get_shader_name_from_file(file_names[i].c_str(), shader_name);

		Extend_Shader *shader = find_shader_in_shader_table(shader_name);
		if (shader) {
			shader->name = shader_name;

			u8 *byte_code = NULL;
			s32 byte_code_size = 0;
			if ((shader->byte_code == NULL) && (shader->byte_code_size == 0)) {
				byte_code = (u8 *)read_entire_file(path_to_shader_file, "rb", &byte_code_size);
				if (!byte_code) {
					print("Shader_Manager::init: Failed to read shader byte code from {}.", &path_to_shader_file);
					continue;
				}
			}
			Shader_Type shader_type;
			if (!get_shader_type_from_file_name(file_names[i].c_str(), &shader_type)) {
				print("Shader_Manager::init: The shader manager can get a shader type from {}.", file_names[i].c_str());
				continue;
			}
			print("Shader_Manager::init: {} was loaded.", shader_name);
			switch (shader_type) {
				case VERTEX_SHADER: {
					gpu_device->create_shader(byte_code, byte_code_size, shader->vertex_shader);
					if ((shader->byte_code_size == 0) && (shader->byte_code == NULL)) {
						//@Note: Save vertex shader byte code for creating directx input layouts.
						shader->byte_code = byte_code;
						shader->byte_code_size = byte_code_size;
					}
					break;
				}
				case GEOMETRY_SHADER: {
					gpu_device->create_shader(byte_code, byte_code_size, shader->geometry_shader);
					break;
				}
				case COMPUTE_SHADER: {
					gpu_device->create_shader(byte_code, byte_code_size, shader->compute_shader);
					break;
				}
				case HULL_SHADER: {
					gpu_device->create_shader(byte_code, byte_code_size, shader->hull_shader);
					break;
				}
				case DOMAIN_SHADER: {
					gpu_device->create_shader(byte_code, byte_code_size, shader->domain_shader);
					break;
				}
				case PIXEL_SHADER: {
					gpu_device->create_shader(byte_code, byte_code_size, shader->pixel_shader);
					break;
				}
				default: {
					assert(false);
				}
			}
		} else {
			print("Shader_Manager::init: The shader table doesn't have a shader entiry with name {}.", &shader_name);
		}
	}
}

void Shader_Manager::reload(void *arg)
{
	const char *shader_file_name = (const char *)arg;
	print("Recompile and reload", shader_file_name);
}

void Shader_Manager::shutdown()
{
	Extend_Shader *shader = (Extend_Shader *)&shaders;
	for (u32 i = 0; i < SHADER_COUNT; i++) {
		shader->free();
		shader++;
	}
}

Extend_Shader::Extend_Shader()
{
}

Extend_Shader::~Extend_Shader()
{
	free();
}

void Extend_Shader::free()
{
	Shader::free();
	DELETE_PTR(byte_code);
	byte_code_size = 0;
	name.free();
}
