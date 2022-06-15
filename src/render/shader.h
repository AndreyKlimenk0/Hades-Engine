#ifndef SHADER_H
#define SHADER_H

#include "directx.h"
#include "../libs/ds/hash_table.h"


enum Shader_Type {
	VERTEX_SHADER,
	GEOMETRY_SHADER,
	COMPUTE_SHADER,
	HULL_SHADER,
	DOMAIN_SHADER,
	PIXEL_SHADER,
};

//enum Shader_Buffer_Type {
//
//};

struct Shader_Buffer {
	u32 index;
	//Shader_Buffer_Type type;
	String name;
};

struct Shader {
	Shader() {};
	~Shader();

	Vertex_Shader *vertex_shader = NULL;
	Geometry_Shader *geometry_shader = NULL;
	Compute_Shader *compute_shader = NULL;
	Hull_Shader *hull_shader = NULL;
	Domain_Shader *domain_shader = NULL;
	Pixel_Shader *pixel_shader = NULL;

	u8 *byte_code = NULL;
	u32 byte_code_size = 0;

	String name;
};

struct Shader_Manager {
	Shader_Manager() {};

	Hash_Table<String, Shader *> shaders;

	void init();
	Shader *get_shader(const char *name);
};

inline Shader *Shader_Manager::get_shader(const char *shader_name)
{
	return shaders[shader_name];
}

#define DECLARE_CBUFFER(name);


struct CB_Render_Matix {
	Matrix4 world_view_perspective;
	Matrix4 orthographic;
};

#endif
