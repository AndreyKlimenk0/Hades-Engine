#include "render_pass.h"
#include "render_helpers.h"


Render_Pass::Render_Pass(void *render_context) : render_context(render_context)
{
}

void Render_Pass::init(Render_System *render_sys)
{
	render_pipeline_state.setup_default_state(render_sys);
	
	render_sys->gpu_device.create_constant_buffer(sizeof(Render_Pass::Pass_Data), &pass_data_cbuffer);
}

bool Render_Pass::validate_render_pipeline(const char *render_pass_name, Render_System *render_system)
{
	if (!render_system->shader_table.key_in_table(render_pipeline_state.shader_name)) {
		print("{}::validate_render_pipeline: Validation failed. Shader '{}' was not found.", render_pass_name, render_pipeline_state.shader_name);
		return false;
	}
	if (!render_pipeline_state.render_target_view && !render_pipeline_state.depth_stencil_view) {
		print("{}::validate_render_pipeline: Validation failed. Render target buffer and depth stencil buffer was not set.", render_pass_name);
	}
	render_pipeline_state.shader = render_system->shader_table[render_pipeline_state.shader_name];
	return true;
}

Forwar_Light_Pass::Forwar_Light_Pass(void *render_context) : Render_Pass(render_context)
{
}

bool Forwar_Light_Pass::setup_pipeline_state(Render_System *render_system)
{
	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_TRIANGLES;
	render_pipeline_state.shader_name = "forward_light.hlsl";
	render_pipeline_state.depth_stencil_view = render_system->depth_back_buffer.dsv;
	render_pipeline_state.render_target_view = render_system->back_buffer.rtv;

	return Render_Pass::validate_render_pipeline("Forwar_Light_Pass", render_system);
}

void Forwar_Light_Pass::render(Render_Pipeline *render_pipeline)
{
	Render_World *render_world = (Render_World *)render_context;

	render_pipeline->apply(&render_pipeline_state);

	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrix_struct_buffer);
	
	render_pipeline->set_vertex_shader_resource(2, render_world->triangle_meshes.mesh_struct_buffer);
	render_pipeline->set_vertex_shader_resource(4, render_world->triangle_meshes.index_struct_buffer);
	render_pipeline->set_vertex_shader_resource(5, render_world->triangle_meshes.vertex_struct_buffer);

	render_pipeline->set_pixel_shader_resource(0, render_world->default_texture.srv);
	render_pipeline->set_pixel_shader_resource(1, render_world->shadow_atlas.srv);
	render_pipeline->set_pixel_shader_resource(7, render_world->lights_struct_buffer);


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
	render_pipeline_state.depth_stencil_view = render_system->depth_back_buffer.dsv;
	render_pipeline_state.render_target_view = render_system->back_buffer.rtv;

	return Render_Pass::validate_render_pipeline("Draw_Lines_Pass", render_system);
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
	render_sys->gpu_device.create_constant_buffer(sizeof(Render_Pass::Pass_Data), &pass_data_cbuffer);
	render_sys->gpu_device.create_constant_buffer(sizeof(Shadow_Pass::Shadow_Cascade_Info), &shadow_cascade_cbuffer);
}

bool Shadow_Pass::setup_pipeline_state(Render_System *render_system)
{
	Render_World *render_world = (Render_World *)render_context;

	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_TRIANGLES;
	render_pipeline_state.shader_name = "depth_map.hlsl";
	render_pipeline_state.depth_stencil_view = render_world->shadow_atlas.dsv;

	return Render_Pass::validate_render_pipeline("Shadow_Pass", render_system);
}

void Shadow_Pass::render(Render_Pipeline *render_pipeline)
{
	Render_World *render_world = (Render_World *)render_context;

	render_world->render_sys->render_pipeline.dx11_context.Get()->ClearDepthStencilView(render_world->shadow_atlas.dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	render_pipeline->apply(&render_pipeline_state);

	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrix_struct_buffer);

	render_pipeline->set_vertex_shader_resource(2, render_world->triangle_meshes.mesh_struct_buffer);
	render_pipeline->set_vertex_shader_resource(4, render_world->triangle_meshes.index_struct_buffer);
	render_pipeline->set_vertex_shader_resource(5, render_world->triangle_meshes.vertex_struct_buffer);

	Render_Pass::Pass_Data pass_data;
	Shadow_Pass::Shadow_Cascade_Info shadow_cascade_info;

	Cascaded_Shadow_Map *cascaded_shadow_map = NULL;
	For(render_world->cascaded_shadow_maps, cascaded_shadow_map) {
		Shadow_Cascade *shadow_cascade = NULL;
		For(cascaded_shadow_map->shadow_cascades, shadow_cascade) {
			render_pipeline->set_viewport(&shadow_cascade->viewport);

			shadow_cascade_info.cascade_view_matrix = shadow_cascade->get_cascade_view_matrix();
			shadow_cascade_info.cascade_projection_matrix = shadow_cascade->get_cascade_projection_matrix();

			render_pipeline->update_constant_buffer(&shadow_cascade_cbuffer, (void *)&shadow_cascade_info);
			render_pipeline->set_vertex_shader_resource(4, shadow_cascade_cbuffer);

			Render_Entity *render_entity = NULL;
			For(render_world->render_entities, render_entity) {
				pass_data.mesh_idx = render_entity->mesh_idx;
				pass_data.world_matrix_idx = render_entity->world_matrix_idx;

				render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);

				//@Note: Must I set constant buffer after updating ?
				render_pipeline->set_vertex_shader_resource(3, pass_data_cbuffer);

				render_pipeline->draw(render_world->triangle_meshes.mesh_instances[render_entity->mesh_idx].index_count);
			}
		}
	}
}