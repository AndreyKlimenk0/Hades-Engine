#include <assert.h>
#include <string.h>

#include "base.h"
#include "effect.h"

#include "../sys/sys_local.h"

#include "../libs/str.h"
#include "../libs/os/file.h"
#include "../libs/os/path.h"


Fx_Shader_Manager fx_shader_manager;


Hash_Table<const char *, ID3DX11Effect *> *get_fx_shaders(const Direct3D *direct3d)
{
	static Hash_Table<const char *, ID3DX11Effect *> *effects = NULL;

	if (!effects) {
		Array<char *> file_name;
		Array<String> file_names;
		String path_to_shader_dir;

		effects = new Hash_Table<const char *, ID3DX11Effect *>();

		os_path.data_dir_paths.get("shader", path_to_shader_dir);
		
		bool success = get_file_names_from_dir(path_to_shader_dir + "\\", &file_names);
		assert(success);
		
		for (int i = 0; i < file_names.count; i++) {
			int file_size;
			
			String *path_to_shader_file = os_path.build_full_path_to_shader_file(&String(file_names[i]));
			defer(path_to_shader_file->free());

			char *compiled_shader = read_entire_file(*path_to_shader_file, "rb", &file_size);
			if (!compiled_shader)
				continue;

			ID3DX11Effect *fx = NULL;
			HR(D3DX11CreateEffectFromMemory(compiled_shader, sizeof(char) * file_size, 0, direct3d->device, &fx, NULL));

			split(file_names[i], ".", &file_name);
			effects->set(file_name[0], fx);

			DELETE_PTR(compiled_shader);
		}
	}
	return effects;
}

#define VALID_VAR_NAME(var, var_name, var_type) { \
	if (!var->IsValid()) { \
		if (was_var_not_found(var_name)) { \
			print("There is no the variable with name {} of type {} in {}.fx", var_name, var_type, name); \
			not_found_vars.push(var_name); \
		}  \
		return; \
	} \
}

Fx_Shader::Fx_Shader(String *shader_name, ID3DX11Effect *fx_shader)
{
	name = *shader_name;
	shader = fx_shader;
	
	D3DX11_EFFECT_DESC effect_desc;
	fx_shader->GetDesc(&effect_desc);
	technique_count = effect_desc.Techniques;
}

void Fx_Shader::attach(u32 technique_index, u32 pass_index)
{
	shader->GetTechniqueByIndex(technique_index)->GetPassByIndex(pass_index)->Apply(0, direct3d.device_context);
}

void Fx_Shader::bind(const char *var_name, Vector4 *vector)
{
	ID3DX11EffectVectorVariable * var = shader->GetVariableByName(var_name)->AsVector();
	VALID_VAR_NAME(var, var_name, "Vector");
	var->SetFloatVector(*vector);
}

void Fx_Shader::bind(const char *var_name, Matrix4 *matrix)
{
	ID3DX11EffectMatrixVariable *var = shader->GetVariableByName(var_name)->AsMatrix();
	VALID_VAR_NAME(var, var_name, "Matrix");
	var->SetMatrix(*matrix);
}

void Fx_Shader::bind(const char *var_name, ID3D11ShaderResourceView *texture)
{
	ID3DX11EffectShaderResourceVariable * var = shader->GetVariableByName(var_name)->AsShaderResource();
	VALID_VAR_NAME(var, var_name, "ShaderResource");
	var->SetResource(texture);
}

bool Fx_Shader::was_var_not_found(const char *var_name)
{
	String *var = NULL;
	FOR(not_found_vars, var) {
		if (*var == var_name) {
			return false;
		}
	}
	return true;
}

Fx_Shader::~Fx_Shader()
{
	RELEASE_COM(shader);
}

Fx_Shader_Manager::~Fx_Shader_Manager()
{
	DELETE_PTR(shaders);
}

void Fx_Shader_Manager::init()
{
	Array<String> file_names;
	String path_to_shader_dir;

	os_path.data_dir_paths.get("shader", path_to_shader_dir);

	bool success = get_file_names_from_dir(path_to_shader_dir + "\\", &file_names);
	assert(success);

	shaders = new Hash_Table<String, Fx_Shader *>(file_names.count);

	for (int i = 0; i < file_names.count; i++) {
		int file_size;

		String *path_to_shader_file = os_path.build_full_path_to_shader_file(&String(file_names[i]));
		defer(path_to_shader_file->free());

		char *compiled_shader = read_entire_file(*path_to_shader_file, "rb", &file_size);
		if (!compiled_shader)
			continue;

		ID3DX11Effect *fx = NULL;
		HR(D3DX11CreateEffectFromMemory(compiled_shader, sizeof(char) * file_size, 0, direct3d.device, &fx, NULL));

		String *name = extract_name_from_file(&file_names[i]);
		defer(name->free());
		
		Fx_Shader *fx_shader = new Fx_Shader(name, fx);
		shaders->set(*name, fx_shader);

		DELETE_PTR(compiled_shader);
	}
}

Fx_Shader *Fx_Shader_Manager::get_shader(const char *name)
{
	return shaders->operator[](name);
}