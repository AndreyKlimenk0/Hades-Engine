#include <assert.h>

#include "hlsl.h"
#include "render_pass.h"
#include "render_world.h"

const u32 SKIP_RENDER_TARGET_VIEW_VALIDATION = 0x1;
const u32 SKIP_VIEWPORT_VALIDATION           = 0x2;
const u32 SKIP_PIXEL_SHADER_VALIDATION       = 0x4;

inline void setup_default_render_pipeline_state(Render_Pipeline_State *render_pipeline_state, Render_Pipeline_States *render_pipeline_states)
{
	render_pipeline_state->blend_state = render_pipeline_states->default_blend_state;
	render_pipeline_state->depth_stencil_state = render_pipeline_states->default_depth_stencil_state;
	render_pipeline_state->rasterizer_state = render_pipeline_states->default_rasterizer_state;
}

inline bool validate_render_pass(Render_Pass *render_pass, u32 validation_flags = 0)
{
	assert(render_pass);

	bool result = true;
	if (render_pass->name.is_empty()) {
		print("validate_render_pass: A render pass doesn't have a name. The render pass is not valid.");
		result = false;
	}
	if (!render_pass->render_pipeline_state.blend_state) {
		print("validate_render_pass: Render Pass '{}' is not valid. For a render pipeline was not assigned a blend state.", &render_pass->name);
		result = false;
	}
	if (!render_pass->render_pipeline_state.depth_stencil_state) {
		print("validate_render_pass: Render Pass '{}' is not valid. For a render pipeline was not assigned a depth stencil state.", &render_pass->name);
		result = false;
	} 
	if (((render_pass->render_pipeline_state.viewport.width == 0) || (render_pass->render_pipeline_state.viewport.height == 0)) && !(validation_flags & SKIP_VIEWPORT_VALIDATION)) {
		print("validate_render_pass: Render Pass '{}' is not valid.. A render pipeline has a wrong setup viewport.", &render_pass->name);
		result = false;
	} 
	if (!render_pass->render_pipeline_state.depth_stencil_view) {
		print("validate_render_pass: Render Pass '{}' is not valid. For a render pipeline was not assigned a depth stencil view.", &render_pass->name);
		result = false;
	} if ((!render_pass->render_pipeline_state.render_target_view) && !(validation_flags & SKIP_RENDER_TARGET_VIEW_VALIDATION)) {
		print("validate_render_pass: Render Pass '{}' is not valid. For a render pipeline was not assigned a render target view.", &render_pass->name);
		result = false;
	}
	if (render_pass->render_pipeline_state.shader) {
		Extend_Shader *shader = (Extend_Shader *)render_pass->render_pipeline_state.shader;
		if (validation_flags & SKIP_PIXEL_SHADER_VALIDATION) {
			if (!is_valid(shader, VALIDATE_RENDERING_SHADER)) {
				print("validate_render_pass: Render Pass '{}' is not valid. {} was not initialized correctly.", &render_pass->name, shader->name);
				result = false;
			}
		} else {
			if (!is_valid(shader, VALIDATE_VERTEX_SHADER)) {
				print("validate_render_pass: Render Pass '{}' is not valid. {} was not initialized correctly.", &render_pass->name, shader->name);
				result = false;
			}
		}
	} else {
		print("validate_render_pass: Render Pass '{}' is not valid. For a render pipeline was not assigned a shader.", &render_pass->name);
		result = false;
	}
	if (result) {
		render_pass->is_valid = true;
	}
	return result;
}

void Render_Pass::init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states)
{
	assert(gpu_device);
	assert(_render_pipeline_states);

	render_pipeline_states = _render_pipeline_states;
	setup_default_render_pipeline_state(&render_pipeline_state, render_pipeline_states);
}

void Forwar_Light_Pass::init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states)
{
	name = "Forward_Light";
	gpu_device->create_constant_buffer(sizeof(Render_Pass::Pass_Data), &pass_data_cbuffer);
	gpu_device->create_constant_buffer(sizeof(CB_Shadow_Atlas_Info), &shadow_atlas_info_cbuffer);
	Render_Pass::init(gpu_device, _render_pipeline_states);
}

bool Forwar_Light_Pass::setup_render_pipeline(Shader_Manager *shader_manager, const Depth_Stencil_View &depth_stencil_view, const Render_Target_View &render_target_view, Viewport *viewport)
{
	assert(shader_manager);
	assert(viewport);

	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_TRIANGLES;
	render_pipeline_state.shader = GET_SHADER(shader_manager, forward_light);
	render_pipeline_state.depth_stencil_view = depth_stencil_view;
	render_pipeline_state.render_target_view = render_target_view;
	render_pipeline_state.viewport = *viewport;
	return validate_render_pass(this);
}

void Forwar_Light_Pass::render(Render_World *render_world, Render_Pipeline *render_pipeline)
{
	assert(render_world);
	assert(render_pipeline);
	assert(is_valid);

	render_pipeline->apply(&render_pipeline_state);

	CB_Shadow_Atlas_Info shadow_atlas_info;
	shadow_atlas_info.shadow_atlas_size = SHADOW_ATLAS_SIZE;
	shadow_atlas_info.shadow_cascade_size = CASCADE_SIZE;
	shadow_atlas_info.jittering_sampling_tile_size = render_world->jittering_tile_size;
	shadow_atlas_info.jittering_sampling_filter_size = render_world->jittering_filter_size;
	shadow_atlas_info.jittering_sampling_scaling = render_world->jittering_scaling;

	render_pipeline->update_constant_buffer(&shadow_atlas_info_cbuffer, (void *)&shadow_atlas_info);

	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrices_struct_buffer);
	
	render_pipeline->set_vertex_shader_resource(2, render_world->triangle_meshes.mesh_struct_buffer);
	render_pipeline->set_vertex_shader_resource(4, render_world->triangle_meshes.index_struct_buffer);
	render_pipeline->set_vertex_shader_resource(5, render_world->triangle_meshes.vertex_struct_buffer);

	render_pipeline->set_pixel_shader_resource(CB_SHADOW_ATLAS_INFO_REGISTER, shadow_atlas_info_cbuffer);
	render_pipeline->set_pixel_shader_resource(0, render_world->default_texture.srv);
	render_pipeline->set_pixel_shader_resource(SHADOW_ATLAS_TEXTURE_REGISTER, render_world->shadow_atlas.srv);
	render_pipeline->set_pixel_shader_resource(7, render_world->lights_struct_buffer);
	render_pipeline->set_pixel_shader_resource(8, render_world->cascaded_view_projection_matrices_sb);
	render_pipeline->set_pixel_shader_resource(9, render_world->cascaded_shadows_info_sb);
	render_pipeline->set_pixel_shader_resource(JITTERING_SAMPLES_TEXTURE_REGISTER, render_world->jittering_samples.srv);
	render_pipeline->set_pixel_shader_sampler(LINEAR_SAMPLING_REGISTER, render_pipeline_states->linear_sampling);

	Render_Entity *render_entity = NULL;
	Forwar_Light_Pass::Pass_Data pass_data;

	For(render_world->forward_rendering_entities, render_entity) {
		pass_data.mesh_idx = render_entity->mesh_idx;
		pass_data.world_matrix_idx = render_entity->world_matrix_idx;

		render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);
		render_pipeline->set_vertex_shader_resource(CB_PASS_DATA_REGISTER, pass_data_cbuffer);

		render_pipeline->set_pixel_shader_resource(10, render_world->render_entity_texture_storage.textures[render_entity->ambient_texture_idx].srv);
		render_pipeline->set_pixel_shader_resource(11, render_world->render_entity_texture_storage.textures[render_entity->normal_texture_idx].srv);
		render_pipeline->set_pixel_shader_resource(12, render_world->render_entity_texture_storage.textures[render_entity->diffuse_texture_idx].srv);
		render_pipeline->set_pixel_shader_resource(13, render_world->render_entity_texture_storage.textures[render_entity->specular_texture_idx].srv);
		render_pipeline->set_pixel_shader_resource(14, render_world->render_entity_texture_storage.textures[render_entity->displacement_texture_idx].srv);

		render_pipeline->draw(render_world->triangle_meshes.mesh_instances[render_entity->mesh_idx].index_count);
	}
	// Reset shadow atlas in order to get rid of warnings (Resource being set to OM DepthStencil is still bound on input!, Forcing PS shader resource slot 1 to NULL) from directx 11.
	render_pipeline->reset_pixel_shader_resource(SHADOW_ATLAS_TEXTURE_REGISTER);
}

void Draw_Lines_Pass::init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states)
{
	name = "Draw_Lines";
	gpu_device->create_constant_buffer(sizeof(Render_Pass::Pass_Data), &pass_data_cbuffer);
	Render_Pass::init(gpu_device, _render_pipeline_states);
}

bool Draw_Lines_Pass::setup_render_pipeline(Shader_Manager *shader_manager, const Depth_Stencil_View &depth_stencil_view, const Render_Target_View &render_target_view, Viewport *viewport)
{
	assert(shader_manager);
	assert(viewport);

	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_LINES;
	render_pipeline_state.shader = GET_SHADER(shader_manager, draw_lines);
	render_pipeline_state.depth_stencil_view = depth_stencil_view;
	render_pipeline_state.render_target_view = render_target_view;
	render_pipeline_state.viewport = *viewport;
	return validate_render_pass(this);
}

void Draw_Lines_Pass::render(Render_World *render_world, Render_Pipeline *render_pipeline)
{
	assert(render_world);
	assert(render_pipeline);
	assert(is_valid);

	render_pipeline->apply(&render_pipeline_state);

	render_pipeline->set_vertex_shader_resource(0, render_world->line_meshes.vertex_struct_buffer);
	render_pipeline->set_vertex_shader_resource(1, render_world->line_meshes.index_struct_buffer);
	render_pipeline->set_vertex_shader_resource(2, render_world->line_meshes.mesh_struct_buffer);
	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrices_struct_buffer);

	Render_Entity *render_entity = NULL;
	Forwar_Light_Pass::Pass_Data pass_data;

	For(render_world->line_rendering_entities, render_entity) {
		pass_data.mesh_idx = render_entity->mesh_idx;
		pass_data.world_matrix_idx = render_entity->world_matrix_idx;

		render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);
		render_pipeline->set_vertex_shader_resource(CB_PASS_DATA_REGISTER, pass_data_cbuffer);

		render_pipeline->draw(render_world->line_meshes.mesh_instances[render_entity->mesh_idx].index_count);
	}
}

void Shadows_Pass::init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states)
{
	name = "Shadows";
	gpu_device->create_constant_buffer(sizeof(Shadows_Pass::Pass_Data), &pass_data_cbuffer);
	Render_Pass::init(gpu_device, _render_pipeline_states);
}

bool Shadows_Pass::setup_render_pipeline(Shader_Manager *shader_manager, const Depth_Stencil_View &depth_stencil_view)
{
	assert(shader_manager);

	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_TRIANGLES;
	render_pipeline_state.shader = GET_SHADER(shader_manager, depth_map);
	render_pipeline_state.depth_stencil_view = depth_stencil_view;
	render_pipeline_state.render_target_view = nullptr;
	return validate_render_pass(this, SKIP_VIEWPORT_VALIDATION | SKIP_RENDER_TARGET_VIEW_VALIDATION);
}

void Shadows_Pass::render(Render_World *render_world, Render_Pipeline *render_pipeline)
{
	assert(render_world);
	assert(render_pipeline);
	assert(is_valid);

	//@Note: this code can be moved to render world
	render_pipeline->clear_depth_stencil_view(render_world->shadow_atlas.dsv);

	render_pipeline->apply(&render_pipeline_state);

	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrices_struct_buffer);

	render_pipeline->set_vertex_shader_resource(2, render_world->triangle_meshes.mesh_struct_buffer);
	render_pipeline->set_vertex_shader_resource(4, render_world->triangle_meshes.index_struct_buffer);
	render_pipeline->set_vertex_shader_resource(5, render_world->triangle_meshes.vertex_struct_buffer);
	render_pipeline->set_vertex_shader_resource(6, render_world->cascaded_view_projection_matrices_sb);

	Shadows_Pass::Pass_Data pass_data;

	Cascaded_Shadows *cascaded_shadows = NULL;
	For(render_world->cascaded_shadows_list, cascaded_shadows) {
		Cascaded_Shadow_Map *cascaded_shadow_map = NULL;
		For(cascaded_shadows->cascaded_shadow_maps, cascaded_shadow_map) {
			render_pipeline->set_viewport(&cascaded_shadow_map->viewport);

			Render_Entity *render_entity = NULL;
			For(render_world->forward_rendering_entities, render_entity) {
				pass_data.mesh_idx = render_entity->mesh_idx;
				pass_data.world_matrix_idx = render_entity->world_matrix_idx;
				pass_data.cascade_view_projection_matrix_idx = cascaded_shadow_map->view_projection_matrix_index;

				render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);
				render_pipeline->set_vertex_shader_resource(CB_PASS_DATA_REGISTER, pass_data_cbuffer);

				render_pipeline->draw(render_world->triangle_meshes.mesh_instances[render_entity->mesh_idx].index_count);
			}
		}
	}
}

void Debug_Cascade_Shadows_Pass::init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states)
{
	name = "Debug_Cascade_Shadows";
	gpu_device->create_constant_buffer(sizeof(Render_Pass::Pass_Data), &pass_data_cbuffer);
	gpu_device->create_constant_buffer(sizeof(CB_Shadow_Atlas_Info), &shadow_atlas_info_cbuffer);
	return Render_Pass::init(gpu_device, _render_pipeline_states);
}

bool Debug_Cascade_Shadows_Pass::setup_render_pipeline(Shader_Manager *shader_manager, const Depth_Stencil_View &depth_stencil_view, const Render_Target_View &render_target_view, Viewport *viewport)
{
	assert(shader_manager);
	assert(viewport);

	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_TRIANGLES;
	render_pipeline_state.shader = GET_SHADER(shader_manager, debug_cascaded_shadows);
	render_pipeline_state.depth_stencil_view = depth_stencil_view;
	render_pipeline_state.render_target_view = render_target_view;
	render_pipeline_state.viewport = *viewport;
	return validate_render_pass(this);
}

void Debug_Cascade_Shadows_Pass::render(Render_World *render_world, Render_Pipeline *render_pipeline)
{
	assert(render_world);
	assert(render_pipeline);
	assert(is_valid);

	render_pipeline->apply(&render_pipeline_state);

	CB_Shadow_Atlas_Info shadow_atlas_info;
	shadow_atlas_info.shadow_atlas_size = SHADOW_ATLAS_SIZE;
	shadow_atlas_info.shadow_cascade_size = CASCADE_SIZE;
	shadow_atlas_info.jittering_sampling_tile_size = render_world->jittering_tile_size;
	shadow_atlas_info.jittering_sampling_filter_size = render_world->jittering_filter_size;
	shadow_atlas_info.jittering_sampling_scaling = render_world->jittering_scaling;
	
	render_pipeline->update_constant_buffer(&shadow_atlas_info_cbuffer, (void *)&shadow_atlas_info);

	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrices_struct_buffer);

	render_pipeline->set_vertex_shader_resource(2, render_world->triangle_meshes.mesh_struct_buffer);
	render_pipeline->set_vertex_shader_resource(4, render_world->triangle_meshes.index_struct_buffer);
	render_pipeline->set_vertex_shader_resource(5, render_world->triangle_meshes.vertex_struct_buffer);

	render_pipeline->set_pixel_shader_resource(CB_SHADOW_ATLAS_INFO_REGISTER, shadow_atlas_info_cbuffer);
	render_pipeline->set_pixel_shader_resource(SHADOW_ATLAS_TEXTURE_REGISTER, render_world->shadow_atlas.srv);
	render_pipeline->set_pixel_shader_resource(7, render_world->lights_struct_buffer);
	render_pipeline->set_pixel_shader_resource(8, render_world->cascaded_view_projection_matrices_sb);
	render_pipeline->set_pixel_shader_resource(9, render_world->cascaded_shadows_info_sb);
	render_pipeline->set_pixel_shader_resource(JITTERING_SAMPLES_TEXTURE_REGISTER, render_world->jittering_samples.srv);
	render_pipeline->set_pixel_shader_sampler(POINT_SAMPLING_REGISTER, render_pipeline_states->point_sampling);


	Render_Entity *render_entity = NULL;
	Debug_Cascade_Shadows_Pass::Pass_Data pass_data;

	For(render_world->forward_rendering_entities, render_entity) {
		pass_data.mesh_idx = render_entity->mesh_idx;
		pass_data.world_matrix_idx = render_entity->world_matrix_idx;

		render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);
		render_pipeline->set_vertex_shader_resource(CB_PASS_DATA_REGISTER, pass_data_cbuffer);

		render_pipeline->set_pixel_shader_resource(10, render_world->render_entity_texture_storage.textures[render_entity->ambient_texture_idx].srv);
		render_pipeline->set_pixel_shader_resource(11, render_world->render_entity_texture_storage.textures[render_entity->normal_texture_idx].srv);
		render_pipeline->set_pixel_shader_resource(12, render_world->render_entity_texture_storage.textures[render_entity->diffuse_texture_idx].srv);
		render_pipeline->set_pixel_shader_resource(13, render_world->render_entity_texture_storage.textures[render_entity->specular_texture_idx].srv);
		render_pipeline->set_pixel_shader_resource(14, render_world->render_entity_texture_storage.textures[render_entity->displacement_texture_idx].srv);

		render_pipeline->draw(render_world->triangle_meshes.mesh_instances[render_entity->mesh_idx].index_count);
	}
	// Reset shadow atlas in order to get rid of warnings (Resource being set to OM DepthStencil is still bound on input!, Forcing PS shader resource slot 1 to NULL) from directx 11.
	render_pipeline->reset_pixel_shader_resource(SHADOW_ATLAS_TEXTURE_REGISTER);
}

void Draw_Vertices_Pass::init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states)
{
	name = "Draw_Vertices";
	gpu_device->create_constant_buffer(sizeof(Render_Pass::Pass_Data), &pass_data_cbuffer);
	gpu_device->create_constant_buffer(sizeof(Vector4), &mesh_color_cbuffer);
	Render_Pass::init(gpu_device, _render_pipeline_states);
}

bool Draw_Vertices_Pass::setup_render_pipeline(Shader_Manager *shader_manager, const Depth_Stencil_View &depth_stencil_view, const Render_Target_View &render_target_view, Viewport *viewport)
{
	assert(shader_manager);
	assert(viewport);

	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_TRIANGLES;
	render_pipeline_state.shader = GET_SHADER(shader_manager, draw_vertices);
	render_pipeline_state.blend_state = render_pipeline_states->transparent_blend_state;
	render_pipeline_state.depth_stencil_view = depth_stencil_view;
	render_pipeline_state.render_target_view = render_target_view;
	render_pipeline_state.viewport = *viewport;
	return validate_render_pass(this);
}

void Draw_Vertices_Pass::render(Render_World *render_world, Render_Pipeline *render_pipeline)
{
	assert(render_world);
	assert(render_pipeline);
	assert(is_valid);

	render_pipeline->apply(&render_pipeline_state);

	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrices_struct_buffer);
	render_pipeline->set_vertex_shader_resource(2, render_world->triangle_meshes.mesh_struct_buffer);
	render_pipeline->set_vertex_shader_resource(4, render_world->triangle_meshes.index_struct_buffer);
	render_pipeline->set_vertex_shader_resource(5, render_world->triangle_meshes.vertex_struct_buffer);

	Render_Entity *render_entity = NULL;
	Debug_Cascade_Shadows_Pass::Pass_Data pass_data;

	for (u32 i = 0; i < render_world->vertex_rendering_entities.count; i++) {
		Color mesh_color = render_world->vertex_rendering_entity_colors[i];
		Render_Entity *render_entity = &render_world->vertex_rendering_entities[i];

		pass_data.mesh_idx = render_entity->mesh_idx;
		pass_data.world_matrix_idx = render_entity->world_matrix_idx;

		render_pipeline->update_constant_buffer(&mesh_color_cbuffer, (void *)&mesh_color);
		render_pipeline->set_pixel_shader_resource(1, mesh_color_cbuffer);

		render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);
		render_pipeline->set_vertex_shader_resource(CB_PASS_DATA_REGISTER, pass_data_cbuffer);

		render_pipeline->draw(render_world->triangle_meshes.mesh_instances[render_entity->mesh_idx].index_count);
	}
}