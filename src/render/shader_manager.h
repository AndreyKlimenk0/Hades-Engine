#ifndef SHADER_MANAGER
#define SHADER_MANAGER

#include <stdio.h>

#include "render_api.h"
#include "../win32/win_types.h"

#define GET_SHADER(shader_manager, shader_name) ((Extend_Shader *)&shader_manager->shaders.shader_name)

struct Extend_Shader : Shader {
	Extend_Shader();
	~Extend_Shader();

	u8 *byte_code = NULL;
	u32 byte_code_size = 0;
	u32 compiling_flags = 0;

	void free();
};

struct Shader_Manager {
	Shader_Manager();
	~Shader_Manager();

	struct Shader_List {
		Extend_Shader cascaded_shadow;
		Extend_Shader debug_cascaded_shadows;
		Extend_Shader depth_map;
		Extend_Shader draw_line;
		Extend_Shader draw_vertices;
		Extend_Shader forward_light;
		Extend_Shader outlining;
		Extend_Shader render_2d;
	} shaders;

	Gpu_Device *gpu_device = NULL;

	void init(Gpu_Device *gpu_device);
	void reload(void *arg);
	void shutdown();
};
#endif