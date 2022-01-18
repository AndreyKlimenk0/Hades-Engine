#ifndef EFFECT_H
#define EFFECT_H

#include <d3dx11effect.h>
#include "../game/world.h"
#include "../win32//win_types.h"
#include "../libs/os/camera.h"
#include "../libs/ds/hash_table.h"
#include "../sys/sys_local.h"


struct Fx_Shader_Manager;

extern Fx_Shader_Manager fx_shader_manager;

struct Fx_Shader {
	Fx_Shader(String *shader_name, ID3DX11Effect *fx_shader);
	~Fx_Shader();
	
	u32 technique_count;
	String name;
	Array<String> not_found_vars;
	ID3DX11Effect *shader = NULL;

	void attach(u32 technique_index = 0, u32 pass_index = 0);
	void attach(const char *technique_name, u32 pass_index = 0);
	void bind(const char *var_name, int scalar);
	void bind(const char *var_name, Vector4 *vector);
	void bind(const char *var_name, Vector3 *vector);
	void bind(const char *var_name, Matrix4 *matrix);
	void bind(const char *var_name, const Matrix4 &matrix);
	void bind(const char *var_name, ID3D11ShaderResourceView *texture);
	void bind(const char *var_name, void *struct_ptr, u32 struct_size);
	void bind_entity(Entity *entity, Matrix4 &view, Matrix4 &perspective, Render_Mesh *r);
	void bind_per_frame_info(Free_Camera *camera);
};

#define VALID_VAR_NAME(var, var_name, var_type) { \
	if (!var->IsValid()) { \
		error("There is no the variable with name {} of type {} in {}.fx", var_name, var_type, name); \
	} \
}

inline void Fx_Shader::attach(u32 technique_index, u32 pass_index)
{
	assert(technique_count > technique_index);

	shader->GetTechniqueByIndex(technique_index)->GetPassByIndex(pass_index)->Apply(0, directx11.device_context);
}

inline void Fx_Shader::attach(const char *technique_name, u32 pass_index)
{
	assert(technique_name);

	HR(shader->GetTechniqueByName(technique_name)->GetPassByIndex(pass_index)->Apply(0, directx11.device_context));
}

inline void Fx_Shader::bind(const char *var_name, int scalar)
{
	ID3DX11EffectScalarVariable * var = shader->GetVariableByName(var_name)->AsScalar();
	VALID_VAR_NAME(var, var_name, "Scalar");
	var->SetInt(scalar);
}

inline void Fx_Shader::bind(const char *var_name, Vector3 *vector)
{
	ID3DX11EffectVectorVariable *var = shader->GetVariableByName(var_name)->AsVector();
	VALID_VAR_NAME(var, var_name, "Vector3");
	var->SetRawValue(*vector, 0, sizeof(Vector3));
}

inline void Fx_Shader::bind(const char *var_name, Vector4 *vector)
{
	ID3DX11EffectVectorVariable * var = shader->GetVariableByName(var_name)->AsVector();
	VALID_VAR_NAME(var, var_name, "Vector4");
	var->SetFloatVector(*vector);
}

inline void Fx_Shader::bind(const char *var_name, Matrix4 *matrix)
{
	ID3DX11EffectMatrixVariable *var = shader->GetVariableByName(var_name)->AsMatrix();
	VALID_VAR_NAME(var, var_name, "Matrix");
	HR(var->SetMatrix(*matrix));
}

inline void Fx_Shader::bind(const char * var_name, const Matrix4 &matrix)
{
	ID3DX11EffectMatrixVariable *var = shader->GetVariableByName(var_name)->AsMatrix();
	VALID_VAR_NAME(var, var_name, "Matrix");
	HR(var->SetMatrix((Matrix4)matrix));
}

inline void Fx_Shader::bind(const char *var_name, ID3D11ShaderResourceView *texture)
{
	ID3DX11EffectShaderResourceVariable * var = shader->GetVariableByName(var_name)->AsShaderResource();
	VALID_VAR_NAME(var, var_name, "ShaderResource");
	HR(var->SetResource(texture));
}

inline void Fx_Shader::bind(const char *var_name, void *struct_ptr, u32 struct_size)
{
	ID3DX11EffectVariable * var = shader->GetVariableByName(var_name);
	VALID_VAR_NAME(var, var_name, "Struct");
	HR(var->SetRawValue(struct_ptr, 0, struct_size));
}

inline void Fx_Shader::bind_per_frame_info(Free_Camera *camera)
{
	bind("camera_position", &camera->position);
	bind("camera_direction", &camera->target);
}

struct Fx_Shader_Manager {
	~Fx_Shader_Manager();
	
	Hash_Table<String, Fx_Shader *> *shaders = NULL;
	
	void init();
	Fx_Shader *get_shader(const char *name);
};

struct Fx_Light {
	Vector4 position;
	Vector4 directon;
	Vector4 color;

	u32 light_type;
	float radius;
	float range;
	float pad;

};

void bind_light(Fx_Shader *forward_light_shader, Array<Light *> *lights);
void bind(Fx_Shader *fx_shader, Entity *entity);
void bind(Fx_Shader *fx_shader, Render_Mesh *render_mesh);

#endif