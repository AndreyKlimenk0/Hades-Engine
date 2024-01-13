#ifndef SHADER_MANAGER
#define SHADER_MANAGER

#include <stdio.h>

#include "render_api.h"
#include "../libs/str.h"
#include "../win32/win_types.h"

#define GET_SHADER(shader_manager, shader_name) ((Extend_Shader *)&shader_manager->shaders.shader_name)

//@Note: I think it is better in the future to split the struct on Render_Shader(it shader holds vertex shader and shader), Computer_Shader, Geometry_Shader and Tessellation_Shader
struct Extend_Shader : Shader {
	Extend_Shader();
	~Extend_Shader();

	u8 *byte_code = NULL;
	u32 byte_code_size = 0;
	u32 compiling_flags = 0;
	String name;

	void free();
};

const u32 VALIDATE_VERTEX_SHADER = 0x1;
const u32 VALIDATE_GEOMETRY_SHADER = 0x2;
const u32 VALIDATE_COMPUTE_SHADER = 0x4;
const u32 VALIDATE_HULL_SHADER = 0x8;
const u32 VALIDATE_DOMAIN_SHADER = 0x10;
const u32 VALIDATE_PIXEL_SHADER = 0x20;
const u32 VALIDATE_RENDERING_SHADER = VALIDATE_VERTEX_SHADER | VALIDATE_PIXEL_SHADER;

bool is_valid(Shader *shader, u32 flags);

struct Shader_Manager {
	Shader_Manager();
	~Shader_Manager();

	struct Shader_List {
		Extend_Shader cascaded_shadow;
		Extend_Shader debug_cascaded_shadows;
		Extend_Shader depth_map;
		Extend_Shader draw_lines;
		Extend_Shader draw_vertices;
		Extend_Shader forward_light;
		Extend_Shader outlining;
		Extend_Shader render_2d;
		Extend_Shader silhouette;
	} shaders;

	Gpu_Device *gpu_device = NULL;

	void init(Gpu_Device *gpu_device);
	void reload(void *arg);
	void shutdown();
};
#endif