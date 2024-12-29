#include "render_world.h"
#include "render_system.h"
#include "render_passes.h"
#include "shader_manager.h"

struct alignas(256) World_Matrix {
	Matrix4 world_matrix;
};

struct alignas(256) View_Matrix {
	Matrix4 view_matrix;
};

struct alignas(256) Perspective_Matrix {
	Matrix4 perspective_matrix;
};

void Box_Pass::setup_root_signature(Gpu_Device &device)
{
	root_signature.add_cb_descriptor_table_parameter(1, 0);
	root_signature.add_cb_descriptor_table_parameter(2, 0);
	root_signature.add_cb_descriptor_table_parameter(3, 0);
	root_signature.create(device, ALLOW_INPUT_LAYOUT_ACCESS | ALLOW_VERTEX_SHADER_ACCESS);
	//root_signature.begin_descriptor_table_parameter(0, VISIBLE_TO_VERTEX_SHADER);
	//root_signature.add_descriptor_range(1, world_matrix_cb_desc);
	//root_signature.add_descriptor_range(2, view_matrix_cb_desc);
	//root_signature.add_descriptor_range(3, pers_matrix_cb_desc);
	//root_signature.end_parameter();
}

const u32 RENDER_TARGET_BACK_BUFFER = 0x1;

void Box_Pass::setup_pipeline(Gpu_Device &gpu_device, Shader_Manager *shader_manager)
{
	Shader *shader = GET_SHADER(shader_manager, draw_box);

	Render_Pipeline_Desc render_pipeline_desc;
	render_pipeline_desc.root_signature = &root_signature;
	render_pipeline_desc.vertex_shader = shader;
	render_pipeline_desc.pixel_shader = shader;
	render_pipeline_desc.add_layout("POSITION", DXGI_FORMAT_R32G32B32_FLOAT);
	render_pipeline_desc.add_layout("COLOR", DXGI_FORMAT_R32G32B32_FLOAT);
	render_pipeline_desc.depth_stencil_format = DXGI_FORMAT_D32_FLOAT;
	render_pipeline_desc.add_render_target(DXGI_FORMAT_R8G8B8A8_UNORM);
	render_pipeline_desc.viewport.width = 1900;
	render_pipeline_desc.viewport.height = 980;

	pipeline_state.create(gpu_device, render_pipeline_desc);
}

void Box_Pass::init(Gpu_Device &device, Shader_Manager *shader_manager, Pipeline_Resource_Storage *pipeline_resource_storage)
{
	world_matrix_buffer = pipeline_resource_storage->request_constant_buffer(sizeof(World_Matrix));
	view_matrix_buffer = pipeline_resource_storage->request_constant_buffer(sizeof(World_Matrix));
	pers_matrix_buffer = pipeline_resource_storage->request_constant_buffer(sizeof(World_Matrix));

	setup_root_signature(device);
	setup_pipeline(device, shader_manager);
}

void Box_Pass::render(Render_Command_Buffer *render_command_buffer, Render_World *render_world, void *args)
{
	Render_System *render_sys = (Render_System *)args;
	Size_u32 window = render_sys->get_window_size();

	render_command_buffer->apply(pipeline_state, RENDER_TARGET_BUFFER_BUFFER);
	
	D3D12_RECT clip_rect;
	D3D12_VIEWPORT viewport;
	
	ZeroMemory(&viewport, sizeof(D3D12_VIEWPORT));
	viewport.Width = (float)window.width;
	viewport.Height = (float)window.height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	ZeroMemory(&clip_rect, sizeof(D3D12_RECT));
	clip_rect.right = window.width;
	clip_rect.bottom = window.height;

	auto world_matrix = make_identity_matrix();
	auto position = Vector3(0.0f, 0.0f, -10.0f);
	auto direction = Vector3::base_z;
	auto view_matrix = make_look_at_matrix(position, position + direction, Vector3::base_y);
	auto perspective_matrix = make_perspective_matrix(90, 16.0f / 9.0f, 1.0f, 1000.0f);

	world_matrix_buffer->write((void *)&world_matrix, sizeof(Matrix4));
	view_matrix_buffer->write((void *)&view_matrix, sizeof(Matrix4));
	pers_matrix_buffer->write((void *)&perspective_matrix, sizeof(Matrix4));

	auto &command_list = render_command_buffer->graphics_command_list;

	//command_list.get()->SetGraphicsRootSignature(root_signature.get());
	//command_list.get()->SetPipelineState(pipeline_state.get());
	//command_list.get()->SetDescriptorHeaps(1, render_sys->descriptors_pool.cbsrua_descriptor_heap.get_address());

	command_list.get()->SetGraphicsRootDescriptorTable(0, world_matrix_buffer->get_frame_resource()->cb_descriptor.gpu_handle);
	command_list.get()->SetGraphicsRootDescriptorTable(1, view_matrix_buffer->get_frame_resource()->cb_descriptor.gpu_handle);
	command_list.get()->SetGraphicsRootDescriptorTable(2, pers_matrix_buffer->get_frame_resource()->cb_descriptor.gpu_handle);
	
	//command_list.get()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	command_list.set_vertex_buffer(render_sys->vertex_buffer);
	command_list.set_index_buffer(render_sys->index_buffer);

	render_command_buffer->draw(36);
	
	//command_list.get()->RSSetViewports(1, &viewport);
	//command_list.get()->RSSetScissorRects(1, &clip_rect);
	//auto temp = render_sys->back_buffer_textures[render_sys->back_buffer_index].rt_descriptor.cpu_handle;
	//const auto temp2 = render_sys->back_buffer_depth_texture.ds_descriptor.cpu_handle;
	//command_list.get()->OMSetRenderTargets(1, &temp, FALSE, &temp2);

	//command_list.get()->DrawIndexedInstanced(36, 1, 0, 0, 0);
}


//#include <assert.h>
//
//#include "hlsl.h"
//#include "render_passes.h"
//#include "render_system.h"
//#include "render_world.h"
//#include "../libs/math/matrix.h"
//#include "../libs/math/functions.h"
//#include "../sys/sys.h"
//
//
//const u32 SKIP_RENDER_TARGET_VIEW_VALIDATION = 0x1;
//const u32 SKIP_DEPTH_STENCIL_VIEW_VALIDATION = 0x2;
//const u32 SKIP_VIEWPORT_VALIDATION = 0x4;
//const u32 SKIP_PIXEL_SHADER_VALIDATION = 0x8;
//
//inline void setup_default_render_pipeline_state(Render_Pipeline_State *render_pipeline_state, Render_Pipeline_States *render_pipeline_states)
//{
//	assert(render_pipeline_state);
//	assert(render_pipeline_states);
//
//	render_pipeline_state->blend_state = render_pipeline_states->default_blend_state;
//	render_pipeline_state->depth_stencil_state = render_pipeline_states->default_depth_stencil_state;
//	render_pipeline_state->rasterizer_state = render_pipeline_states->default_rasterizer_state;
//}
//
//inline bool validate_render_pipeline(String *render_pass_name, Render_Pipeline_State *render_pipeline_state, u32 validation_flags = 0)
//{
//	assert(render_pass_name);
//	assert(render_pipeline_state);
//
//	bool result = true;
//	if (!render_pipeline_state->blend_state) {
//		print("validate_render_pass: Render Pass '{}' is not valid. For a render pipeline was not assigned a blend state.", render_pass_name);
//		result = false;
//	}
//	if (!render_pipeline_state->depth_stencil_state) {
//		print("validate_render_pass: Render Pass '{}' is not valid. For a render pipeline was not assigned a depth stencil state.", render_pass_name);
//		result = false;
//	}
//	if (((render_pipeline_state->viewport.width == 0) || (render_pipeline_state->viewport.height == 0)) && !(validation_flags & SKIP_VIEWPORT_VALIDATION)) {
//		print("validate_render_pass: Render Pass '{}' is not valid.. A render pipeline has a wrong setup viewport.", render_pass_name);
//		result = false;
//	}
//	if (!render_pipeline_state->depth_stencil_view && !(validation_flags & SKIP_DEPTH_STENCIL_VIEW_VALIDATION)) {
//		print("validate_render_pass: Render Pass '{}' is not valid. For a render pipeline was not assigned a depth stencil view.", render_pass_name);
//		result = false;
//	} if (!render_pipeline_state->render_target_view && !(validation_flags & SKIP_RENDER_TARGET_VIEW_VALIDATION)) {
//		print("validate_render_pass: Render Pass '{}' is not valid. For a render pipeline was not assigned a render target view.", render_pass_name);
//		result = false;
//	}
//	if (render_pipeline_state->shader) {
//		Extend_Shader *shader = (Extend_Shader *)render_pipeline_state->shader;
//		if (validation_flags & SKIP_PIXEL_SHADER_VALIDATION) {
//			if (!is_valid(shader, VALIDATE_RENDERING_SHADER)) {
//				print("validate_render_pass: Render Pass '{}' is not valid. {} was not initialized correctly.", render_pass_name, shader->file_name);
//				result = false;
//			}
//		} else {
//			if (!is_valid(shader, VALIDATE_VERTEX_SHADER)) {
//				print("validate_render_pass: Render Pass '{}' is not valid. {} was not initialized correctly.", render_pass_name, shader->file_name);
//				result = false;
//			}
//		}
//	} else {
//		print("validate_render_pass: Render Pass '{}' is not valid. For a render pipeline was not assigned a shader.", render_pass_name);
//		result = false;
//	}
//	return result;
//}
//
//struct Depth_Map_Pass_Data {
//	u32 mesh_idx;
//	u32 world_matrix_idx;
//	Pad2 pad;
//	Matrix4 view_projection_matrix;
//};
//
//void Render_Pass::init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states)
//{
//	assert(gpu_device);
//	assert(_render_pipeline_states);
//	assert(!name.is_empty());
//
//	render_pipeline_states = _render_pipeline_states;
//	setup_default_render_pipeline_state(&render_pipeline_state, render_pipeline_states);
//}
//
//void Forwar_Light_Pass::init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states)
//{
//	name = "Forward_Light";
//	gpu_device->create_constant_buffer(sizeof(Render_Pass::Pass_Data), &pass_data_cbuffer);
//	gpu_device->create_constant_buffer(sizeof(CB_Shadow_Atlas_Info), &shadow_atlas_info_cbuffer);
//	Render_Pass::init(gpu_device, _render_pipeline_states);
//}
//
//void Forwar_Light_Pass::setup_render_pipeline(Shader_Manager *shader_manager, const Depth_Stencil_View &depth_stencil_view, const Render_Target_View &render_target_view, Viewport *viewport)
//{
//	assert(shader_manager);
//	assert(viewport);
//
//	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_TRIANGLES;
//	render_pipeline_state.shader = GET_SHADER(shader_manager, forward_light);
//	render_pipeline_state.depth_stencil_view = depth_stencil_view;
//	render_pipeline_state.render_target_view = render_target_view;
//	render_pipeline_state.viewport = *viewport;
//
//	is_valid = validate_render_pipeline(&name, &render_pipeline_state);
//}
//
//void Forwar_Light_Pass::render(Render_World *render_world, Render_Pipeline *render_pipeline)
//{
//	assert(render_world);
//	assert(render_pipeline);
//	assert(is_valid);
//
//	begin_mark_rendering_event(L"Forward rendering");
//	render_pipeline->apply(&render_pipeline_state);
//
//	CB_Shadow_Atlas_Info shadow_atlas_info;
//	shadow_atlas_info.shadow_atlas_size = SHADOW_ATLAS_SIZE;
//	shadow_atlas_info.shadow_cascade_size = CASCADE_SIZE;
//	shadow_atlas_info.jittering_sampling_tile_size = render_world->jittering_tile_size;
//	shadow_atlas_info.jittering_sampling_filter_size = render_world->jittering_filter_size;
//	shadow_atlas_info.jittering_sampling_scaling = render_world->jittering_scaling;
//
//	render_pipeline->update_constant_buffer(&shadow_atlas_info_cbuffer, (void *)&shadow_atlas_info);
//
//	render_pipeline->set_vertex_shader_resource(CB_PASS_DATA_REGISTER, pass_data_cbuffer);
//	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrices_struct_buffer);
//	render_pipeline->set_vertex_shader_resource(2, render_world->triangle_meshes.mesh_struct_buffer);
//	render_pipeline->set_vertex_shader_resource(4, render_world->triangle_meshes.index_struct_buffer);
//	render_pipeline->set_vertex_shader_resource(5, render_world->triangle_meshes.vertex_struct_buffer);
//
//	render_pipeline->set_pixel_shader_resource(CB_SHADOW_ATLAS_INFO_REGISTER, shadow_atlas_info_cbuffer);
//	render_pipeline->set_pixel_shader_resource(SHADOW_ATLAS_TEXTURE_REGISTER, render_world->shadow_atlas.srv);
//	render_pipeline->set_pixel_shader_resource(7, render_world->lights_struct_buffer);
//	render_pipeline->set_pixel_shader_resource(8, render_world->cascaded_view_projection_matrices_sb);
//	render_pipeline->set_pixel_shader_resource(9, render_world->cascaded_shadows_info_sb);
//	render_pipeline->set_pixel_shader_resource(JITTERING_SAMPLES_TEXTURE_REGISTER, render_world->jittering_samples.srv);
//	render_pipeline->set_pixel_shader_sampler(LINEAR_SAMPLING_REGISTER, render_pipeline_states->linear_sampling);
//	// Bind point sampling in order to get rid of a d3d11 warning.
//	// May be just better have a method for binding samplers once per a frame.
//	// The Pixel Shader unit expects a Sampler to be set at Slot 0, but none is bound. This is perfectly valid, as a NULL Sampler maps to default Sampler state. However, the developer may not want to rely on the defaults.
//	render_pipeline->set_pixel_shader_sampler(POINT_SAMPLING_REGISTER, render_pipeline_states->point_sampling);
//
//	Render_Entity *render_entity = NULL;
//	Forwar_Light_Pass::Pass_Data pass_data;
//
//	For(render_world->game_render_entities, render_entity) {
//		pass_data.mesh_idx = render_entity->mesh_id.instance_idx;
//		pass_data.world_matrix_idx = render_entity->world_matrix_idx;
//
//		render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);
//
//		Mesh_Textures *mesh_textures = render_world->triangle_meshes.get_mesh_textures(render_entity->mesh_id.textures_idx);
//		render_pipeline->set_pixel_shader_resource(11, render_world->triangle_meshes.get_texture(mesh_textures->normal_idx)->srv);
//		render_pipeline->set_pixel_shader_resource(12, render_world->triangle_meshes.get_texture(mesh_textures->diffuse_idx)->srv);
//		render_pipeline->set_pixel_shader_resource(13, render_world->triangle_meshes.get_texture(mesh_textures->specular_idx)->srv);
//		render_pipeline->set_pixel_shader_resource(14, render_world->triangle_meshes.get_texture(mesh_textures->displacement_idx)->srv);
//
//		render_pipeline->draw(render_world->triangle_meshes.mesh_instances[render_entity->mesh_id.instance_idx].index_count);
//	}
//	// Reset shadow atlas in order to get rid of warnings (Resource being set to OM DepthStencil is still bound on input!, Forcing PS shader resource slot 1 to NULL) from directx 11.
//	render_pipeline->reset_pixel_shader_resource(SHADOW_ATLAS_TEXTURE_REGISTER);
//	end_mark_rendering_event();
//}
//
//void Shadows_Pass::init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states)
//{
//	name = "Shadows";
//	gpu_device->create_constant_buffer(sizeof(Depth_Map_Pass_Data), &pass_data_cbuffer);
//	Render_Pass::init(gpu_device, _render_pipeline_states);
//}
//
//void Shadows_Pass::setup_render_pipeline(Shader_Manager *shader_manager, const Depth_Stencil_View &depth_stencil_view)
//{
//	assert(shader_manager);
//
//	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_TRIANGLES;
//	render_pipeline_state.shader = GET_SHADER(shader_manager, depth_map);
//	render_pipeline_state.depth_stencil_view = depth_stencil_view;
//	render_pipeline_state.render_target_view = nullptr;
//	is_valid = validate_render_pipeline(&name, &render_pipeline_state, SKIP_VIEWPORT_VALIDATION | SKIP_RENDER_TARGET_VIEW_VALIDATION);
//}
//
//void Shadows_Pass::render(Render_World *render_world, Render_Pipeline *render_pipeline)
//{
//	assert(render_world);
//	assert(render_pipeline);
//	assert(is_valid);
//
//	begin_mark_rendering_event(L"Shadows rendering");
//	//@Note: this code can be moved to render world
//	render_pipeline->clear_depth_stencil_view(render_world->shadow_atlas.dsv);
//
//	render_pipeline->apply(&render_pipeline_state);
//
//	render_pipeline->set_vertex_shader_resource(CB_PASS_DATA_REGISTER, pass_data_cbuffer);
//	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrices_struct_buffer);
//	render_pipeline->set_vertex_shader_resource(2, render_world->triangle_meshes.mesh_struct_buffer);
//	render_pipeline->set_vertex_shader_resource(4, render_world->triangle_meshes.index_struct_buffer);
//	render_pipeline->set_vertex_shader_resource(5, render_world->triangle_meshes.vertex_struct_buffer);
//
//	Depth_Map_Pass_Data pass_data;
//
//	Cascaded_Shadows *cascaded_shadows = NULL;
//	For(render_world->cascaded_shadows_list, cascaded_shadows) {
//		Cascaded_Shadow_Map *cascaded_shadow_map = NULL;
//		For(cascaded_shadows->cascaded_shadow_maps, cascaded_shadow_map) {
//			render_pipeline->set_viewport(&cascaded_shadow_map->viewport);
//
//			Render_Entity *render_entity = NULL;
//			For(render_world->game_render_entities, render_entity) {
//				pass_data.mesh_idx = render_entity->mesh_id.instance_idx;
//				pass_data.world_matrix_idx = render_entity->world_matrix_idx;
//				pass_data.view_projection_matrix = cascaded_shadow_map->view_projection_matrix;
//
//				render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);
//				render_pipeline->draw(render_world->triangle_meshes.mesh_instances[render_entity->mesh_id.instance_idx].index_count);
//			}
//		}
//	}
//	end_mark_rendering_event();
//}
//
//void Debug_Cascade_Shadows_Pass::init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states)
//{
//	name = "Debug_Cascade_Shadows";
//	gpu_device->create_constant_buffer(sizeof(Render_Pass::Pass_Data), &pass_data_cbuffer);
//	gpu_device->create_constant_buffer(sizeof(CB_Shadow_Atlas_Info), &shadow_atlas_info_cbuffer);
//	return Render_Pass::init(gpu_device, _render_pipeline_states);
//}
//
//void Debug_Cascade_Shadows_Pass::setup_render_pipeline(Shader_Manager *shader_manager, const Depth_Stencil_View &depth_stencil_view, const Render_Target_View &render_target_view, Viewport *viewport)
//{
//	assert(shader_manager);
//	assert(viewport);
//
//	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_TRIANGLES;
//	render_pipeline_state.shader = GET_SHADER(shader_manager, debug_cascaded_shadows);
//	render_pipeline_state.depth_stencil_view = depth_stencil_view;
//	render_pipeline_state.render_target_view = render_target_view;
//	render_pipeline_state.viewport = *viewport;
//
//	is_valid = validate_render_pipeline(&name, &render_pipeline_state);
//}
//
//void Debug_Cascade_Shadows_Pass::render(Render_World *render_world, Render_Pipeline *render_pipeline)
//{
//	assert(render_world);
//	assert(render_pipeline);
//	assert(is_valid);
//
//	begin_mark_rendering_event(L"Debug cascaded shadows");
//	render_pipeline->apply(&render_pipeline_state);
//
//	CB_Shadow_Atlas_Info shadow_atlas_info;
//	shadow_atlas_info.shadow_atlas_size = SHADOW_ATLAS_SIZE;
//	shadow_atlas_info.shadow_cascade_size = CASCADE_SIZE;
//	shadow_atlas_info.jittering_sampling_tile_size = render_world->jittering_tile_size;
//	shadow_atlas_info.jittering_sampling_filter_size = render_world->jittering_filter_size;
//	shadow_atlas_info.jittering_sampling_scaling = render_world->jittering_scaling;
//
//	render_pipeline->update_constant_buffer(&shadow_atlas_info_cbuffer, (void *)&shadow_atlas_info);
//
//	render_pipeline->set_vertex_shader_resource(CB_PASS_DATA_REGISTER, pass_data_cbuffer);
//	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrices_struct_buffer);
//	render_pipeline->set_vertex_shader_resource(2, render_world->triangle_meshes.mesh_struct_buffer);
//	render_pipeline->set_vertex_shader_resource(4, render_world->triangle_meshes.index_struct_buffer);
//	render_pipeline->set_vertex_shader_resource(5, render_world->triangle_meshes.vertex_struct_buffer);
//
//	render_pipeline->set_pixel_shader_resource(CB_SHADOW_ATLAS_INFO_REGISTER, shadow_atlas_info_cbuffer);
//	render_pipeline->set_pixel_shader_resource(SHADOW_ATLAS_TEXTURE_REGISTER, render_world->shadow_atlas.srv);
//	render_pipeline->set_pixel_shader_resource(7, render_world->lights_struct_buffer);
//	render_pipeline->set_pixel_shader_resource(8, render_world->cascaded_view_projection_matrices_sb);
//	render_pipeline->set_pixel_shader_resource(9, render_world->cascaded_shadows_info_sb);
//	render_pipeline->set_pixel_shader_resource(JITTERING_SAMPLES_TEXTURE_REGISTER, render_world->jittering_samples.srv);
//	render_pipeline->set_pixel_shader_sampler(POINT_SAMPLING_REGISTER, render_pipeline_states->point_sampling);
//
//	Render_Entity *render_entity = NULL;
//	Debug_Cascade_Shadows_Pass::Pass_Data pass_data;
//
//	For(render_world->game_render_entities, render_entity) {
//		pass_data.mesh_idx = render_entity->mesh_id.instance_idx;
//		pass_data.world_matrix_idx = render_entity->world_matrix_idx;
//
//		render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);
//
//		Mesh_Textures *mesh_textures = render_world->triangle_meshes.get_mesh_textures(render_entity->mesh_id.textures_idx);
//		render_pipeline->set_pixel_shader_resource(11, render_world->triangle_meshes.get_texture(mesh_textures->normal_idx)->srv);
//		render_pipeline->set_pixel_shader_resource(12, render_world->triangle_meshes.get_texture(mesh_textures->diffuse_idx)->srv);
//		render_pipeline->set_pixel_shader_resource(13, render_world->triangle_meshes.get_texture(mesh_textures->specular_idx)->srv);
//		render_pipeline->set_pixel_shader_resource(14, render_world->triangle_meshes.get_texture(mesh_textures->displacement_idx)->srv);
//
//		render_pipeline->draw(render_world->triangle_meshes.mesh_instances[render_entity->mesh_id.instance_idx].index_count);
//	}
//	// Reset shadow atlas in order to get rid of warnings (Resource being set to OM DepthStencil is still bound on input!, Forcing PS shader resource slot 1 to NULL) from directx 11.
//	render_pipeline->reset_pixel_shader_resource(SHADOW_ATLAS_TEXTURE_REGISTER);
//	end_mark_rendering_event();
//}
//
//void Outlining_Pass::add_render_entity_index(u32 entity_index)
//{
//	render_entity_indices.push(entity_index);
//}
//
//void Outlining_Pass::delete_render_entity_index(u32 entity_index)
//{
//	for (u32 i = 0; i < render_entity_indices.count; i++) {
//		if (render_entity_indices[i] == entity_index) {
//			render_entity_indices.remove(i);
//		}
//	}
//}
//
//void Outlining_Pass::reset_render_entity_indices()
//{
//	render_entity_indices.count = 0;
//}
//
//void Outlining_Pass::setup_outlining(u32 outlining_size_in_pixels, const Color &color)
//{
//	u32 samples_in_one_row = outlining_size_in_pixels * 2 + 1;
//	outlining_info.offset_range = (s32)floor((float)samples_in_one_row * 0.5f);
//	outlining_info.color = (Color)color;
//}
//
//void Outlining_Pass::init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states)
//{
//	name = "Outlining";
//	gpu_device->create_constant_buffer(sizeof(Render_Pass::Pass_Data), &pass_data_cbuffer);
//	gpu_device->create_constant_buffer(sizeof(Outlining_Info), &outlining_info_cbuffer);
//	Render_Pass::init(gpu_device, _render_pipeline_states);
//}
//
//void Outlining_Pass::setup_render_pipeline(Shader_Manager *shader_manager, Texture2D *_silhouette_back_buffer, Texture2D *_silhouette_depth_stencil_buffer, Texture2D *_screen_back_buffer, Texture2D *_screen_depth_stencil_back_buffer, Viewport *viewport)
//{
//	assert(shader_manager);
//	assert(_silhouette_back_buffer);
//	assert(_silhouette_depth_stencil_buffer);
//	assert(_screen_back_buffer);
//	assert(_screen_depth_stencil_back_buffer);
//	assert(viewport);
//
//	silhouette_back_buffer = _silhouette_back_buffer;
//	silhoueete_depth_stencil_buffer = _silhouette_depth_stencil_buffer;
//	screen_back_buffer = _screen_back_buffer;
//	screen_depth_stencil_back_buffer = _screen_depth_stencil_back_buffer;
//
//	outlining_compute_shader = GET_SHADER(shader_manager, outlining);
//
//	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_TRIANGLES;
//	render_pipeline_state.shader = GET_SHADER(shader_manager, silhouette);
//	render_pipeline_state.blend_state = render_pipeline_states->disabled_blend_state; // DXGI_FORMAT_R32_UINT doesn't support blending.
//	render_pipeline_state.depth_stencil_view = silhoueete_depth_stencil_buffer->dsv;
//	render_pipeline_state.render_target_view = silhouette_back_buffer->rtv;
//	render_pipeline_state.viewport = *viewport;
//
//	float compute_shader_thread_number = 32.0f;
//	thread_group_count_x = (u32)math::ceil((float)viewport->width / compute_shader_thread_number);
//	thread_group_count_y = (u32)math::ceil((float)viewport->height / compute_shader_thread_number);
//
//	is_valid = validate_render_pipeline(&name, &render_pipeline_state) && (outlining_info.offset_range != 0) && ::is_valid(outlining_compute_shader, VALIDATE_COMPUTE_SHADER);
//}
//
//void Outlining_Pass::render(Render_World *render_world, Render_Pipeline *render_pipeline)
//{
//	assert(render_world);
//	assert(render_pipeline);
//	assert(is_valid);
//	assert(outlining_compute_shader);
//
//	begin_mark_rendering_event(L"Outlining rendering");
//	render_pipeline->apply(&render_pipeline_state);
//
//	render_pipeline->set_vertex_shader_resource(CB_PASS_DATA_REGISTER, pass_data_cbuffer);
//	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrices_struct_buffer);
//	render_pipeline->set_vertex_shader_resource(2, render_world->triangle_meshes.mesh_struct_buffer);
//	render_pipeline->set_vertex_shader_resource(4, render_world->triangle_meshes.index_struct_buffer);
//	render_pipeline->set_vertex_shader_resource(5, render_world->triangle_meshes.vertex_struct_buffer);
//	render_pipeline->set_pixel_shader_resource(CB_PASS_DATA_REGISTER, pass_data_cbuffer);
//
//	Render_Entity *render_entity = NULL;
//	Render_Pass::Pass_Data pass_data;
//
//	for (u32 i = 0; i < render_entity_indices.count; i++) {
//		u32 index = render_entity_indices[i];
//		Render_Entity *render_entity = &render_world->game_render_entities[index];
//
//		pass_data.mesh_idx = render_entity->mesh_id.instance_idx;
//		pass_data.world_matrix_idx = render_entity->world_matrix_idx;
//		pass_data.pad1 = i + 1;
//
//		render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);
//		render_pipeline->draw(render_world->triangle_meshes.mesh_instances[render_entity->mesh_id.instance_idx].index_count);
//	}
//	render_pipeline->reset_render_target();
//
//	render_pipeline->update_constant_buffer(&outlining_info_cbuffer, (void *)&outlining_info);
//	render_pipeline->set_compute_shader_resource(CB_OUTLINING_INFO_REGISTER, outlining_info_cbuffer);
//	render_pipeline->set_compute_shader(outlining_compute_shader);
//	render_pipeline->set_compute_shader_resource(SCREEN_BACK_BUFFER_REGISTER, screen_back_buffer->uav);
//	render_pipeline->set_compute_shader_resource(SCREEN_BACK_BUFFER_DEPTH_STECHIL_REGISTER, screen_depth_stencil_back_buffer->srv);
//	render_pipeline->set_compute_shader_resource(SILHOUETTE_TEXTURE_REGISTER, silhouette_back_buffer->srv);
//	render_pipeline->set_compute_shader_resource(SILHOUETTE_DEPTH_STENCIL_TEXTURE_REGISTER, silhoueete_depth_stencil_buffer->srv);
//
//	render_pipeline->dispatch(thread_group_count_x, thread_group_count_y, 1);
//
//	render_pipeline->reset_compute_unordered_access_view(SCREEN_BACK_BUFFER_REGISTER);
//	render_pipeline->reset_compute_shader_resource_view(SILHOUETTE_TEXTURE_REGISTER);
//	render_pipeline->reset_compute_shader_resource_view(SILHOUETTE_DEPTH_STENCIL_TEXTURE_REGISTER);
//	render_pipeline->reset_compute_shader_resource_view(SCREEN_BACK_BUFFER_DEPTH_STECHIL_REGISTER);
//	end_mark_rendering_event();
//}
//
//void Voxelization::init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states)
//{
//	name = "Voxelization";
//	gpu_device->create_constant_buffer(sizeof(Render_Pass::Pass_Data), &pass_data_cbuffer);
//	gpu_device->create_constant_buffer(sizeof(Voxelization_Info), &voxelization_info_cbuffer);
//	Render_Pass::init(gpu_device, _render_pipeline_states);
//}
//
//void Voxelization::setup_render_pipeline(Shader_Manager *shader_manager, const Unordered_Access_View &voxel_buffer_view, const Render_Target_View &render_target_view)
//{
//	assert(shader_manager);
//	assert(voxel_buffer_view);
//
//	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_TRIANGLES;
//	render_pipeline_state.shader = GET_SHADER(shader_manager, voxelization);
//	render_pipeline_state.rasterizer_state = render_pipeline_states->disabled_multisampling_state;
//	render_pipeline_state.blend_state = render_pipeline_states->disabled_blend_state;
//	render_pipeline_state.depth_stencil_state = render_pipeline_states->disabled_depth_test;
//	render_pipeline_state.depth_stencil_view = nullptr;
//	render_pipeline_state.render_target_view = render_target_view;
//	render_pipeline_state.unordered_access_view = voxel_buffer_view;
//
//	is_valid = validate_render_pipeline(&name, &render_pipeline_state, SKIP_DEPTH_STENCIL_VIEW_VALIDATION | SKIP_RENDER_TARGET_VIEW_VALIDATION | SKIP_VIEWPORT_VALIDATION);
//}
//
//void Voxelization::render(Render_World *render_world, Render_Pipeline *render_pipeline)
//{
//	assert(is_valid);
//	assert(render_world);
//	assert(render_pipeline);
//
//	begin_mark_rendering_event(L"Voxelization");
//	Viewport viewport;
//	viewport.width = render_world->voxel_grid.grid_size.width;
//	viewport.height = render_world->voxel_grid.grid_size.height;
//
//	render_pipeline_state.viewport = viewport;
//
//	Voxelization_Info voxelization_info;
//	voxelization_info.grid_size = render_world->voxel_grid.grid_size;
//	voxelization_info.ceil_size = render_world->voxel_grid.ceil_size;
//	voxelization_info.texel_size = Vector2(1.0f / viewport.width, 1.0f / viewport.height);
//	voxelization_info.grid_center = render_world->voxel_grid_center;
//	voxelization_info.voxel_orthographic_matrix = render_world->voxel_matrix;
//	voxelization_info.voxel_view_matrices[0] = render_world->left_to_right_voxel_view_matrix;
//	voxelization_info.voxel_view_matrices[1] = render_world->top_to_down_voxel_view_matrix;
//	voxelization_info.voxel_view_matrices[2] = render_world->back_to_front_voxel_view_matrix;
//
//	render_pipeline->apply(&render_pipeline_state);
//	render_pipeline->set_geometry_shader(render_pipeline_state.shader);
//	
//	render_pipeline->update_constant_buffer(&voxelization_info_cbuffer, (void *)&voxelization_info);
//
//	render_pipeline->set_vertex_shader_resource(CB_PASS_DATA_REGISTER, pass_data_cbuffer);
//	render_pipeline->set_vertex_shader_resource(3, render_world->world_matrices_struct_buffer);
//	render_pipeline->set_vertex_shader_resource(2, render_world->triangle_meshes.mesh_struct_buffer);
//	render_pipeline->set_vertex_shader_resource(4, render_world->triangle_meshes.index_struct_buffer);
//	render_pipeline->set_vertex_shader_resource(5, render_world->triangle_meshes.vertex_struct_buffer);
//	render_pipeline->set_vertex_shader_resource(1, voxelization_info_cbuffer);
//
//	render_pipeline->set_geometry_shader_resource(1, voxelization_info_cbuffer);
//	
//	render_pipeline->set_pixel_shader_resource(1, voxelization_info_cbuffer);
//	render_pipeline->set_pixel_shader_sampler(LINEAR_SAMPLING_REGISTER, render_pipeline_states->linear_sampling);
//
//	Render_Entity *render_entity = NULL;
//	Render_Pass::Pass_Data pass_data;
//
//	For(render_world->game_render_entities, render_entity) {
//		pass_data.mesh_idx = render_entity->mesh_id.instance_idx;
//		pass_data.world_matrix_idx = render_entity->world_matrix_idx;
//		
//		Mesh_Textures *mesh_textures = render_world->triangle_meshes.get_mesh_textures(render_entity->mesh_id.textures_idx);
//		render_pipeline->set_pixel_shader_resource(12, render_world->triangle_meshes.get_texture(mesh_textures->diffuse_idx)->srv);
//		
//		render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);
//		render_pipeline->draw(render_world->triangle_meshes.mesh_instances[render_entity->mesh_id.instance_idx].index_count);
//	}
//	render_pipeline->dx11_context->GSSetShader(nullptr, 0, 0);
//	end_mark_rendering_event();
//}