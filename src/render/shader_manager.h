#ifndef SHADER_MANAGER
#define SHADER_MANAGER

#include <stdio.h>
#include <d3d12.h>

#include "../libs/str.h"
#include "../libs/number_types.h"
#include "../libs/structures/array.h"

#define GET_SHADER(shader_manager, shader_name) ((Shader *)&shader_manager->shaders.shader_name)

enum Shader_Type {
	VERTEX_SHADER,
	GEOMETRY_SHADER,
	COMPUTE_SHADER,
	HULL_SHADER,
	DOMAIN_SHADER,
	PIXEL_SHADER,
};

struct Bytecode {
	Bytecode();
	~Bytecode();

	u8 *data = NULL;
	u32 size = 0; // Should it be s64 ?

	void free();
	void move(u8 *bytecode, u32 bytecode_size);
	D3D12_SHADER_BYTECODE d3d12_shader_bytecode();
};

struct Shader {
	Shader();
	~Shader();

	Bytecode vs_bytecode;
	Bytecode gs_bytecode;
	Bytecode cs_bytecode;
	Bytecode hs_bytecode;
	Bytecode ds_bytecode;
	Bytecode ps_bytecode;

	String file_name;
	Array<Shader_Type> types;

	void free();
};

const u32 VALIDATE_VERTEX_SHADER = 0x1;
const u32 VALIDATE_GEOMETRY_SHADER = 0x2;
const u32 VALIDATE_COMPUTE_SHADER = 0x4;
const u32 VALIDATE_HULL_SHADER = 0x8;
const u32 VALIDATE_DOMAIN_SHADER = 0x10;
const u32 VALIDATE_PIXEL_SHADER = 0x20;
const u32 VALIDATE_RENDERING_SHADER = VALIDATE_VERTEX_SHADER | VALIDATE_PIXEL_SHADER;


struct Shader_Manager {
	Shader_Manager();
	~Shader_Manager();

	struct Shader_List {
		Shader debug_cascaded_shadows;
		Shader depth_map;
		Shader draw_vertices;
		Shader forward_light;
		Shader outlining;
		Shader render_2d;
		Shader silhouette;
		Shader voxelization;
		Shader draw_box;
	} shaders;

	void init();
	void reload(void *arg);
	void recompile_and_reload_shaders(Array<Shader *> &shaders);
	void shutdown();
};
#endif