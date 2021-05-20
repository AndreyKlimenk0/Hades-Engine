#ifndef EFFECT_H
#define EFFECT_H

#include <d3dx11effect.h>
#include "../game/entity.h"
#include "../win32//win_types.h"
#include "../libs/os/camera.h"
#include "../libs/ds/hash_table.h"


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
	void bind(const char *var_name, ID3D11ShaderResourceView *texture);
	void bind(const char *var_name, void *struct_ptr, u32 struct_size);
	void bind_per_entity_vars(Entity *entity, Matrix4 &view, Matrix4 &perspective);
	void bind_per_frame_vars(Free_Camera *camera);

	bool is_var_in_not_found_vars_array(const char *var_name);
};

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

void bind_light_entities(Fx_Shader *forward_light_shader, Array<Light *> *lights);

#endif