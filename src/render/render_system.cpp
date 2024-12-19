#include <assert.h>

#include "hlsl.h"
#include "font.h"
#include "render_system.h"
#include "render_helpers.h"

#include "../sys/sys.h"
#include "../sys/vars.h"
#include "../sys/engine.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/math/functions.h"
#include "../libs/math/structures.h"


//void View::update_projection_matries(u32 fov_in_degrees, u32 width, u32 height, float _near_plane, float _far_plane)
//{
//	ratio = (float)width / (float)height;
//	fov = degrees_to_radians(60.0f);
//	near_plane = _near_plane;
//	far_plane = _far_plane;
//	perspective_matrix = XMMatrixPerspectiveFovLH(fov, ratio, near_plane, far_plane);
//	orthogonal_matrix = XMMatrixOrthographicOffCenterLH(0.0f, (float)width, (float)height, 0.0f, near_plane, far_plane);
//}

Descriptor_Heap_Pool::Descriptor_Heap_Pool()
{
}

Descriptor_Heap_Pool::~Descriptor_Heap_Pool()
{
}

void Descriptor_Heap_Pool::allocate_rt_descriptor(Texture &texture)
{
	texture.rt_descriptor = rt_descriptor_heap.place_descriptor(rt_descriptor_count++, texture);
}

void Descriptor_Heap_Pool::allocate_ds_descriptor(Texture &texture)
{
	texture.ds_descriptor = ds_descriptor_heap.place_descriptor(ds_descriptor_count++, texture);
}

void Descriptor_Heap_Pool::allocate_sr_descriptor(Texture &texture)
{
	texture.sr_descriptor = cbsrua_descriptor_heap.place_sr_descriptor(sr_descriptor_count++, texture);
}

void Descriptor_Heap_Pool::allocate_pool(Gpu_Device &device, u32 descriptors_count)
{
	cbsrua_descriptor_heap.create(device, 1000);
	rt_descriptor_heap.create(device, 1000);
	ds_descriptor_heap.create(device, 1000);
	sampler_descriptor_heap.create(device, 1000);
}

void Render_System::init(Win32_Window *win32_window, Variable_Service *variable_service)
{
	assert(win32_window->width > 0);
	assert(win32_window->height > 0);

	init_vars(win32_window, variable_service);

	if (!create_d3d12_gpu_device(gpu_device)) {
	}

	frame_fence.create(gpu_device);
	frame_fence_values.reserve(back_buffer_count);
	back_buffer_textures.reserve(back_buffer_count);
	zero_memory(&frame_fence_values);

	copy_queue.create(gpu_device, COMMAND_LIST_TYPE_COPY);
	compute_queue.create(gpu_device, COMMAND_LIST_TYPE_COMPUTE);
	graphics_queue.create(gpu_device, COMMAND_LIST_TYPE_DIRECT);

	command_list.create(gpu_device, back_buffer_count);

	bool allow_tearing = false;
	if (!window.vsync && window.windowed && check_tearing_support()) {
		sync_interval = 0;
		present_flags = DXGI_PRESENT_ALLOW_TEARING;
		allow_tearing = true;
	}

	swap_chain.create(allow_tearing, back_buffer_count, window.width, window.height, win32_window->handle, graphics_queue);

	descriptors_pool.allocate_pool(gpu_device, 1000);

	for (u32 i = 0; i < back_buffer_count; i++) {
		swap_chain.get_buffer(i, back_buffer_textures[i]);
		descriptors_pool.allocate_rt_descriptor(back_buffer_textures[i]);
	}

	Texture2D_Desc depth_texture_desc;
	depth_texture_desc.width = window.width;
	depth_texture_desc.height = window.height;
	depth_texture_desc.miplevels = 1;
	depth_texture_desc.format = DXGI_FORMAT_D32_FLOAT;
	depth_texture_desc.flags = DEPTH_STENCIL_RESOURCE;
	depth_texture_desc.clear_value = Clear_Value(1.0f, 0);

	back_buffer_depth_texture.create(gpu_device, depth_texture_desc);

	descriptors_pool.allocate_ds_descriptor(back_buffer_depth_texture);
}

void Render_System::init_vars(Win32_Window *win32_window, Variable_Service *variable_service)
{
	window.width = win32_window->width;
	window.height = win32_window->height;

	Variable_Service *rendering = variable_service->find_namespace("rendering");
	rendering->attach("vsync", &window.vsync);
	rendering->attach("windowed", &window.windowed);
	rendering->attach("back_buffer_count", (s32 *)&back_buffer_count);
}

void Render_System::resize(u32 window_width, u32 window_height)
{
	if (Engine::initialized()) {
	}
}

void Render_System::render()
{
	command_list.reset(back_buffer_index);

	Texture &back_buffer = back_buffer_textures[back_buffer_index];

	command_list.resource_barrier(Transition_Resource_Barrier(back_buffer, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET));
	command_list.clear_render_target_view(back_buffer.rt_descriptor, Color::LightSteelBlue);
	command_list.clear_depth_stencil_view(back_buffer_depth_texture.ds_descriptor);
	command_list.resource_barrier(Transition_Resource_Barrier(back_buffer, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT));
	command_list.close();
	graphics_queue.execute_command_list(command_list);

	swap_chain.present(sync_interval, present_flags);
	frame_fence_values[back_buffer_index] = graphics_queue.signal(frame_fence_value, frame_fence);

	back_buffer_index = swap_chain.get_current_back_buffer_index();
	
	frame_fence.wait_for_gpu(frame_fence_values[back_buffer_index]);
}

Size_u32 Render_System::get_window_size()
{
	return { window.width, window.height };
}

void Box_Pass::setup_root_signature()
{
	root_signature.begin_descriptor_table_parameter(0, VISIBLE_TO_VERTEX_SHADER);
	//root_signature.add_descriptor_range(1, world_matrix_cb_desc);
	//root_signature.add_descriptor_range(2, view_matrix_cb_desc);
	//root_signature.add_descriptor_range(3, pers_matrix_cb_desc);
	//root_signature.end_parameter();
}

void Box_Pass::setup_pipeline()
{
}

void Box_Pass::setup()
{
}


void Box_Pass::render()
{
}
