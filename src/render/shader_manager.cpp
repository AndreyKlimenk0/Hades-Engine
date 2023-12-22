#include <assert.h>

#include "shader_manager.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../sys/sys_local.h"
#include "../render/render_api.h"
#include "../libs/str.h"

const u32 COMPILE_AS_VERTEX_SHADER   = 0x2;
const u32 COMPILE_AS_GEOMETRY_SHADER = 0x4;
const u32 COMPILE_AS_COMPUTE_SHADER  = 0x8;
const u32 COMPILE_AS_HULL_SHADER     = 0x10;
const u32 COMPILE_AS_DOMAIN_SHADER   = 0x20;
const u32 COMPILE_AS_PIXEL_SHADER    = 0x30;

enum Shader_Type {
	VERTEX_SHADER,
	GEOMETRY_SHADER,
	COMPUTE_SHADER,
	HULL_SHADER,
	DOMAIN_SHADER,
	PIXEL_SHADER,
};

struct Shader_Table_Entiry {
	const char *name = NULL;
	Extend_Shader *shader = NULL;
};

static const String HLSL_FILE_EXTENSION = "cso";
static const u32 SHADER_COUNT = sizeof(Shader_Manager::Shader_List) / sizeof(Shader);
static Shader_Table_Entiry shader_table[SHADER_COUNT];

static void get_shader_name_from_file(const char *file_name, String &name)
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

static bool get_shader_type_from_file_name(const char *file_name, Shader_Type *shader_type)
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

static inline Extend_Shader *find_shader_in_shader_table(const char *shader_name)
{
	for (u32 i = 0; i < SHADER_COUNT; i++) {
		if ((shader_table[i].name == NULL) || (shader_table[i].shader == NULL)) {
			continue;
		}
		if (shader_table[i].name == shader_name) {
			return shader_table[i].shader;
		}
	}
	return NULL;
}

static inline void check_shader_table_initialization()
{

}

void Shader_Manager::init(Gpu_Device *_gpu_device)
{
	assert(gpu_device);

	gpu_device = _gpu_device;

	u32 shader_index = 0;
	shader_table[shader_index++] = { "cascaded_shadow.hlsl", &shaders.cascaded_shadow };
	shader_table[shader_index++] = { "debug_cascaded_shadows.hlsl", &shaders.debug_cascaded_shadows };
	shader_table[shader_index++] = { "depth_map.hlsl", &shaders.depth_map };
	shader_table[shader_index++] = { "draw_line.hlsl", &shaders.draw_line };
	shader_table[shader_index++] = { "draw_vertices.hlsl", &shaders.draw_vertices };
	shader_table[shader_index++] = { "forward_light.hlsl", &shaders.forward_light };
	shader_table[shader_index++] = { "outlining.hlsl", &shaders.outlining };
	shader_table[shader_index++] = { "render_2d.hlsl", &shaders.render_2d };

	if ((SHADER_COUNT - 1) > shader_index) {
		print("WARNING");
	}

	String path_to_shader_dir;
	get_path_to_data_dir("shader", path_to_shader_dir);

	Array<String> file_names;
	bool success = get_file_names_from_dir(path_to_shader_dir, &file_names);

	if (!success) {
		error("init_shaders_table: has not found compiled shader files.");
	}

	for (u32 i = 0; i < file_names.count; i++) {
		String path_to_shader_file;
		build_full_path_to_shader_file(file_names[i], path_to_shader_file);

		String shader_name;
		get_shader_name_from_file(file_names[i].c_str(), shader_name);

		Extend_Shader *shader = find_shader_in_shader_table(shader_name);
		if (shader) {
			if ((shader->byte_code == NULL) && (shader->byte_code_size == 0)) {
				s32 file_size;
				char *compiled_shader = read_entire_file(path_to_shader_file, "rb", &file_size);
				if (!compiled_shader) {
					continue;
				}
				shader->byte_code = (u8 *)compiled_shader;
				shader->byte_code_size = file_size;
			}
			Shader_Type shader_type;
			if (!get_shader_type_from_file_name(file_names[i].c_str(), &shader_type)) {
				continue;
			}
			switch (shader_type) {
				case VERTEX_SHADER: {
					gpu_device->create_shader(shader->byte_code, shader->byte_code_size, shader->vertex_shader);
					break;
				}
				case GEOMETRY_SHADER: {
					gpu_device->create_shader(shader->byte_code, shader->byte_code_size, shader->geometry_shader);
					break;
				}
				case COMPUTE_SHADER: {
					gpu_device->create_shader(shader->byte_code, shader->byte_code_size, shader->compute_shader);
					break;
				}
				case HULL_SHADER: {
					gpu_device->create_shader(shader->byte_code, shader->byte_code_size, shader->hull_shader);
					break;
				}
				case DOMAIN_SHADER: {
					gpu_device->create_shader(shader->byte_code, shader->byte_code_size, shader->domain_shader);
					break;
				}
				case PIXEL_SHADER: {
					gpu_device->create_shader(shader->byte_code, shader->byte_code_size, shader->pixel_shader);
					break;
				}
				default: {
					assert(false);
				}
			}
		} else {

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
	Shader *shader = (Shader *)&shaders;
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
}
