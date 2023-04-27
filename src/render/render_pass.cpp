#include "render_pass.h"
#include "render_helpers.h"


Render_Pass::Render_Pass(void *render_context) : render_context(render_context)
{
}

void Render_Pass::init(Render_System *render_sys)
{
	render_sys->gpu_device.create_constant_buffer(sizeof(Render_Pass::Pass_Data), &pass_data_cbuffer);
}

bool Render_Pass::setup_pipeline_state(const char *render_pass_name, Render_System *render_system)
{
	if (!render_pipeline_state.setup(render_system)) {
		print("{}::setup_pipeline_state: Failed to init Pipeline_state", render_pass_name);
		return false;
	}
	return true;
}

Forwar_Light_Pass::Forwar_Light_Pass(void *render_context) : Render_Pass(render_context)
{
}

bool Forwar_Light_Pass::setup_pipeline_state(Render_System *render_system)
{
	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_TRIANGLES;
	render_pipeline_state.shader_name = "forward_light.hlsl";
	render_pipeline_state.depth_stencil_buffer = &render_system->render_targes.back_buffer_depth;
	render_pipeline_state.render_target = &render_system->render_targes.back_buffer;

	return Render_Pass::setup_pipeline_state("Forwar_Light_Pass", render_system);
}

void Forwar_Light_Pass::render(Render_Pipeline *render_pipeline)
{
	Render_World *render_world = (Render_World *)render_context;

	render_pipeline->apply(&render_pipeline_state);

	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrix_struct_buffer);
	
	render_pipeline->set_vertex_shader_resource(2, render_world->triangle_meshes.mesh_struct_buffer);
	render_pipeline->set_vertex_shader_resource(4, render_world->triangle_meshes.index_struct_buffer);
	render_pipeline->set_vertex_shader_resource(5, render_world->triangle_meshes.vertex_struct_buffer);

	render_pipeline->set_pixel_shader_sampler(Render_Pipeline_States::default_sampler_state);
	render_pipeline->set_pixel_shader_resource(0, render_world->default_texture.view);
	render_pipeline->set_pixel_shader_resource(1, render_world->shadow_atlas.view);
	render_pipeline->set_pixel_shader_resource(7, render_world->lights_struct_buffer);
	render_pipeline->set_pixel_shader_resource(8, render_world->light_view_matrices_struct_buffer);
	render_pipeline->set_pixel_shader_resource(6, render_world->shadow_maps_struct_buffer);

	render_pipeline->set_pixel_shader_resource(2, render_world->light_projections_cbuffer);

	Render_Entity *render_entity = NULL;
	Forwar_Light_Pass::Pass_Data pass_data;
	
	For(render_world->render_entities, render_entity) {
		
		pass_data.mesh_idx = render_entity->mesh_idx;
		pass_data.world_matrix_idx = render_entity->world_matrix_idx;

		render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);
		//@Note: Must I set constant buffer after updating ?
		render_pipeline->set_vertex_shader_resource(2, pass_data_cbuffer);

		render_pipeline->draw(render_world->triangle_meshes.mesh_instances[render_entity->mesh_idx].index_count);
	}
}

Draw_Lines_Pass::Draw_Lines_Pass(void *render_context) : Render_Pass(render_context)
{
}

bool Draw_Lines_Pass::setup_pipeline_state(Render_System *render_system)
{
	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_LINES;
	render_pipeline_state.shader_name = "draw_lines.hlsl";
	render_pipeline_state.depth_stencil_buffer = &render_system->render_targes.back_buffer_depth;
	render_pipeline_state.render_target = &render_system->render_targes.back_buffer;

	return Render_Pass::setup_pipeline_state("Draw_Lines_Pass", render_system);
}

void Draw_Lines_Pass::render(Render_Pipeline *render_pipeline)
{
	Render_World *render_world = (Render_World *)render_context;

	render_pipeline->apply(&render_pipeline_state);

	render_pipeline->set_vertex_shader_resource(0, render_world->line_meshes.vertex_struct_buffer);
	render_pipeline->set_vertex_shader_resource(1, render_world->line_meshes.index_struct_buffer);
	render_pipeline->set_vertex_shader_resource(2, render_world->line_meshes.mesh_struct_buffer);
	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrix_struct_buffer);

	Render_Entity *render_entity = NULL;
	Forwar_Light_Pass::Pass_Data pass_data;

	For(render_world->bounding_box_entities, render_entity) {

		pass_data.mesh_idx = render_entity->mesh_idx;
		pass_data.world_matrix_idx = render_entity->world_matrix_idx;

		render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);
		//@Note: Must I set constant buffer after updating ?
		render_pipeline->set_vertex_shader_resource(2, pass_data_cbuffer);

		render_pipeline->draw(render_world->line_meshes.mesh_instances[render_entity->mesh_idx].index_count);
	}
}

Shadow_Pass::Shadow_Pass(void *render_context) : Render_Pass(render_context)
{
}

void Shadow_Pass::init(Render_System *render_sys)
{
	render_sys->gpu_device.create_constant_buffer(sizeof(Shadow_Pass::Pass_Data), &pass_data_cbuffer);
}

bool Shadow_Pass::setup_pipeline_state(Render_System *render_system)
{
	Render_World *render_world = (Render_World *)render_context;

	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_TRIANGLES;
	render_pipeline_state.shader_name = "depth_map.hlsl";
	render_pipeline_state.depth_stencil_buffer = &render_world->temp_shadow_storage;
	render_pipeline_state.render_target = NULL;

	return Render_Pass::setup_pipeline_state("Shadow_Pass", render_system);
}

void Shadow_Pass::render(Render_Pipeline *render_pipeline)
{
	Render_World *render_world = (Render_World *)render_context;

	fill_texture_with_value((void *)&DEFAULT_DEPTH_VALUE, &render_world->temp_shadow_storage.texture);

	render_pipeline->apply(&render_pipeline_state);

	render_pipeline->set_vertex_shader_resource(2, render_world->light_projections_cbuffer);

	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrix_struct_buffer);
	render_pipeline->set_vertex_shader_resource(6, render_world->light_view_matrices_struct_buffer);

	render_pipeline->set_vertex_shader_resource(2, render_world->triangle_meshes.mesh_struct_buffer);
	render_pipeline->set_vertex_shader_resource(4, render_world->triangle_meshes.index_struct_buffer);
	render_pipeline->set_vertex_shader_resource(5, render_world->triangle_meshes.vertex_struct_buffer);

	Render_Entity *render_entity = NULL;
	Shadow_Pass::Pass_Data pass_data;

	Shadow_Map *shadow_map = NULL;
	For(render_world->shadow_maps, shadow_map) {
		
		pass_data.light_view_matrix_idx = shadow_map->light_view_matrix_idx;
		
		For(render_world->render_entities, render_entity) {

			pass_data.mesh_idx = render_entity->mesh_idx;
			pass_data.world_matrix_idx = render_entity->world_matrix_idx;

			render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);

			//@Note: Must I set constant buffer after updating ?
			render_pipeline->set_vertex_shader_resource(3, pass_data_cbuffer);

			render_pipeline->draw(render_world->triangle_meshes.mesh_instances[render_entity->mesh_idx].index_count);
		}
		//render_pipeline->copy_subresource(render_world->shadow_atlas, shadow_map->coordinates_in_atlas.x, shadow_map->coordinates_in_atlas.y, render_world->temp_shadow_storage.texture);
		render_pipeline->copy_subresource(render_world->shadow_atlas, 0, 0, render_world->temp_shadow_storage.texture);
	}
}
