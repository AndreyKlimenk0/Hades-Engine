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

//void View::update_projection_matries(u32 fov_in_degrees, u32 width, u32 height, float _near_plane, float _far_plane)
//{
//	ratio = (float)width / (float)height;
//	fov = degrees_to_radians(60.0f);
//	near_plane = _near_plane;
//	far_plane = _far_plane;
//	perspective_matrix = XMMatrixPerspectiveFovLH(fov, ratio, near_plane, far_plane);
//	orthogonal_matrix = XMMatrixOrthographicOffCenterLH(0.0f, (float)width, (float)height, 0.0f, near_plane, far_plane);
//}

void Render_System::init(Win32_Window *win32_window, Variable_Service *variable_service)
{
	assert(win32_window->width > 0);
	assert(win32_window->height > 0);

	init_vars(win32_window, variable_service);

	window_view_plane.update(60, window.width, window.height, 1.0f, 10000.0f);

	//if (!create_d3d12_gpu_device(gpu_device)) {
	//}

	//frame_fence.create(gpu_device);
	//back_buffer_textures.reserve(back_buffer_count);

	//copy_queue.create(gpu_device, COMMAND_LIST_TYPE_COPY);
	//copy_queue.set_debug_name("Copy Queue");

	//compute_queue.create(gpu_device, COMMAND_LIST_TYPE_COMPUTE);
	//compute_queue.set_debug_name("Compute Queue");

	//graphics_queue.create(gpu_device, COMMAND_LIST_TYPE_DIRECT);
	//graphics_queue.set_debug_name("Graphics Queue");

	//command_buffer.create(gpu_device, back_buffer_count);

	//bool allow_tearing = false;
	//if (!window.vsync && window.windowed && check_tearing_support()) {
	//	sync_interval = 0;
	//	present_flags = DXGI_PRESENT_ALLOW_TEARING;
	//	allow_tearing = true;
	//}
	//swap_chain.create(allow_tearing, back_buffer_count, window.width, window.height, win32_window->handle, graphics_queue);

	//descriptors_pool.allocate_pool(gpu_device, 4000);

	//for (u32 i = 0; i < back_buffer_count; i++) {
	//	back_buffer_textures[i].descriptor_pool = &descriptors_pool;
	//	swap_chain.get_buffer(i, back_buffer_textures[i]);
	//}
	//
	//Texture temp;
	//swap_chain.get_current_buffer(temp);
	//Texture_Desc back_buffer_texture_desc = temp.get_texture_desc();
	//resource_manager.init(gpu_device, &descriptors_pool, &copy_manager, &back_buffer_texture_desc);

	init_passes();

	command_buffer.descriptors_pool = &descriptors_pool;

	copy_manager.init(this);

	begin_frame();
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

void Render_System::init_passes()
{
	Shader_Manager *sm = &Engine::get_instance()->shader_manager;
	generate_mipmaps.init(gpu_device, sm, &resource_manager);

	auto pass = new Box_Pass();
	pass->init("Box pass", gpu_device, sm, &resource_manager);

	auto shadows_pass = new Shadows_Pass();
	shadows_pass->init("Shadows pass", gpu_device, sm, &resource_manager);

	auto forward_pass = new Forward_Pass();
	forward_pass->init("Forward rendering pass", gpu_device, sm, &resource_manager);
	
	passes.push(shadows_pass);
	//passes.push(pass);
	passes.push(forward_pass);
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

/*void wait_for_gpu(Fence *fence, u64 expected_value)
{
	u64 completed_value = fence->get()->GetCompletedValue();
	if (completed_value < expected_value) {
		fence->get()->SetEventOnCompletion(expected_value, fence->handle);
		WaitForSingleObject(fence->handle, INFINITE);
	}
}*/

void Render_System::render()
{
	Render_World *render_world = Engine::get_render_world();

	GPU_Frame_Info frame_info;
	frame_info.view_matrix = render_world->rendering_view.view_matrix;
	frame_info.perspective_matrix = window_view_plane.perspective_matrix;
	frame_info.orthographic_matrix = window_view_plane.orthographic_matrix;
	frame_info.near_plane = window_view_plane.near_plane;
	frame_info.far_plane = window_view_plane.far_plane;

	frame_info.view_position = render_world->rendering_view.position;
	frame_info.view_direction = render_world->rendering_view.direction;
	frame_info.light_count = render_world->lights.count;

	copy_manager.execute_copying();

	resource_manager.update_frame_info_constant_buffer(&frame_info);

	auto *command_list = command_buffer.get_graphics_command_list();

	command_list->reset(back_buffer_index);

	Texture &back_buffer = back_buffer_textures[back_buffer_index];

	command_list->resource_barrier(Transition_Resource_Barrier(back_buffer, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET));

	command_buffer.begin_frame(&back_buffer);
	
	for (u32 i = 0; i < passes.count; i++) {
		passes[i]->render(&command_buffer, render_world, (void *)this);
	}
	
	command_list->resource_barrier(Transition_Resource_Barrier(back_buffer, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT));
	command_list->close();
	graphics_queue.execute_command_list(*command_list);

	swap_chain.present(sync_interval, present_flags);

	graphics_queue.signal(frame_fence);

	back_buffer_index = swap_chain.get_current_back_buffer_index();

	wait_for_gpu(&frame_fence, frame_fence.expected_value - 1);
	
	end_frame(frame_fence.expected_value - 1);

	begin_frame();
}

Size_u32 Render_System::get_window_size()
{
	return { window.width, window.height };
}

void Resource_Manager::init()
{
	//default_render_target_desc.width = back_buffer_texture_desc->width;
	//default_render_target_desc.height = back_buffer_texture_desc->height;
	//default_render_target_desc.format = back_buffer_texture_desc->format;
	//default_render_target_desc.clear_value = Clear_Value(Color::LightSteelBlue);

	//default_depth_stencil_desc.width = back_buffer_texture_desc->width;
	//default_depth_stencil_desc.height = back_buffer_texture_desc->height;
	//default_depth_stencil_desc.format = DXGI_FORMAT_D32_FLOAT;
	//default_depth_stencil_desc.clear_value = Clear_Value(1.0f, 0);

	//Sampler point_sampler = Sampler(SAMPLER_FILTER_POINT, ADDRESS_MODE_WRAP);
	//Sampler linear_sampler = Sampler(SAMPLER_FILTER_LINEAR, ADDRESS_MODE_WRAP);
	//Sampler anisotropic_sampler = Sampler(SAMPLER_FILTER_ANISOTROPIC, ADDRESS_MODE_WRAP);

	//resource_allocator.init(Engine::get_render_system());

	//point_sampler_descriptor = descriptors_pool->allocate_sampler_descriptor(point_sampler);
	//linear_sampler_descriptor = descriptors_pool->allocate_sampler_descriptor(linear_sampler);
	//anisotropic_sampler_descriptor = descriptors_pool->allocate_sampler_descriptor(anisotropic_sampler);

	//Buffer_Desc buffer_desc = Buffer_Desc(sizeof(GPU_Frame_Info), CONSTANT_BUFFER_ALIGNMENT);
	//frame_info_buffer = create_buffer(BUFFER_TYPE_UPLOAD, &buffer_desc);
}

void Resource_Manager::update_frame_info_constant_buffer(GPU_Frame_Info *frame_info)
{
	frame_info_buffer->write(frame_info, sizeof(GPU_Frame_Info), 256);
}

//Buffer *Resource_Manager::create_buffer(Buffer_Type buffer_type, Buffer_Desc *buffer_desc)
//{
//	auto buffer = new Buffer();
//	buffer->create(buffer_type, buffer_desc, &resource_allocator, descriptors_pool, copy_manager);
//	buffer->begin_frame(frame_count);
//	buffers.push(buffer);
//	return buffer;
//}

Texture *Resource_Manager::create_texture(Texture_Desc *texture_desc)
{
	//auto texture = new Texture();
	//texture->create(texture_desc, &resource_allocator, descriptors_pool);
	//textures.push(texture);
	//return texture;
}

Texture *Resource_Manager::find_depth_stencil_texture(const char *texture_name)
{
	//Texture *texture = NULL;
	//if (!texture_table.get(texture_name, &texture)) {
	//	texture = new Texture();
	//	texture_table.set(texture_name, texture);
	//}
	//return texture;
}

Texture *Resource_Manager::create_depth_stencil_texture(const char *texture_name, Depth_Stencil_Texture_Desc *depth_stencil_desc)
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

	//Texture_Desc depth_texture_desc;
	//depth_texture_desc.dimension = TEXTURE_DIMENSION_2D;
	//depth_texture_desc.width = filled_depth_stencil_desc.width;
	//depth_texture_desc.height = filled_depth_stencil_desc.height;
	//depth_texture_desc.format = filled_depth_stencil_desc.format;
	//depth_texture_desc.flags = DEPTH_STENCIL_RESOURCE;
	//depth_texture_desc.clear_value = filled_depth_stencil_desc.clear_value;
	//depth_texture_desc.resource_state = RESOURCE_STATE_DEPTH_WRITE;

	//Texture *texture = NULL;
	//if (!texture_table.get(texture_name, &texture)) {
	//	texture = new Texture();
	//	texture_table.set(texture_name, texture);
	//	texture->create(&depth_texture_desc, &resource_allocator, descriptors_pool);
	//}
	//
	//return texture;
}

void GPU_Linear_Allocator::create(Gpu_Device &device, u64 heap_size_in_bytes, GPU_Heap_Type heap_type, GPU_Heap_Content content)
{
	heap_size = heap_size_in_bytes;
	heap.create(device, heap_size, heap_type, content);
}

u64 GPU_Linear_Allocator::allocate(u64 size, u64 alignment)
{
	assert((heap_offset + size + alignment) <= heap_size);
	u64 offset = align_address(heap_offset, alignment);
	heap_offset += size + alignment;
	return offset;
}

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

Copy_Manager::Copy_Manager()
{
}

Copy_Manager::~Copy_Manager()
{
}

void Copy_Manager::init(Render_System *render_system)
{
	render_sys = render_system;
	copy_fence.create(render_system->gpu_device, 1);
	copy_command_list.create(render_sys->gpu_device, render_sys->back_buffer_count);
}

void Copy_Manager::add_copy_command(Copy_Command *copy_command)
{
	copy_commands.push(*copy_command);
}

void Copy_Manager::execute_copying()
{
	if (!copy_commands.is_empty()) {
		copy_command_list.reset(render_sys->back_buffer_index);

		for (u32 i = 0; i < copy_commands.count; i++) {
			Copy_Command *copy_command = &copy_commands[i];
			if (copy_command->resource_type == BUFFER_RESOURCE) {
				copy_command_list.copy_resources(*copy_command->dest_resource, *copy_command->source_resource);
			}
		}
		copy_commands.reset();

		copy_command_list.close();
		render_sys->copy_queue.execute_command_list(copy_command_list);
		copy_fence.increment_expected_value();
		render_sys->copy_queue.signal(copy_fence);
		render_sys->graphics_queue.wait(copy_fence);
	}
}

void Resource_Allocator::init(Render_System *render_sys)
{
	Gpu_Device &gpu_device = render_sys->gpu_device;

	default_heap_size = megabytes_to_bytes(500);
	upload_heap_size = megabytes_to_bytes(500);
	texture_heap_size = megabytes_to_bytes(1500);


	default_buffer_heap.create(gpu_device, default_heap_size, GPU_HEAP_TYPE_DEFAULT, GPU_HEAP_CONTAIN_BUFFERS);
	upload_buffer_heap.create(gpu_device, upload_heap_size, GPU_HEAP_TYPE_UPLOAD, GPU_HEAP_CONTAIN_BUFFERS);
	texture_heap.create(gpu_device, texture_heap_size, GPU_HEAP_TYPE_DEFAULT, GPU_HEAP_CONTAIN_BUFFERS_AND_TEXTURES);
}

Resource_Allocation Resource_Allocator::allocate_buffer(Buffer_Type buffer_type, u64 allocation_size)
{
	Resource_Allocation resource_allocation;
	u64 aligned_allozation_size = align_address<u64>(allocation_size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	if (buffer_type == BUFFER_TYPE_DEFAULT) {
		assert((default_buffer_offset + aligned_allozation_size) <= default_heap_size);

		resource_allocation.heap = &default_buffer_heap;
		resource_allocation.heap_offset = default_buffer_offset;
		default_buffer_offset += aligned_allozation_size;
	} else if (buffer_type == BUFFER_TYPE_UPLOAD) {
		assert((upload_buffer_offset + aligned_allozation_size) <= upload_heap_size);

		resource_allocation.heap = &upload_buffer_heap;
		resource_allocation.heap_offset = upload_buffer_offset;
		upload_buffer_offset += aligned_allozation_size;
	} else {
		assert(false);
	}
	return resource_allocation;
}

Resource_Allocation Resource_Allocator::allocate_texture(u64 texture_size)
{
	assert((texture_offset + texture_size) <= texture_heap_size);

	Resource_Allocation resource_allocation;
	resource_allocation.heap = &texture_heap;
	resource_allocation.heap_offset = texture_offset;
	texture_offset += texture_size;
	return resource_allocation;
}