#ifndef EFFECT_H
#define EFFECT_H

#include <d3dx11effect.h>
#include "../render/base.h"
#include "../libs/ds/hash_table.h"
#include "../win32//win_types.h"


Hash_Table<const char *, ID3DX11Effect *> *get_fx_shaders(const Direct3D *direct3d);


struct Fx_Shader {
	Fx_Shader(String *shader_name, ID3DX11Effect *fx_shader);
	~Fx_Shader();
	
	u32 technique_count;
	String name;
	Array<String> not_found_vars;
	ID3DX11Effect *shader = NULL;

	void attach(u32 technique_index = 0, u32 pass_index = 0);
	void bind(const char *var_name, Vector4 *vector);
	void bind(const char *var_name, Matrix4 *matrix);
	void bind(const char *var_name, ID3D11ShaderResourceView *texture);

	bool was_var_not_found(const char *var_name);
};

struct Fx_Shader_Manager {
	~Fx_Shader_Manager();
	
	Hash_Table<String, Fx_Shader *> *shaders = NULL;
	
	void init();
	Fx_Shader *get_shader(const char *name);
};

extern Fx_Shader_Manager fx_shader_manager;

#endif