#include <limits.h>

#include "../sys/sys.h"
#include "render_world.h"
#include "render_system.h"
#include "render_passes.h"
#include "shader_manager.h"
#include "renderer.h"

#include "render_api/base_structs.h"

struct Pass_Data {
	u32 parameter0;
	u32 parameter1;
	u32 parameter2;
	u32 parameter3;
};

Render_Pass::Render_Pass()
{
	name = "Unnamed render pass";
}

Render_Pass::~Render_Pass()
{
}

void Render_Pass::init(const char *pass_name, Render_Device *device, Shader_Manager *shader_manager, Pipeline_Resource_Manager *resource_manager)
{
	name = pass_name;
	root_signature = device->create_root_signature();

	setup_root_signature(device);
	setup_pipeline(device, shader_manager);
	schedule_resources(resource_manager);
}

void Render_Pass::setup_root_signature(Render_Device *device)
{
	root_signature->add_constant_buffer_parameter(0, 10); // Global Info CB
	root_signature->add_constant_buffer_parameter(1, 10); // Frame Info CB
	root_signature->add_shader_resource_parameter(0, 10, UNBOUNDED_DESCRIPTORS_NUMBER);  //Textures
	root_signature->add_sampler_parameter(0, 10, UNBOUNDED_DESCRIPTORS_NUMBER); //Samplers

	root_signature->compile(access);
}

void Render_Pass::schedule_resources(Pipeline_Resource_Manager *resource_manager)
{
}

void Shadows_Pass::init(Render_Device *device, Shader_Manager *shader_manager, Pipeline_Resource_Manager *resource_manager)
{
	Render_Pass::init("Shadows", device, shader_manager, resource_manager);
}

void Shadows_Pass::schedule_resources(Pipeline_Resource_Manager *resource_manager)
{
	Depth_Stencil_Texture_Desc depth_stencil_desc;
	depth_stencil_desc.width = SHADOW_ATLAS_SIZE;
	depth_stencil_desc.height = SHADOW_ATLAS_SIZE;
	depth_stencil_desc.format = DXGI_FORMAT_D32_FLOAT;
	
	shadow_atlas = resource_manager->create_depth_stencil("shadow_atlas", &depth_stencil_desc);
}

struct Depth_Map_Pass_Data {
	u32 mesh_idx;
	u32 world_matrix_idx;
	Pad2 pad;
	Matrix4 view_projection_matrix;
};

void Shadows_Pass::setup_root_signature(Render_Device *device)
{
	root_signature->add_32bit_constants_parameter(0, 0, sizeof(Depth_Map_Pass_Data));
	root_signature->add_shader_resource_parameter(0, 0); //World matrices
	root_signature->add_shader_resource_parameter(1, 0); //Mesh instances
	root_signature->add_shader_resource_parameter(2, 0); //unified vertex buffer
	root_signature->add_shader_resource_parameter(3, 0); //Unified index buffer

	access = ALLOW_VERTEX_SHADER_ACCESS;
	Render_Pass::setup_root_signature(device);
}

void Shadows_Pass::setup_pipeline(Render_Device *render_device, Shader_Manager *shader_manager)
{
	Graphics_Pipeline_Desc graphics_pipeline_desc;
	graphics_pipeline_desc.root_signature = root_signature;
	graphics_pipeline_desc.vs_bytecode = GET_SHADER(shader_manager, depth_map)->vs_bytecode.bytecode_ref();
	graphics_pipeline_desc.ps_bytecode = GET_SHADER(shader_manager, depth_map)->ps_bytecode.bytecode_ref();
	graphics_pipeline_desc.depth_stencil_format = DXGI_FORMAT_D32_FLOAT;

	pipeline_state = render_device->create_pipeline_state(&graphics_pipeline_desc);
}

void Shadows_Pass::render(Graphics_Command_List *graphics_command_list, void *context, void *args)
{
	Render_World *render_world = (Render_World *)context;
	Render_System *render_sys = (Render_System *)args;

	graphics_command_list->begin_event("Shadows mapping");

	graphics_command_list->clear_depth_stencil(shadow_atlas);
	graphics_command_list->set_render_target(NULL, shadow_atlas);

	graphics_command_list->apply(pipeline_state);

	Pipeline_Resource_Manager *pipeline_resource_manager = &render_sys->pipeline_resource_manager;
	pipeline_resource_manager->global_buffer;
	
	graphics_command_list->set_graphics_descriptor_table(0, 10, SAMPLER_REGISTER, render_sys->render_device->base_sampler_descriptor());

	graphics_command_list->set_graphics_constant_buffer(0, 10, pipeline_resource_manager->global_buffer);
	graphics_command_list->set_graphics_constant_buffer(1, 10, pipeline_resource_manager->frame_info_buffer);

	graphics_command_list->set_graphics_descriptor_table(0, 0, SHADER_RESOURCE_REGISTER, render_world->world_matrices_buffer->shader_resource_descriptor());
	graphics_command_list->set_graphics_descriptor_table(1, 0, SHADER_RESOURCE_REGISTER, render_world->model_storage.mesh_instance_buffer->shader_resource_descriptor());
	graphics_command_list->set_graphics_descriptor_table(2, 0, SHADER_RESOURCE_REGISTER, render_world->model_storage.unified_vertex_buffer->shader_resource_descriptor());
	graphics_command_list->set_graphics_descriptor_table(3, 0, SHADER_RESOURCE_REGISTER, render_world->model_storage.unified_index_buffer->shader_resource_descriptor());
	
	Depth_Map_Pass_Data pass_data;

	Cascaded_Shadows *cascaded_shadows = NULL;
	For(render_world->cascaded_shadows_list, cascaded_shadows) {
		Cascaded_Shadow_Map *cascaded_shadow_map = NULL;
		For(cascaded_shadows->cascaded_shadow_maps, cascaded_shadow_map) {
			graphics_command_list->set_viewport(cascaded_shadow_map->viewport);

			Render_Entity *render_entity = NULL;
			For(render_world->game_render_entities, render_entity) {
				pass_data.mesh_idx = render_entity->mesh_idx;
				pass_data.world_matrix_idx = render_entity->world_matrix_idx;
				pass_data.view_projection_matrix = cascaded_shadow_map->view_projection_matrix;

				graphics_command_list->set_graphics_constants(0, 0, sizeof(Depth_Map_Pass_Data), (void *)&pass_data);
				graphics_command_list->draw(render_world->model_storage.render_models[render_entity->mesh_idx]->mesh.index_count());
			}
		}
	}
	graphics_command_list->end_event();
}

void Forward_Pass::init(Render_Device *device, Shader_Manager *shader_manager, Pipeline_Resource_Manager *resource_manager)
{
	Render_Pass::init("Forward rendering", device, shader_manager, resource_manager);
}

void Forward_Pass::schedule_resources(Pipeline_Resource_Manager *resource_manager)
{
	shadow_atlas = resource_manager->read_texture("shadow_atlas");
}

struct Shadow_Atlas {
	u32 atlas_size;
	u32 cascade_size;
	Pad2 pad;
};

struct Jittering_Filter {
	u32 tile_size;
	u32 filter_size;
	u32 scaling;
	Pad1 pad;
};

void Forward_Pass::setup_root_signature(Render_Device *device)
{
	root_signature->add_32bit_constants_parameter(0, 0, sizeof(Pass_Data)); //Pass data
	root_signature->add_shader_resource_parameter(0, 0); //World matrices
	root_signature->add_shader_resource_parameter(1, 0); //Mesh instances
	root_signature->add_shader_resource_parameter(2, 0); //unified vertex buffer
	root_signature->add_shader_resource_parameter(3, 0); //Unified index buffer
	root_signature->add_shader_resource_parameter(4, 0); //Lights buffer
	
	root_signature->add_32bit_constants_parameter(0, 2, sizeof(Shadow_Atlas)); //shadow atals info
	root_signature->add_32bit_constants_parameter(1, 2, sizeof(Jittering_Filter)); //jittering filter info
	
	root_signature->add_shader_resource_parameter(0, 2); //shadow atlas texture
	root_signature->add_shader_resource_parameter(1, 2); //jittering_samples
	root_signature->add_shader_resource_parameter(2, 2); //cascaded_shadows_list
	root_signature->add_shader_resource_parameter(3, 2); //shadow_cascade_view_projection_matrices

	access = ALLOW_VERTEX_SHADER_ACCESS | ALLOW_PIXEL_SHADER_ACCESS;
	Render_Pass::setup_root_signature(device);
}

void Forward_Pass::setup_pipeline(Render_Device *render_device, Shader_Manager *shader_manager)
{
	Graphics_Pipeline_Desc graphics_pipeline_desc;
	graphics_pipeline_desc.root_signature = root_signature;
	graphics_pipeline_desc.vs_bytecode = GET_SHADER(shader_manager, forward_light)->vs_bytecode.bytecode_ref();
	graphics_pipeline_desc.ps_bytecode = GET_SHADER(shader_manager, forward_light)->ps_bytecode.bytecode_ref();
	graphics_pipeline_desc.depth_stencil_format = DXGI_FORMAT_D32_FLOAT;
	graphics_pipeline_desc.add_render_target(DXGI_FORMAT_R8G8B8A8_UNORM);

	pipeline_state = render_device->create_pipeline_state(&graphics_pipeline_desc);
}

inline Viewport make_viewport_from_texture(Texture *texture)
{
	Texture_Desc texture_desc = texture->get_texture_desc();
	return { Size_f32(texture_desc.width, texture_desc.height) };
}

void Forward_Pass::render(Graphics_Command_List *graphics_command_list, void *context, void *args)
{
	Render_World *render_world = (Render_World *)context;
	Render_System *render_sys = (Render_System *)args;

	graphics_command_list->begin_event("Forward rendering");

	graphics_command_list->clear_depth_stencil(render_sys->swap_chain->get_depth_stencil_buffer());
	graphics_command_list->clear_render_target(render_sys->swap_chain->get_back_buffer(), Color::LightSteelBlue);
	graphics_command_list->set_render_target(render_sys->swap_chain->get_back_buffer(), render_sys->swap_chain->get_depth_stencil_buffer());

	graphics_command_list->apply(pipeline_state);
	
	Pipeline_Resource_Manager *pipeline_resource_manager = &render_sys->pipeline_resource_manager;
	
	graphics_command_list->set_graphics_descriptor_table(0, 10, SAMPLER_REGISTER, render_sys->render_device->base_sampler_descriptor());
	graphics_command_list->set_graphics_descriptor_table(0, 10, SHADER_RESOURCE_REGISTER, render_sys->render_device->base_shader_resource_descriptor());

	graphics_command_list->set_graphics_constant_buffer(0, 10, pipeline_resource_manager->global_buffer);
	graphics_command_list->set_graphics_constant_buffer(1, 10, pipeline_resource_manager->frame_info_buffer);

	graphics_command_list->set_viewport(make_viewport_from_texture(render_sys->swap_chain->get_back_buffer()));
	
	graphics_command_list->transition_resource_barrier(shadow_atlas, RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_ALL_SHADER_RESOURCE);

	graphics_command_list->set_graphics_descriptor_table(0, 0, SHADER_RESOURCE_REGISTER, render_world->world_matrices_buffer->shader_resource_descriptor());
	graphics_command_list->set_graphics_descriptor_table(1, 0, SHADER_RESOURCE_REGISTER, render_world->model_storage.mesh_instance_buffer->shader_resource_descriptor());
	graphics_command_list->set_graphics_descriptor_table(2, 0, SHADER_RESOURCE_REGISTER, render_world->model_storage.unified_vertex_buffer->shader_resource_descriptor());
	graphics_command_list->set_graphics_descriptor_table(3, 0, SHADER_RESOURCE_REGISTER, render_world->model_storage.unified_index_buffer->shader_resource_descriptor());
	
	graphics_command_list->set_graphics_descriptor_table(4, 0, SHADER_RESOURCE_REGISTER, render_world->lights_buffer->shader_resource_descriptor());
	graphics_command_list->set_graphics_descriptor_table(0, 2, SHADER_RESOURCE_REGISTER, shadow_atlas->shader_resource_descriptor());
	graphics_command_list->set_graphics_descriptor_table(1, 2, SHADER_RESOURCE_REGISTER, render_world->jittering_samples->shader_resource_descriptor());
	graphics_command_list->set_graphics_descriptor_table(2, 2, SHADER_RESOURCE_REGISTER, render_world->cascaded_shadows_info_buffer->shader_resource_descriptor());
	graphics_command_list->set_graphics_descriptor_table(3, 2, SHADER_RESOURCE_REGISTER, render_world->casded_view_projection_matrices_buffer->shader_resource_descriptor());

	Shadow_Atlas shadow_atlas_info;
	shadow_atlas_info.atlas_size = SHADOW_ATLAS_SIZE;
	shadow_atlas_info.cascade_size = CASCADE_SIZE;

	Jittering_Filter filter;
	filter.tile_size = render_world->jittering_tile_size;
	filter.filter_size = render_world->jittering_filter_size;
	filter.scaling = render_world->jittering_scaling;
	
	graphics_command_list->set_graphics_constants(0, 2, &shadow_atlas_info);
	graphics_command_list->set_graphics_constants(1, 2, &filter);

	Pass_Data pass_data;
	Render_Entity *render_entity = NULL;
	For(render_world->game_render_entities, render_entity) {
		pass_data.parameter0 = render_entity->mesh_idx;
		pass_data.parameter1 = render_entity->world_matrix_idx;
		graphics_command_list->set_graphics_constants(0, 0, &pass_data);

		graphics_command_list->draw(render_world->model_storage.render_models[render_entity->mesh_idx]->mesh.index_count());
	}

	graphics_command_list->transition_resource_barrier(shadow_atlas, RESOURCE_STATE_ALL_SHADER_RESOURCE, RESOURCE_STATE_DEPTH_WRITE);
	graphics_command_list->end_event();
}

void Render_2D_Pass::init(Render_Device *device, Shader_Manager *shader_manager, Pipeline_Resource_Manager *resource_manager)
{
	Render_Pass::init("Render 2D", device, shader_manager, resource_manager);
}

void Render_2D_Pass::schedule_resources(Pipeline_Resource_Manager *resource_manager)
{
}

struct Render_2D_Info {
	Matrix4 orthographics_matrix;
	Vector4 primitive_color;
};

void Render_2D_Pass::setup_root_signature(Render_Device *device)
{
	root_signature->add_32bit_constants_parameter(0, 0, sizeof(Render_2D_Info));
	root_signature->add_shader_resource_parameter(0, 0);
	
	access = ALLOW_INPUT_LAYOUT_ACCESS | ALLOW_VERTEX_SHADER_ACCESS | ALLOW_PIXEL_SHADER_ACCESS;
	Render_Pass::setup_root_signature(device);
}

void Render_2D_Pass::setup_pipeline(Render_Device *render_device, Shader_Manager *shader_manager)
{
	Blending_Desc blending_desc;
	blending_desc.enable = true;
	blending_desc.src = BLEND_SRC_ALPHA;
	blending_desc.dest = BLEND_INV_SRC_ALPHA;
	blending_desc.blend_op = BLEND_OP_ADD;
	blending_desc.src = BLEND_SRC_ALPHA;
	blending_desc.src_alpha = BLEND_ONE;
	blending_desc.dest_alpha = BLEND_INV_SRC_ALPHA;
	blending_desc.blend_op_alpha = BLEND_OP_ADD;

	Depth_Stencil_Desc depth_stencil_desc;
	depth_stencil_desc.enable_depth_test = true;
	depth_stencil_desc.depth_compare_func = COMPARISON_ALWAYS;

	Array<Input_Layout> input_layouts;
	input_layouts.push({ "POSITION", DXGI_FORMAT_R32G32_FLOAT });
	input_layouts.push({ "TEXCOORD", DXGI_FORMAT_R32G32_FLOAT });

	Graphics_Pipeline_Desc graphics_pipeline_desc;
	graphics_pipeline_desc.root_signature = root_signature;
	graphics_pipeline_desc.input_layouts = input_layouts;
	graphics_pipeline_desc.vs_bytecode = GET_SHADER(shader_manager, render_2d)->vs_bytecode.bytecode_ref();
	graphics_pipeline_desc.ps_bytecode = GET_SHADER(shader_manager, render_2d)->ps_bytecode.bytecode_ref();
	graphics_pipeline_desc.blending_desc = blending_desc;
	graphics_pipeline_desc.depth_stencil_desc = depth_stencil_desc;
	graphics_pipeline_desc.depth_stencil_format = DXGI_FORMAT_D32_FLOAT;
	graphics_pipeline_desc.add_render_target(DXGI_FORMAT_R8G8B8A8_UNORM);

	pipeline_state = render_device->create_pipeline_state(&graphics_pipeline_desc);
}

void Render_2D_Pass::render(Graphics_Command_List *graphics_command_list, void *context, void *args)
{
	Render_2D *render_2d = (Render_2D *)context;
	Render_System *render_sys = (Render_System *)args;
	Render_Device *render_device = render_sys->render_device;

	if ((render_2d->total_vertex_count == 0) || !render_2d->initialized) {
		return;
	}
	graphics_command_list->begin_event("Rendering 2D");

	graphics_command_list->set_render_target(render_sys->swap_chain->get_back_buffer(), render_sys->swap_chain->get_depth_stencil_buffer());
	graphics_command_list->set_viewport(make_viewport_from_texture(render_sys->swap_chain->get_back_buffer()));

	graphics_command_list->apply(pipeline_state);

	Pipeline_Resource_Manager *pipeline_resource_manager = &render_sys->pipeline_resource_manager;

	graphics_command_list->set_graphics_descriptor_table(0, 10, SAMPLER_REGISTER, render_sys->render_device->base_sampler_descriptor());
	graphics_command_list->set_graphics_descriptor_table(0, 10, SHADER_RESOURCE_REGISTER, render_sys->render_device->base_shader_resource_descriptor());

	graphics_command_list->set_graphics_constant_buffer(0, 10, pipeline_resource_manager->global_buffer);
	graphics_command_list->set_graphics_constant_buffer(1, 10, pipeline_resource_manager->frame_info_buffer);
	
	graphics_command_list->set_vertex_buffer(render_2d->vertex_buffer);
	graphics_command_list->set_index_buffer(render_2d->index_buffer);

	Render_2D_Info cb_render_info;

	Render_Primitive_List *list = NULL;
	For(render_2d->draw_list, list) {
		Render_Primitive_2D *render_primitive = NULL;
		For(list->render_primitives, render_primitive) {

			graphics_command_list->set_clip_rect(render_primitive->clip_rect);
			cb_render_info.orthographics_matrix = render_primitive->transform_matrix * render_sys->window_view_plane.orthographic_matrix;

			cb_render_info.primitive_color = render_primitive->color.value;
			graphics_command_list->set_graphics_constants(0, 0, &cb_render_info);
			graphics_command_list->set_graphics_descriptor_table(0, 0, SHADER_RESOURCE_REGISTER, render_primitive->texture->shader_resource_descriptor());

			Primitive_2D *primitive = render_primitive->primitive;
			graphics_command_list->draw_indexed(primitive->indices.count, primitive->index_offset, primitive->vertex_offset);
		}
	}
	graphics_command_list->end_event();
	
	For(render_2d->draw_list, list) {
		list->render_primitives.reset();
	}
	render_2d->draw_list.reset();
}

void Silhouette_Pass::add_render_entity_index(u32 entity_index)
{
	render_entity_indices.push(entity_index);
}

void Silhouette_Pass::delete_render_entity_index(u32 entity_index)
{
	for (u32 i = 0; i < render_entity_indices.count; i++) {
		if (render_entity_indices[i] == entity_index) {
			render_entity_indices.remove(i);
		}
	}
}

void Silhouette_Pass::reset_render_entity_indices()
{
	render_entity_indices.count = 0;
}

void Silhouette_Pass::init(Render_Device *device, Shader_Manager *shader_manager, Pipeline_Resource_Manager *resource_manager)
{
	Render_Pass::init("Silhouette", device, shader_manager, resource_manager);
}

void Silhouette_Pass::schedule_resources(Pipeline_Resource_Manager *resource_manager)
{
	Render_Target_Texture_Desc render_target_desc;
	render_target_desc.name = "Silhouette";
	render_target_desc.format = DXGI_FORMAT_R32_UINT;
	render_target_desc.clear_value = Clear_Value(Color(0.0f, 0.0f, 0.0f, 0.0f));
	silhouette = resource_manager->create_render_target("silhouette", &render_target_desc);
	
	Depth_Stencil_Texture_Desc depth_stencil_texture_desc;
	depth_stencil_texture_desc.name = "Silhouette depth";
	depth_stencil_texture_desc.format = DXGI_FORMAT_D32_FLOAT;
	depth_stencil_texture_desc.clear_value = Clear_Value(1.0f, 0);
	silhouette_depth = resource_manager->create_depth_stencil("silhouette_depth", &depth_stencil_texture_desc);
}

void Silhouette_Pass::setup_root_signature(Render_Device *device)
{
	root_signature->add_32bit_constants_parameter(0, 0, sizeof(Pass_Data)); //Pass data
	root_signature->add_shader_resource_parameter(0, 0); //World matrices
	root_signature->add_shader_resource_parameter(1, 0); //Mesh instances
	root_signature->add_shader_resource_parameter(2, 0); //unified vertex buffer
	root_signature->add_shader_resource_parameter(3, 0); //Unified index buffer

	access = ALLOW_VERTEX_SHADER_ACCESS | ALLOW_PIXEL_SHADER_ACCESS;
	Render_Pass::setup_root_signature(device);
}

void Silhouette_Pass::setup_pipeline(Render_Device *render_device, Shader_Manager *shader_manager)
{
	Graphics_Pipeline_Desc graphics_pipeline_desc;
	graphics_pipeline_desc.root_signature = root_signature;
	graphics_pipeline_desc.vs_bytecode = GET_SHADER(shader_manager, silhouette)->vs_bytecode.bytecode_ref();
	graphics_pipeline_desc.ps_bytecode = GET_SHADER(shader_manager, silhouette)->ps_bytecode.bytecode_ref();
	graphics_pipeline_desc.depth_stencil_format = DXGI_FORMAT_D32_FLOAT;
	graphics_pipeline_desc.add_render_target(DXGI_FORMAT_R32_UINT);

	pipeline_state = render_device->create_pipeline_state(&graphics_pipeline_desc);
}

void Silhouette_Pass::render(Graphics_Command_List *graphics_command_list, void *context, void *args)
{
	Render_World *render_world = (Render_World *)context;
	Render_System *render_sys = (Render_System *)args;

	u32 back_buffer_index = render_sys->swap_chain->get_current_back_buffer_index();

	graphics_command_list->begin_event("Silhouette");
	graphics_command_list->clear_render_target(silhouette, Color(0.0f, 0.0f, 0.0f, 0.0f));
	graphics_command_list->clear_depth_stencil(silhouette_depth);

	graphics_command_list->apply(pipeline_state);
	graphics_command_list->set_render_target(silhouette, silhouette_depth);

	graphics_command_list->set_graphics_descriptor_table(0, 10, SAMPLER_REGISTER, render_sys->render_device->base_sampler_descriptor());
	graphics_command_list->set_graphics_descriptor_table(0, 10, SHADER_RESOURCE_REGISTER, render_sys->render_device->base_shader_resource_descriptor());

	Pipeline_Resource_Manager *pipeline_resource_manager = &render_sys->pipeline_resource_manager;

	graphics_command_list->set_graphics_constant_buffer(0, 10, pipeline_resource_manager->global_buffer);
	graphics_command_list->set_graphics_constant_buffer(1, 10, pipeline_resource_manager->frame_info_buffer);

	graphics_command_list->set_viewport(make_viewport_from_texture(render_sys->swap_chain->get_back_buffer()));
	
	graphics_command_list->set_graphics_descriptor_table(0, 0, SHADER_RESOURCE_REGISTER, render_world->world_matrices_buffer->shader_resource_descriptor());
	graphics_command_list->set_graphics_descriptor_table(1, 0, SHADER_RESOURCE_REGISTER, render_world->model_storage.mesh_instance_buffer->shader_resource_descriptor());
	graphics_command_list->set_graphics_descriptor_table(2, 0, SHADER_RESOURCE_REGISTER, render_world->model_storage.unified_vertex_buffer->shader_resource_descriptor());
	graphics_command_list->set_graphics_descriptor_table(3, 0, SHADER_RESOURCE_REGISTER, render_world->model_storage.unified_index_buffer->shader_resource_descriptor());

	Pass_Data pass_data;

	Render_Entity *render_entity = NULL;
	for (u32 i = 0; i < render_entity_indices.count; i++) {
		u32 index = render_entity_indices[i];
		Render_Entity *render_entity = &render_world->game_render_entities[index];

		pass_data.parameter0 = render_entity->mesh_idx;
		pass_data.parameter1 = render_entity->world_matrix_idx;
		pass_data.parameter2 = i + 1;

		graphics_command_list->set_graphics_constants(0, 0, &pass_data);
		graphics_command_list->draw(render_world->model_storage.render_models[render_entity->mesh_idx]->mesh.index_count());
	}	
	graphics_command_list->end_event();
}

void Outlining_Pass::setup_outlining(u32 outlining_size_in_pixels, const Color &color)
{
	u32 samples_in_one_row = outlining_size_in_pixels * 2 + 1;
	pass_data.offset_range = (s32)floor((float)samples_in_one_row * 0.5f);
	pass_data.color = (Color)color;
}

void Outlining_Pass::init(Render_Device *device, Shader_Manager *shader_manager, Pipeline_Resource_Manager *resource_manager)
{
	Render_Pass::init("Outlining", device, shader_manager, resource_manager);
	setup_outlining(2, Color(245, 176, 66));
}

void Outlining_Pass::schedule_resources(Pipeline_Resource_Manager *resource_manager)
{
	Texture_Desc texture_desc;
	texture_desc.resource_state = RESOURCE_STATE_UNORDERED_ACCESS;
	texture_desc.flags = ALLOW_UNORDERED_ACCESS;
	texture_desc.name = "Outlining";

	outlining = resource_manager->create_texture("outlining", &texture_desc);
	silhouette = resource_manager->read_texture("silhouette");
	silhouette_depth = resource_manager->read_texture("silhouette_depth");
}

void Outlining_Pass::setup_root_signature(Render_Device *device)
{
	root_signature->add_32bit_constants_parameter(0, 0, sizeof(Pass_Data));
	root_signature->add_shader_resource_parameter(0, 0);
	root_signature->add_shader_resource_parameter(1, 0);
	root_signature->add_shader_resource_parameter(2, 0);
	root_signature->add_unordered_access_parameter(0, 0);

	access = 0;
	Render_Pass::setup_root_signature(device);
}

void Outlining_Pass::setup_pipeline(Render_Device *render_device, Shader_Manager *shader_manager)
{
	Compute_Pipeline_Desc compute_pipeline_desc;
	compute_pipeline_desc.root_signature = root_signature;
	compute_pipeline_desc.cs_bytecode = GET_SHADER(shader_manager, outlining)->cs_bytecode.bytecode_ref();

	pipeline_state = render_device->create_pipeline_state(&compute_pipeline_desc);
}

void Outlining_Pass::render(Graphics_Command_List *graphics_command_list, void *context, void *args)
{
	Render_World *render_world = (Render_World *)context;
	Render_System *render_sys = (Render_System *)args;

	graphics_command_list->begin_event("Outlining");
	
	graphics_command_list->apply(pipeline_state);

	graphics_command_list->set_compute_constants(0, 0, &pass_data);

	graphics_command_list->set_compute_descriptor_table(0, 0, SHADER_RESOURCE_REGISTER, silhouette->shader_resource_descriptor());
	graphics_command_list->set_compute_descriptor_table(1, 0, SHADER_RESOURCE_REGISTER, silhouette_depth->shader_resource_descriptor());
	graphics_command_list->set_compute_descriptor_table(2, 0, SHADER_RESOURCE_REGISTER, render_sys->swap_chain->get_depth_stencil_buffer()->shader_resource_descriptor());
	graphics_command_list->set_compute_descriptor_table(0, 0, UNORDERED_ACCESS_REGISTER, outlining->unordered_access_descriptor());

	Viewport viewport = make_viewport_from_texture(render_sys->swap_chain->get_back_buffer());

	float compute_shader_thread_number = 32.0f;
	u32 thread_group_count_x = (u32)math::ceil(viewport.width / compute_shader_thread_number);
	u32 thread_group_count_y = (u32)math::ceil(viewport.height / compute_shader_thread_number);

	graphics_command_list->dispatch(thread_group_count_x, thread_group_count_y);

	graphics_command_list->end_event();
}