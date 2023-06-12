#include "render_pass.h"
#include "render_helpers.h"
#include "render_world.h"
#include "render_system.h"

bool Render_Pass::init(void *_render_context, Render_System *render_sys)
{
	render_context = _render_context;
	render_pipeline_state.setup_default_state(render_sys);	
	if (setup_pipeline_state(render_sys)) {
		initialized = true;
		return true;
	}
	return false;
}

//@Note: Just enough to pass the shader table instaed of Render_System
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

bool Forwar_Light_Pass::init(void *_render_context, Render_System *render_sys)
{
	name = "Forward_Light";
	render_sys->gpu_device.create_constant_buffer(sizeof(Render_Pass::Pass_Data), &pass_data_cbuffer);
	return Render_Pass::init(_render_context, render_sys);
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
	assert(initialized);

	Render_World *render_world = (Render_World *)render_context;

	render_pipeline->apply(&render_pipeline_state);

	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrices_struct_buffer);
	
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

bool Draw_Lines_Pass::init(void *_render_context, Render_System *render_sys)
{
	name = "Draw_Lines";
	render_sys->gpu_device.create_constant_buffer(sizeof(Render_Pass::Pass_Data), &pass_data_cbuffer);
	return Render_Pass::init(_render_context, render_sys);
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
	assert(initialized);

	Render_World *render_world = (Render_World *)render_context;

	render_pipeline->apply(&render_pipeline_state);

	render_pipeline->set_vertex_shader_resource(0, render_world->line_meshes.vertex_struct_buffer);
	render_pipeline->set_vertex_shader_resource(1, render_world->line_meshes.index_struct_buffer);
	render_pipeline->set_vertex_shader_resource(2, render_world->line_meshes.mesh_struct_buffer);
	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrices_struct_buffer);

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

bool Shadows_Pass::init(void *_render_context, Render_System *render_sys)
{
	name = "Shadows";
	render_sys->gpu_device.create_constant_buffer(sizeof(Shadows_Pass::Pass_Data), &pass_data_cbuffer);
	return Render_Pass::init(_render_context, render_sys);
}

bool Shadows_Pass::setup_pipeline_state(Render_System *render_system)
{
	Render_World *render_world = (Render_World *)render_context;

	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_TRIANGLES;
	render_pipeline_state.shader_name = "depth_map.hlsl";
	render_pipeline_state.depth_stencil_view = render_world->shadow_atlas.dsv;

	return Render_Pass::validate_render_pipeline("Shadow_Pass", render_system);
}

void Shadows_Pass::render(Render_Pipeline *render_pipeline)
{
	assert(initialized);

	Render_World *render_world = (Render_World *)render_context;

	render_world->render_sys->render_pipeline.dx11_context.Get()->ClearDepthStencilView(render_world->shadow_atlas.dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	render_pipeline->apply(&render_pipeline_state);

	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrices_struct_buffer);

	render_pipeline->set_vertex_shader_resource(2, render_world->triangle_meshes.mesh_struct_buffer);
	render_pipeline->set_vertex_shader_resource(4, render_world->triangle_meshes.index_struct_buffer);
	render_pipeline->set_vertex_shader_resource(5, render_world->triangle_meshes.vertex_struct_buffer);
	render_pipeline->set_vertex_shader_resource(6, render_world->cascaded_view_projection_matrices_sb);

	Shadows_Pass::Pass_Data pass_data;

	Cascaded_Shadow_Map *cascaded_shadow_map = NULL;
	For(render_world->cascaded_shadow_maps, cascaded_shadow_map) {
		Cascaded_Shadow *cascaded_shadow = NULL;
		For(cascaded_shadow_map->cascaded_shadows, cascaded_shadow) {
			render_pipeline->set_viewport(&cascaded_shadow->viewport);

			Render_Entity *render_entity = NULL;
			For(render_world->render_entities, render_entity) {
				pass_data.mesh_idx = render_entity->mesh_idx;
				pass_data.world_matrix_idx = render_entity->world_matrix_idx;
				pass_data.cascade_view_projection_matrix_idx = cascaded_shadow->view_projection_matrix_index;

				render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);

				//@Note: Must I set constant buffer after updating ?
				render_pipeline->set_vertex_shader_resource(3, pass_data_cbuffer);

				render_pipeline->draw(render_world->triangle_meshes.mesh_instances[render_entity->mesh_idx].index_count);
			}
		}
	}
}

bool Debug_Cascade_Shadows_Pass::init(void *_render_context, Render_System *render_sys)
{
	name = "Debug_Cascade_Shadows";
	render_sys->gpu_device.create_constant_buffer(sizeof(Render_Pass::Pass_Data), &pass_data_cbuffer);
	render_sys->gpu_device.create_constant_buffer(sizeof(Shadow_Atlas_Info), &shadow_atlas_info_cbuffer);
	return Render_Pass::init(_render_context, render_sys);
}

bool Debug_Cascade_Shadows_Pass::setup_pipeline_state(Render_System *render_system)
{
	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_TRIANGLES;
	render_pipeline_state.shader_name = "debug_cascaded_shadows.hlsl";
	render_pipeline_state.depth_stencil_view = render_system->depth_back_buffer.dsv;
	render_pipeline_state.render_target_view = render_system->back_buffer.rtv;
	return validate_render_pipeline("Debug_Cascade_Shadows_Pass", render_system);
}

void Debug_Cascade_Shadows_Pass::render(Render_Pipeline *render_pipeline)
{
	assert(initialized);

	Render_World *render_world = (Render_World *)render_context;

	render_pipeline->apply(&render_pipeline_state);

	Shadow_Atlas_Info shadow_atlas_info;
	shadow_atlas_info.shadow_atlas_width = SHADOW_ATLAS_WIDTH;
	shadow_atlas_info.shadow_atlas_height = SHADOW_ATLAS_HEIGHT;
	shadow_atlas_info.shadow_cascade_width = CASCADE_WIDTH;
	shadow_atlas_info.shadow_cascade_height = CASCADE_HEIGHT;
	shadow_atlas_info.cascaded_shadow_count = render_world->cascaded_shadow_count;
	render_pipeline->update_constant_buffer(&shadow_atlas_info_cbuffer, (void *)&shadow_atlas_info);

	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrices_struct_buffer);

	render_pipeline->set_vertex_shader_resource(2, render_world->triangle_meshes.mesh_struct_buffer);
	render_pipeline->set_vertex_shader_resource(4, render_world->triangle_meshes.index_struct_buffer);
	render_pipeline->set_vertex_shader_resource(5, render_world->triangle_meshes.vertex_struct_buffer);

	render_pipeline->set_pixel_shader_resource(4, shadow_atlas_info_cbuffer);
	render_pipeline->set_pixel_shader_resource(1, render_world->shadow_atlas.srv);
	render_pipeline->set_pixel_shader_resource(7, render_world->lights_struct_buffer);
	render_pipeline->set_pixel_shader_resource(8, render_world->cascaded_view_projection_matrices_sb);

	Render_Entity *render_entity = NULL;
	Debug_Cascade_Shadows_Pass::Pass_Data pass_data;

	For(render_world->render_entities, render_entity) {

		pass_data.mesh_idx = render_entity->mesh_idx;
		pass_data.world_matrix_idx = render_entity->world_matrix_idx;

		render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);
		//@Note: Must I set constant buffer after updating ?
		render_pipeline->set_vertex_shader_resource(2, pass_data_cbuffer);

		render_pipeline->draw(render_world->triangle_meshes.mesh_instances[render_entity->mesh_idx].index_count);
	}
}
