#include <assert.h>

#include "render_system.h"

#include "../sys/sys.h"
#include "../sys/vars.h"
#include "../sys/engine.h"
#include "../sys/profiling.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/memory/base.h"
#include "../libs/math/functions.h"
#include "../libs/math/structures.h"
#include "../libs/math/3dmath.h"

#include "d3d12_render_api/d3d12_functions.h"


Command_List_Allocator::Command_List_Allocator()
{
}

Command_List_Allocator::~Command_List_Allocator()
{
	
}

void Command_List_Allocator::init(Render_Device *_render_device, u64 frame_start)
{
	frame_number = frame_start;
	render_device = _render_device;
	
	command_list_table[COMMAND_LIST_TYPE_COPY] = &copy_command_lists;
	command_list_table[COMMAND_LIST_TYPE_COMPUTE] = &compute_command_lists;
	command_list_table[COMMAND_LIST_TYPE_DIRECT] = &graphics_command_lists;
}

void Command_List_Allocator::finish_frame(u64 completed_frame)
{
	while (!flight_command_lists.empty() && (flight_command_lists.front().first <= completed_frame)) {
		Command_List *command_list = flight_command_lists.front().second;
		flight_command_lists.pop();
		command_list_table[command_list->type]->push(command_list);
	}
	frame_number++;
}

Command_List *Command_List_Allocator::allocate_command_list(Command_List_Type command_list_type)
{
	Command_List *command_list = NULL;
	if (command_list_table[command_list_type]->is_empty()) {
		command_list = render_device->create_command_list(command_list_type);
	} else {
		command_list = command_list_table[command_list_type]->last();
		command_list_table[command_list_type]->pop();
	}
	flight_command_lists.push({ frame_number, command_list });
	return command_list;
}

void Render_System::init(Win32_Window *win32_window, Variable_Service *variable_service)
{
	assert(win32_window->width > 0);
	assert(win32_window->height > 0);

	window.width = win32_window->width;
	window.height = win32_window->height;
	window.aspect_ration = (float)window.width / (float)window.height;

	u32 back_buffer_count = 2;

	Variable_Service *rendering = variable_service->find_namespace("rendering");
	rendering->attach("vsync", &window.vsync);
	rendering->attach("windowed", &window.windowed);
	rendering->attach("back_buffer_count", (s32 *)&back_buffer_count);

	render_device = create_render_device(back_buffer_count);
	if (!render_device) {
		error("Failed to create render device.");
	}

	frame_fence = render_device->create_fence(back_buffer_count, "Frame fence");

	copy_queue = render_device->create_command_queue(COMMAND_LIST_TYPE_COPY, "Copy Queue");
	compute_queue = render_device->create_command_queue(COMMAND_LIST_TYPE_COMPUTE, "Compute Queue");
	graphics_queue = render_device->create_command_queue(COMMAND_LIST_TYPE_DIRECT, "Graphics Queue");

	bool allow_tearing = false;
	if (!window.vsync && window.windowed && check_tearing_support()) {
		sync_interval = 0;
		present_flags = DXGI_PRESENT_ALLOW_TEARING;
		allow_tearing = true;
	}
	swap_chain = create_swap_chain(allow_tearing, back_buffer_count, window.width, window.height, win32_window->handle, graphics_queue);

	Texture_Desc back_buffer_texture_desc;
	back_buffer_texture_desc.width = window.width;
	back_buffer_texture_desc.height = window.height;
	back_buffer_texture_desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;

	pipeline_resource_manager.init(render_device, &back_buffer_texture_desc);
	command_list_allocator.init(render_device, back_buffer_count);

	render_pass_context.primitive_renderer = &primitive_renderer;
	render_pass_context.mesh_storage = &Engine::get_instance()->mesh_storage;
	render_pass_context.material_storage = &Engine::get_instance()->material_storage;
	render_pass_context.render_world = &Engine::get_instance()->render_world;

	init_passes();
}

void Render_System::init_passes()
{
	primitive_renderer.init(render_device);

	Shader_Manager *shader_manager = &Engine::get_instance()->shader_manager;
	UI_Storage *ui_storage = &Engine::get_instance()->ui_storage;
	Render_World *render_world = Engine::get_render_world();

	passes.shadows_pass.init(render_device, shader_manager, &pipeline_resource_manager);
	passes.forward_pass.init(render_device, shader_manager, &pipeline_resource_manager);
	//passes.debug_shadows_pass.init(render_device, shader_manager, &pipeline_resource_manager);
	passes.silhouette_pass.init(render_device, shader_manager, &pipeline_resource_manager);
	passes.outlining_pass.init(render_device, shader_manager, &pipeline_resource_manager);
	passes.ui_pass.init(render_device, shader_manager, &pipeline_resource_manager);
	
	passes.depth_pass.init(render_device, shader_manager, &pipeline_resource_manager);
	passes.generate_hzb.init(render_device, shader_manager, &pipeline_resource_manager);
	passes.culling_pass.init(render_device, shader_manager, &pipeline_resource_manager);
	passes.primitive_pass.init(render_device, shader_manager, &pipeline_resource_manager);

	render_pass_submissions.push({ &passes.depth_pass, (void *)&render_pass_context,   (void *)this });
	render_pass_submissions.push({ &passes.generate_hzb, (void *)&render_pass_context,   (void *)this });
	render_pass_submissions.push({ &passes.shadows_pass,  (void *)&render_pass_context, (void *)this });
	render_pass_submissions.push({ &passes.culling_pass,  (void *)&render_pass_context, (void *)this });
	render_pass_submissions.push({ &passes.forward_pass,  (void *)&render_pass_context, (void *)this });
	//render_pass_submissions.push({ &passes.debug_shadows_pass,  (void *)&render_pass_context, (void *)this });
	
	render_pass_submissions.push({ &passes.silhouette_pass,  (void *)&render_pass_context, (void *)this });
	render_pass_submissions.push({ &passes.outlining_pass,  (void *)&render_pass_context, (void *)this });

	render_pass_submissions.push({ &passes.ui_pass,  (void *)ui_storage, (void *)this });
	render_pass_submissions.push({ &passes.primitive_pass, (void *)&render_pass_context,   (void *)this });
}

void Render_System::resize(u32 window_width, u32 window_height)
{
	if (Engine::initialized()) {
	}
}

void Render_System::flush()
{
	/*Fence fence;
	fence.create(gpu_device, 1);
	graphics_queue.signal(fence);
	fence.wait_for_gpu();*/
}

void Render_System::notify_start_frame()
{
}

void Render_System::notify_end_frame()
{
	command_list_allocator.finish_frame(frame_fence->expected_value - 1);

	render_device->finish_frame(frame_fence->expected_value - 1);
}

void Render_System::render()
{
	begin_profile_task("Rendering");

	notify_start_frame();

	primitive_renderer.prepare_for_rendering();
	
	pipeline_resource_manager.update_common_constant_buffers();

	Fence *uploading_fence = render_device->execute_uploading();

	Graphics_Command_List *graphics_command_list = static_cast<Graphics_Command_List *>(command_list_allocator.allocate_command_list(COMMAND_LIST_TYPE_DIRECT));
	graphics_command_list->reset();

	graphics_command_list->transition_resource_barrier(swap_chain->get_back_buffer(), RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET);
	
	for (u32 i = 0; i < render_pass_submissions.count; i++) {
		Render_Pass_Submission render_pass_submission = render_pass_submissions[i];
		begin_profile_task(render_pass_submission.render_pass->name);
		render_pass_submission.render_pass->render(graphics_command_list, render_pass_submission.context, render_pass_submission.args);
		end_profile_task();
	}

	graphics_command_list->transition_resource_barrier(swap_chain->get_back_buffer(), RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT);
	graphics_command_list->close();

	graphics_queue->wait(uploading_fence);
	graphics_queue->execute_command_list(graphics_command_list);
	
	begin_profile_task("Present");
	swap_chain->present(sync_interval, present_flags);
	end_profile_task();

	begin_profile_task("Signal");
	graphics_queue->signal(frame_fence);
	end_profile_task();

	begin_profile_task("Wait");
	frame_fence->wait_for_gpu(frame_fence->expected_value - 1);
	end_profile_task();

	begin_profile_task("Finish Frame");
	notify_end_frame();
	end_profile_task();

	frame_fence->increment_expected_value();

	end_profile_task();
}

Size_u32 Render_System::get_window_size()
{
	return { window.width, window.height };
}

void Pipeline_Resource_Manager::init(Render_Device *_render_device, Texture_Desc *back_buffer_texture_desc)
{
	render_device = _render_device;

	default_texture_desc.dimension = TEXTURE_DIMENSION_2D;
	default_texture_desc.width = back_buffer_texture_desc->width;
	default_texture_desc.height = back_buffer_texture_desc->height;
	default_texture_desc.miplevels = find_max_mip_level(default_texture_desc.width, default_texture_desc.height);
	default_texture_desc.depth = back_buffer_texture_desc->depth;
	default_texture_desc.format = back_buffer_texture_desc->format;

	default_render_target_desc.name = "Unknown";
	default_render_target_desc.width = back_buffer_texture_desc->width;
	default_render_target_desc.height = back_buffer_texture_desc->height;
	default_render_target_desc.format = back_buffer_texture_desc->format;
	default_render_target_desc.clear_value = Clear_Value(Color::LightSteelBlue);

	default_depth_stencil_desc.name = "Unknown";
	default_depth_stencil_desc.width = back_buffer_texture_desc->width;
	default_depth_stencil_desc.height = back_buffer_texture_desc->height;
	default_depth_stencil_desc.format = DXGI_FORMAT_D32_FLOAT;
	default_depth_stencil_desc.clear_value = Clear_Value(1.0f, 0);

	anisotropic_sampler = render_device->create_sampler(SAMPLER_FILTER_ANISOTROPIC, ADDRESS_MODE_WRAP);
	linear_sampler = render_device->create_sampler(SAMPLER_FILTER_LINEAR, ADDRESS_MODE_WRAP);
	point_sampler = render_device->create_sampler(SAMPLER_FILTER_POINT, ADDRESS_MODE_WRAP);
	point_clamp_sampler = render_device->create_sampler(SAMPLER_FILTER_POINT, ADDRESS_MODE_CLAMP);

	GPU_Global_Info global_info;
	global_info.anisotropic_sampler_idx = anisotropic_sampler->sampler_descriptor()->index();
	global_info.linear_sampler_idx = linear_sampler->sampler_descriptor()->index();
	global_info.point_sampler_idx = point_sampler->sampler_descriptor()->index();
	global_info.point_clamp_sampler_idx = point_clamp_sampler->sampler_descriptor()->index();
	global_info.hzb_width = round_down_to_power_of_two(back_buffer_texture_desc->width);
	global_info.hzb_height = round_down_to_power_of_two(back_buffer_texture_desc->height);
	global_info.hzb_mips = find_max_mip_level(global_info.hzb_width, global_info.hzb_height);

	Buffer_Desc global_buffer_desc;
	global_buffer_desc.usage = RESOURCE_USAGE_DEFAULT;
	global_buffer_desc.stride = sizeof(GPU_Global_Info) + 256;
	global_buffer_desc.name = "Global Info";

	global_buffer = render_device->create_buffer(&global_buffer_desc);
	global_buffer->request_write();
	global_buffer->write(&global_info, sizeof(GPU_Global_Info), 0, 256);

	Buffer_Desc frame_info_buffer_desc;
	frame_info_buffer_desc.usage = RESOURCE_USAGE_UPLOAD;
	frame_info_buffer_desc.stride = sizeof(GPU_Frame_Info) + 256;
	frame_info_buffer_desc.name = "Frame Info";

	frame_info_buffer = render_device->create_buffer(&frame_info_buffer_desc);
}

void Pipeline_Resource_Manager::update_common_constant_buffers()
{
	Render_System *render_sys = Engine::get_render_system();
	Render_World *render_world = Engine::get_render_world();
	Game_World *game_world = Engine::get_game_world();

	Camera *camera = (Camera *)game_world->get_entity(render_world->camera_id);
	Size_s32 window_size = render_sys->get_window_size();

	GPU_Frame_Info frame_info;
	frame_info.view_matrix = camera->view_matrix;
	frame_info.freeze_view_matrix = camera->get_view_matrix(); // if camera is Editor_Camera and it is frozen, the methods return free_view_matrix.
	frame_info.perspective_matrix = camera->perspective_matrix;
	frame_info.orthographic_matrix = make_orthographic_matrix(0.0f, (float)window_size.width, (float)window_size.height, 0.0f, camera->near_plane, camera->far_plane);
	frame_info.near_plane = camera->near_plane;
	frame_info.far_plane = camera->far_plane;

	frame_info.view_position = camera->position;
	frame_info.view_direction = camera->direction;
	frame_info.light_count = render_world->lights.count;
	
	deriving_frustum_planes(camera->get_view_perspective_matrix(), frame_info.frustum_planes);

	frame_info_buffer->write((void *)&frame_info, sizeof(GPU_Frame_Info), 0, 256);
}

Texture *Pipeline_Resource_Manager::read_texture(const char *texture_name)
{
	Texture *texture = NULL;
	if (!texture_table.get(texture_name, &texture)) {
		error("Pipeline_Resource_Manager::create_texture: Failed to read texture. Texture '{}' doesn't exist.", texture_name);
	}
	return texture;
}

Texture *Pipeline_Resource_Manager::create_texture(const char *texture_name, Texture_Desc *texture_desc)
{
	Texture_Desc filled_texture_desc;
	if (texture_desc) {
		filled_texture_desc.dimension = texture_desc->dimension != TEXTURE_DIMENSION_UNKNOWN ? texture_desc->dimension : default_texture_desc.dimension;
		filled_texture_desc.resource_state = texture_desc->resource_state != RESOURCE_STATE_COMMON ? texture_desc->resource_state : default_texture_desc.resource_state;
		filled_texture_desc.width = texture_desc->width > 0 ? texture_desc->width : default_texture_desc.width;
		filled_texture_desc.height = texture_desc->height > 1 ? texture_desc->height : default_texture_desc.height;
		filled_texture_desc.depth = texture_desc->depth > 1 ? texture_desc->depth : default_texture_desc.depth;
		filled_texture_desc.miplevels = texture_desc->miplevels != 0 ? texture_desc->miplevels : default_texture_desc.miplevels;
		filled_texture_desc.format = texture_desc->format != DXGI_FORMAT_UNKNOWN ? texture_desc->format : default_texture_desc.format;
		filled_texture_desc.flags = texture_desc->flags != 0 ? texture_desc->flags : default_texture_desc.flags;
		filled_texture_desc.data = texture_desc->data ? texture_desc->data : NULL;
		filled_texture_desc.clear_value = texture_desc->clear_value.color_set() ? texture_desc->clear_value : default_texture_desc.clear_value;
		filled_texture_desc.name = texture_desc->name != "Unknown" ? texture_desc->name : default_texture_desc.name;
	} else {
		filled_texture_desc = default_texture_desc;
	}
	Texture *texture = NULL;
	if (!texture_table.get(texture_name, &texture)) {
		texture = render_device->create_texture(&filled_texture_desc);
		texture_table.set(texture_name, texture);
	} else {
		error("Pipeline_Resource_Manager::create_texture: Failed to create texture. Texture '{}' already exists.", texture_name);
	}
	return texture;
}

Texture *Pipeline_Resource_Manager::create_render_target(const char *texture_name, Render_Target_Texture_Desc *render_target_desc)
{
	Render_Target_Texture_Desc filled_render_target_desc;
	if (render_target_desc) {
		filled_render_target_desc.name = !render_target_desc->name.is_empty() ? render_target_desc->name : default_render_target_desc.name;
		filled_render_target_desc.width = render_target_desc->width > 0 ? render_target_desc->width : default_render_target_desc.width;
		filled_render_target_desc.height = render_target_desc->height > 0 ? render_target_desc->height : default_render_target_desc.height;
		filled_render_target_desc.format = render_target_desc->format != DXGI_FORMAT_UNKNOWN ? render_target_desc->format : default_render_target_desc.format;
		filled_render_target_desc.clear_value = render_target_desc->clear_value.depth_stencil_set() ? render_target_desc->clear_value : default_render_target_desc.clear_value;
	} else {
		filled_render_target_desc = default_render_target_desc;
	}
	Texture_Desc render_target_texture_desc;
	render_target_texture_desc.dimension = TEXTURE_DIMENSION_2D;
	render_target_texture_desc.width = filled_render_target_desc.width;
	render_target_texture_desc.height = filled_render_target_desc.height;
	render_target_texture_desc.format = filled_render_target_desc.format;
	render_target_texture_desc.flags = ALLOW_RENDER_TARGET;
	render_target_texture_desc.clear_value = filled_render_target_desc.clear_value;
	render_target_texture_desc.resource_state = RESOURCE_STATE_RENDER_TARGET;
	render_target_texture_desc.name = filled_render_target_desc.name;

	Texture *texture = NULL;
	if (!texture_table.get(texture_name, &texture)) {
		texture = render_device->create_texture(&render_target_texture_desc);
		texture_table.set(texture_name, texture);
	} else {
		error("Pipeline_Resource_Manager::create_render_target: Failed to create render target. Texture '{}' already exists.", texture_name);
	}
	return texture;
}

Texture *Pipeline_Resource_Manager::create_depth_stencil(const char *texture_name, Depth_Stencil_Texture_Desc *depth_stencil_desc)
{
	Depth_Stencil_Texture_Desc filled_depth_stencil_desc;
	if (depth_stencil_desc) {
		filled_depth_stencil_desc.name = !depth_stencil_desc->name.is_empty() ? depth_stencil_desc->name : default_depth_stencil_desc.name;
		filled_depth_stencil_desc.width = depth_stencil_desc->width > 0 ? depth_stencil_desc->width : default_depth_stencil_desc.width;
		filled_depth_stencil_desc.height = depth_stencil_desc->height > 0 ? depth_stencil_desc->height : default_depth_stencil_desc.height;
		filled_depth_stencil_desc.format = depth_stencil_desc->format != DXGI_FORMAT_UNKNOWN ? depth_stencil_desc->format : default_depth_stencil_desc.format;
		filled_depth_stencil_desc.clear_value = depth_stencil_desc->clear_value.depth_stencil_set() ? depth_stencil_desc->clear_value : default_depth_stencil_desc.clear_value;
	} else {
		filled_depth_stencil_desc = default_depth_stencil_desc;
	}
	Texture_Desc depth_texture_desc;
	depth_texture_desc.dimension = TEXTURE_DIMENSION_2D;
	depth_texture_desc.width = filled_depth_stencil_desc.width;
	depth_texture_desc.height = filled_depth_stencil_desc.height;
	depth_texture_desc.format = filled_depth_stencil_desc.format;
	depth_texture_desc.flags = DEPTH_STENCIL_RESOURCE;
	depth_texture_desc.clear_value = filled_depth_stencil_desc.clear_value;
	depth_texture_desc.resource_state = RESOURCE_STATE_DEPTH_WRITE;
	depth_texture_desc.name = texture_name;

	Texture *texture = NULL;
	if (!texture_table.get(texture_name, &texture)) {
		texture = render_device->create_texture(&depth_texture_desc);
		texture_table.set(texture_name, texture);
	} else {
		error("Pipeline_Resource_Manager::create_depth_stencil: Failed to create depth stencil. Texture '{}' already exists.", texture_name);
	}
	return texture;
}

