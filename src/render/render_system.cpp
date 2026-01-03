#include <assert.h>

#include "font.h"
#include "render_system.h"

#include "../sys/sys.h"
#include "../sys/vars.h"
#include "../sys/engine.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/memory/base.h"
#include "../libs/math/functions.h"
#include "../libs/math/structures.h"

#include "d3d12_render_api/d3d12_functions.h"


void View_Plane::update(u32 _fov, u32 _width, u32 _height, float _near_plane, float _far_plane)
{
	width = _width;
	height = _height;
	ratio = (float)width / (float)height;
	fov = degrees_to_radians((float)_fov);
	near_plane = _near_plane;
	far_plane = _far_plane;
	perspective_matrix = XMMatrixPerspectiveFovLH(fov, ratio, near_plane, far_plane);
	orthographic_matrix = XMMatrixOrthographicOffCenterLH(0.0f, (float)width, (float)height, 0.0f, near_plane, far_plane);
}

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

	u32 back_buffer_count = 2;

	Variable_Service *rendering = variable_service->find_namespace("rendering");
	rendering->attach("vsync", &window.vsync);
	rendering->attach("windowed", &window.windowed);
	rendering->attach("back_buffer_count", (s32 *)&back_buffer_count);

	window_view_plane.update(60, window.width, window.height, 1.0f, 10000.0f);

	render_device = create_render_device(back_buffer_count);
	if (!render_device) {
		error("Failed to create render device.");
	}

	frame_fence = render_device->create_fence(back_buffer_count);

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

	init_passes();
}

void Render_System::init_passes()
{
	Shader_Manager *shader_manager = &Engine::get_instance()->shader_manager;

	passes.shadows_pass.init(render_device, shader_manager, &pipeline_resource_manager);
	passes.forward_pass.init(render_device, shader_manager, &pipeline_resource_manager);

	render_passes_list.push(&passes.shadows_pass);
	render_passes_list.push(&passes.forward_pass);
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

//void wait_for_gpu(Fence *fence, u64 expected_value)
//{
//	u64 completed_value = fence->get()->GetCompletedValue();
//	if (completed_value < expected_value) {
//		fence->get()->SetEventOnCompletion(expected_value, fence->handle);
//		WaitForSingleObject(fence->handle, INFINITE);
//	}
//}

void Render_System::render()
{
	notify_start_frame();

	pipeline_resource_manager.update_common_constant_buffers();

	Fence *uploading_fence = render_device->execute_uploading();

	Graphics_Command_List *graphics_command_list = static_cast<Graphics_Command_List *>(command_list_allocator.allocate_command_list(COMMAND_LIST_TYPE_DIRECT));
	graphics_command_list->reset();

	graphics_command_list->transition_resource_barrier(swap_chain->get_back_buffer(), RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET);
	
	Render_World *render_world = Engine::get_render_world();
	for (u32 i = 0; i < render_passes_list.count; i++) {
		render_passes_list[i]->render(graphics_command_list, (void *)render_world, (void *)this);
	}

	graphics_command_list->transition_resource_barrier(swap_chain->get_back_buffer(), RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT);
	graphics_command_list->close();

	graphics_queue->wait(uploading_fence);
	graphics_queue->execute_command_list(graphics_command_list);
	
	swap_chain->present(sync_interval, present_flags);

	graphics_queue->signal(frame_fence);

	frame_fence->wait_for_gpu(frame_fence->expected_value - 1);

	notify_end_frame();

	frame_fence->increment_expected_value();
}

Size_u32 Render_System::get_window_size()
{
	return { window.width, window.height };
}

void Pipeline_Resource_Manager::init(Render_Device *_render_device, Texture_Desc *back_buffer_texture_desc)
{
	render_device = _render_device;

	default_render_target_desc.width = back_buffer_texture_desc->width;
	default_render_target_desc.height = back_buffer_texture_desc->height;
	default_render_target_desc.format = back_buffer_texture_desc->format;
	default_render_target_desc.clear_value = Clear_Value(Color::LightSteelBlue);

	default_depth_stencil_desc.width = back_buffer_texture_desc->width;
	default_depth_stencil_desc.height = back_buffer_texture_desc->height;
	default_depth_stencil_desc.format = DXGI_FORMAT_D32_FLOAT;
	default_depth_stencil_desc.clear_value = Clear_Value(1.0f, 0);

	anisotropic_sampler = render_device->create_sampler(SAMPLER_FILTER_ANISOTROPIC, ADDRESS_MODE_WRAP);
	linear_sampler = render_device->create_sampler(SAMPLER_FILTER_LINEAR, ADDRESS_MODE_WRAP);
	point_sampler = render_device->create_sampler(SAMPLER_FILTER_POINT, ADDRESS_MODE_WRAP);

	GPU_Global_Info global_info;
	global_info.anisotropic_sampler_idx = anisotropic_sampler->sampler_descriptor()->index();
	global_info.linear_sampler_idx = linear_sampler->sampler_descriptor()->index();
	global_info.point_sampler_idx = point_sampler->sampler_descriptor()->index();

	Buffer_Desc global_buffer_desc;
	global_buffer_desc.usage = RESOURCE_USAGE_DEFAULT;
	global_buffer_desc.stride = sizeof(GPU_Global_Info) + 256;
	global_buffer_desc.name = "Global Info";

	global_buffer = render_device->create_buffer(&global_buffer_desc);
	global_buffer->request_write();
	global_buffer->write(&global_info, sizeof(GPU_Global_Info), 256);

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

	GPU_Frame_Info frame_info;
	frame_info.view_matrix = render_world->rendering_view.view_matrix;
	frame_info.perspective_matrix = render_sys->window_view_plane.perspective_matrix;
	frame_info.orthographic_matrix = render_sys->window_view_plane.orthographic_matrix;
	frame_info.near_plane = render_sys->window_view_plane.near_plane;
	frame_info.far_plane = render_sys->window_view_plane.far_plane;

	frame_info.view_position = render_world->rendering_view.position;
	frame_info.view_direction = render_world->rendering_view.direction;
	frame_info.light_count = render_world->lights.count;

	frame_info_buffer->write((void *)&frame_info, sizeof(GPU_Frame_Info), 256);
}

//Buffer *Resource_Manager::create_buffer(Buffer_Type buffer_type, Buffer_Desc *buffer_desc)
//{
//	auto buffer = new Buffer();
//	buffer->create(buffer_type, buffer_desc, &resource_allocator, descriptors_pool, copy_manager);
//	buffer->begin_frame(frame_count);
//	buffers.push(buffer);
//	return buffer;
//}

//Texture *Pipeline_Resource_Manager::create_texture(Texture_Desc *texture_desc)
//{
//	auto texture = new Texture();
//	texture->create(texture_desc, &resource_allocator, descriptors_pool);
//	textures.push(texture);
//	return texture;
//}

//Texture *Pipeline_Resource_Manager::find_depth_stencil_texture(const char *texture_name)
//{
	//Texture *texture = NULL;
	//if (!texture_table.get(texture_name, &texture)) {
	//	texture = new Texture();
	//	texture_table.set(texture_name, texture);
	//}
	//return texture;
//}

Texture *Pipeline_Resource_Manager::create_depth_stencil_texture(const char *texture_name, Depth_Stencil_Texture_Desc *depth_stencil_desc)
{
	Depth_Stencil_Texture_Desc filled_depth_stencil_desc;
	if (depth_stencil_desc) {
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

	Texture *texture = NULL;
	if (!texture_table.get(texture_name, &texture)) {
		texture = render_device->create_texture(&depth_texture_desc);
		texture_table.set(texture_name, texture);
	}
	return texture;
}

