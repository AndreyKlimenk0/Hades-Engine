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

struct Mipmaps_Info {
	Mipmaps_Info() = default;
	Mipmaps_Info(u32 source_mip_level, u32 number_mip_levels, float texel_width, float texel_height) : source_mip_level(source_mip_level), number_mip_levels(number_mip_levels), texel_size(texel_width, texel_height) {}
	~Mipmaps_Info() = default;

	u32 source_mip_level;
	u32 number_mip_levels;
	Vector2 texel_size;
};

//void Generate_Mipmaps::init(Render_Device *device, Shader_Manager *shader_manager, Resource_Manager *resource_manager)
//{
//	setup_root_signature(device);
//	setup_pipeline(device, shader_manager);
//}
//
//void Generate_Mipmaps::setup_pipeline(Render_Device *device, Shader_Manager *shader_manager)
//{
//	Shader *shaders[4];
//	shaders[0] = GET_SHADER(shader_manager, generate_mips_linear);
//	shaders[1] = GET_SHADER(shader_manager, generate_mips_linear_oddx);
//	shaders[2] = GET_SHADER(shader_manager, generate_mips_linear_oddy);
//	shaders[3] = GET_SHADER(shader_manager, generate_mips_linear_odd);
//
//	for (u32 pipeline_index = 0; pipeline_index < 4; pipeline_index++) {
//		Compute_Pipeline_Desc compute_pipeline_desc;
//		compute_pipeline_desc.compute_shader = shaders[pipeline_index];
//		compute_pipeline_desc.root_signature = &root_signature;
//		pipeline_states[pipeline_index].create(device, compute_pipeline_desc);
//	}
//}
//
//void Generate_Mipmaps::setup_root_signature(Render_Device *device)
//{
//	root_signature.add_32bit_constants_parameter(0, 0, sizeof(Mipmaps_Info));
//	root_signature.add_sr_descriptor_table_parameter(0, 0);
//	root_signature.add_ua_descriptor_table_parameter(0, 0, 4);
//	root_signature.add_sampler_descriptor_table_parameter(0, 0);
//
//	root_signature.create(device);
//}
//
//void Generate_Mipmaps::generate(Compute_Command_List *compute_command_list, Array<Texture *> &textures, Render_System *render_sys)
//{
//	compute_command_list->set_compute_root_signature(root_signature);
//	compute_command_list->set_descriptor_heaps(render_sys->descriptors_pool.cbsrua_descriptor_heap, render_sys->descriptors_pool.sampler_descriptor_heap);
//	compute_command_list->set_compute_root_descriptor_table(3, render_sys->resource_manager.linear_sampler_descriptor);
//
//	for (u32 i = 0; i < textures.count; i++) {
//		Texture *texture = textures[i];
//
//		Texture_Desc texture_desc = texture->get_texture_desc();
//		texture_desc.dimension = TEXTURE_DIMENSION_2D;
//		if ((texture_desc.width == 0) || (texture_desc.height == 0)) {
//			continue;
//		}
//
//		if (texture_desc.miplevels <= 1) {
//			continue;
//		}
//
//		u32 x = 0;
//		for (u32 mip_level = 0; mip_level < texture_desc.miplevels; mip_level++) {
//			texture->get_unordered_access_descriptor(mip_level);
//		}
//
//		u32 prev_pipeline_index = UINT_MAX;
//		for (u32 mip_level = 0; mip_level < texture_desc.miplevels - 1;) {
//			u32 source_width = texture_desc.width >> mip_level;
//			u32 source_height = texture_desc.height >> mip_level;
//			u32 dest_width = source_width >> 1;
//			u32 dest_height = source_height >> 1;
//
//			u32 pipeline_index = (source_width & 1) | (source_height & 1) << 1;
//			if (prev_pipeline_index != pipeline_index) {
//				prev_pipeline_index = pipeline_index;
//				compute_command_list->set_pipeline_state(pipeline_states[pipeline_index]);
//			}
//
//			uint32_t AdditionalMips;
//			_BitScanForward((unsigned long *)&AdditionalMips,
//							(dest_width == 1 ? dest_height : dest_width) | (dest_height == 1 ? dest_width : dest_height));
//			uint32_t number_mips = 1 + (AdditionalMips > 3 ? 3 : AdditionalMips);
//			if (mip_level + number_mips > texture_desc.miplevels)
//				number_mips = texture_desc.miplevels - mip_level;
//
//			if (dest_width == 0)
//				dest_width = 1;
//			if (dest_height == 0)
//				dest_height = 1;
//			
//			compute_command_list->set_compute_constants(0, Mipmaps_Info(mip_level, number_mips, 1.0f / dest_width, 1.0f / dest_height));
//			compute_command_list->set_compute_root_descriptor_table(1, texture->get_shader_resource_descriptor());
//			compute_command_list->set_compute_root_descriptor_table(2, texture->get_unordered_access_descriptor(mip_level + 1));
//			compute_command_list->dispatch(dest_width, dest_height);
//
//			mip_level += number_mips;
//		}
//	}
//}

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
	
	shadow_atlas = resource_manager->create_depth_stencil_texture("Shadow_Atlas", &depth_stencil_desc);
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
	shadow_atlas = resource_manager->create_depth_stencil_texture("Shadow_Atlas");
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
	Shader *shader = GET_SHADER(shader_manager, forward_light);

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
	pipeline_resource_manager->global_buffer;
	
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