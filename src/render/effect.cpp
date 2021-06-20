#include <assert.h>
#include <string.h>

#include "effect.h"
#include "directx.h"

#include "../sys/sys_local.h"

#include "../libs/str.h"
#include "../libs/os/file.h"
#include "../libs/os/path.h"

Fx_Shader_Manager fx_shader_manager;


#define VALID_VAR_NAME(var, var_name, var_type) { \
	if (!var->IsValid()) { \
		if (!is_var_in_not_found_vars_array(var_name)) { \
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
	assert(technique_count > technique_index);

	shader->GetTechniqueByIndex(technique_index)->GetPassByIndex(pass_index)->Apply(0, directx11.device_context);
}

void Fx_Shader::attach(const char *technique_name, u32 pass_index)
{
	assert(technique_name);

	shader->GetTechniqueByName(technique_name)->GetPassByIndex(pass_index)->Apply(0, directx11.device_context);
}

void Fx_Shader::bind(const char *var_name, int scalar)
{
	ID3DX11EffectScalarVariable * var = shader->GetVariableByName(var_name)->AsScalar();
	VALID_VAR_NAME(var, var_name, "Scalar");
	var->SetInt(scalar);
}

void Fx_Shader::bind(const char *var_name, Vector3 *vector)
{
	ID3DX11EffectVectorVariable *var = shader->GetVariableByName(var_name)->AsVector();
	VALID_VAR_NAME(var, var_name, "Vector3");
	var->SetRawValue(*vector, 0, sizeof(Vector3));
}

void Fx_Shader::bind(const char *var_name, Vector4 *vector)
{
	ID3DX11EffectVectorVariable * var = shader->GetVariableByName(var_name)->AsVector();
	VALID_VAR_NAME(var, var_name, "Vector4");
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

void Fx_Shader::bind(const char *var_name, void *struct_ptr, u32 struct_size)
{
	ID3DX11EffectVariable * var = shader->GetVariableByName(var_name);
	VALID_VAR_NAME(var, var_name, "Struct");
	var->SetRawValue(struct_ptr, 0, struct_size);
}

void Fx_Shader::bind_per_entity_vars(Entity * entity, Matrix4 & view, Matrix4 & perspective)
{
	Matrix4 world = entity->get_world_matrix();
	Matrix4 wvp_projection = world * view * perspective;

	if (entity->model->render_surface_use == RENDER_MODEL_SURFACE_USE_TEXTURE) {
		bind("texture_map", entity->model->diffuse_texture);
	} else {
		if (!entity->model->model_color) {
			Vector3 default_color = Vector3(1.0f, 0.2f, 1.0f);
			bind("model_color", &default_color);
			print("Color for model {} wasn't set, will be used default color");
		} else {
			bind("model_color", entity->model->model_color);
		}
	}
	bind("world", &world);
	bind("world_view_projection", &wvp_projection);
	bind("material", (void *)&entity->model->material, sizeof(Material));
}

void Fx_Shader::bind_per_frame_vars(Free_Camera *camera)
{
	bind("camera_position", &camera->position);
	bind("camera_direction", &camera->target);
}

bool Fx_Shader::is_var_in_not_found_vars_array(const char * var_name)
{
	String *var = NULL;
	FOR(not_found_vars, var) {
		if (*var == var_name) {
			return true;
		}
	}
	return false;
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

	//shaders = new Hash_Table<String, Fx_Shader *>(file_names.count);
	shaders = new Hash_Table<String, Fx_Shader *>(20);

	for (int i = 0; i < file_names.count; i++) {
		int file_size;

		String *path_to_shader_file = os_path.build_full_path_to_shader_file(&String(file_names[i]));
		defer(path_to_shader_file->free());

		char *compiled_shader = read_entire_file(*path_to_shader_file, "rb", &file_size);
		if (!compiled_shader)
			continue;

		ID3DX11Effect *fx = NULL;
		HR(D3DX11CreateEffectFromMemory(compiled_shader, sizeof(char) * file_size, 0, directx11.device, &fx, NULL));

		String *name = extract_name_from_file(&file_names[i]);
		defer(name->free());
		
		Fx_Shader *fx_shader = new Fx_Shader(name, fx);
		shaders->set(*name, fx_shader);

		DELETE_PTR(compiled_shader);
	}
}

Fx_Shader *Fx_Shader_Manager::get_shader(const char *name)
{
	return (*shaders)[name];
}


void bind_light_entities(Fx_Shader *forward_light_shader, Array<Light *> *lights)
{
	if (lights->is_empty())
		return;

	Array<Fx_Light> fx_lights;

	Light *light = NULL;
	FOR((*lights), light) {
		switch (light->light_type) {
			case DIRECTIONAL_LIGHT_TYPE: {
				Fx_Light fx_light_struct;
				fx_light_struct.position = light->position;
				fx_light_struct.directon = light->direction;
				fx_light_struct.color = light->color;
				fx_light_struct.light_type = light->light_type;
				fx_lights.push(fx_light_struct);
				break;
			}
			case POINT_LIGHT_TYPE: {
				Fx_Light fx_light_struct;
				fx_light_struct.position = light->position;
				fx_light_struct.color = light->color;
				fx_light_struct.light_type = light->light_type;
				fx_light_struct.range = light->range;
				fx_lights.push(fx_light_struct);
				break;
			}
			case SPOT_LIGHT_TYPE: {
				Fx_Light fx_light_struct;
				fx_light_struct.position = light->position;
				fx_light_struct.directon = light->direction;
				fx_light_struct.color = light->color;
				fx_light_struct.light_type = light->light_type;
				fx_light_struct.radius = light->radius;
				fx_lights.push(fx_light_struct);
				break;
			}
		}
	}

	forward_light_shader->bind("lights", (void *)&fx_lights.items[0], sizeof(Fx_Light) * fx_lights.count);
	forward_light_shader->bind("light_count", fx_lights.count);
}