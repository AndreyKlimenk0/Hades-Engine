#include <assert.h>
#include <string.h>

#include "effect.h"
#include "directx.h"

#include "../sys/sys_local.h"

#include "../libs/str.h"
#include "../libs/os/file.h"
#include "../libs/os/path.h"

#include "render_system.h"

Fx_Shader_Manager fx_shader_manager;

Fx_Shader::Fx_Shader(String *shader_name, ID3DX11Effect *fx_shader)
{
	name = *shader_name;
	shader = fx_shader;
	
	D3DX11_EFFECT_DESC effect_desc;
	fx_shader->GetDesc(&effect_desc);
	technique_count = effect_desc.Techniques;
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

		String name;
		extract_file_name(file_names[i].to_str(), name);
		
		Fx_Shader *fx_shader = new Fx_Shader(&name, fx);
		shaders->set(name, fx_shader);

		DELETE_PTR(compiled_shader);
	}
}

Fx_Shader *Fx_Shader_Manager::get_shader(const char *name)
{
	return (*shaders)[name];
}

void Fx_Shader::bind_entity(Entity * entity, Matrix4 &view, Matrix4 &perspective, Render_Mesh *r)
{
	Matrix4 world = entity->get_world_matrix();
	//Matrix4 world = r->position * r->orientation * r->scale;
	Matrix4 wvp_projection = world * view * perspective;
	//Matrix4 w;
	//bind("world", &r->transform);
	bind("world", &world);
	bind("world_view_projection", &wvp_projection);

	if (entity->type != ENTITY_TYPE_LIGHT) {
		bind("texture_map", r->diffuse_texture->shader_resource);
		bind("material", (void *)&r->material, sizeof(Material));

	}
}

void bind_light(Fx_Shader *forward_light_shader, Array<Light *> *lights)
{
	if (lights->is_empty())
		return;

	Array<Fx_Light> fx_lights;

	Light *light = NULL;
	For((*lights), light) {
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

void bind(Fx_Shader *fx_shader, Entity *entity)
{
	Matrix4 world = entity->get_world_matrix();
	Matrix4 screen_projection = world * render_sys.view_matrix * render_sys.view_info->perspective_matrix;
	fx_shader->bind("world", &world);
	fx_shader->bind("world_view_projection", &screen_projection);
}

void bind(Fx_Shader *fx_shader, Render_Mesh *render_mesh)
{
	fx_shader->bind("texture_map", render_mesh->diffuse_texture->shader_resource);
	fx_shader->bind("material", (void *)&render_mesh->material, sizeof(Material));
}
