#include <assert.h>
#include "directx.h"
#include "render_system.h"
#include "../win32/win_local.h"


Render_System render_sys;


enum Engine_Mode {
	GAME_MODE,
	EDITOR_MODE
};

Render_System::~Render_System()
{
	shutdown();
}

void Render_System::init(Render_API render_api, View_Info *view_info)
{
	this->view_info = view_info;

	if (render_api == DIRECTX11) {
		render = new DirectX11();
		render->init();
	} else {
		assert(true);
	}
}

void Render_System::resize()
{
	if (!render) {
		print("Resizing of Render System can be execute because Render is not initialized");
		return;
	}
	render->resize();
	view_info->get_perspective_matrix();
}

void Render_System::shutdown()
{
	if (render) {
		render->shutdown();
	}
}

void Render_System::render_frame()
{
	render->begin_draw();
	view_matrix = free_camera->get_view_matrix();
	
	draw_world_entities(&current_render_world->entity_manager);
	
	((DirectX11 *)render)->direct2d.test_draw();
	
	render->end_draw();
}

void Render_System::draw_world_entities(Entity_Manager *entity_manager)
{
	Fx_Shader *light = fx_shader_manager.get_shader("forward_light");

	bind_light_entities(light, &entity_manager->lights);

	light->bind_per_frame_vars(free_camera);

	Entity * entity = NULL;
	FOR(entity_manager->entities, entity) {
		light->bind_per_entity_vars(entity, view_matrix, view_info->perspective_matrix);

		if (entity->type == ENTITY_TYPE_LIGHT) {
			int light_model_index = 0;
			if (!entity->model) {
				continue;
			}
			light->bind("light_model_index", light_model_index++);
			light->attach("render_light_model");
			render->draw_mesh(&entity->model->mesh);
			continue;
		}

		if (entity->model->render_surface_use == RENDER_MODEL_SURFACE_USE_TEXTURE) {
			light->attach("render_model_use_texture");
		} else {
			light->attach("render_model_use_color");
		}
		render->draw_mesh(&entity->model->mesh);
		render->draw_shadow(entity, light, entity_manager->lights[0], view_matrix, view_info->perspective_matrix);
	}
}


View_Info *make_view_info(float near_plane, float far_plane)
{
	View_Info *view_info = new View_Info();
	view_info->window_width = win32.window_width;
	view_info->window_height = win32.window_height;
	view_info->window_ratio = (float)view_info->window_width / (float)view_info->window_height;
	view_info->fov_y_ratio = XMConvertToRadians(45);
	view_info->near_plane = near_plane;
	view_info->far_plane = far_plane;
	view_info->get_perspective_matrix();
	return view_info;
}
