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

struct VertexP3C3 {
	Vector3 vertex;
	Vector3 color;
};

static VertexP3C3 vertices[8] = {
	{ Vector3(-1.0f, -1.0f, -1.0f), Vector3(0.0f, 0.0f, 0.0f) }, // 0
	{ Vector3(-1.0f,  1.0f, -1.0f), Vector3(0.0f, 1.0f, 0.0f) }, // 1
	{ Vector3(1.0f,  1.0f, -1.0f), Vector3(1.0f, 1.0f, 0.0f) }, // 2
	{ Vector3(1.0f, -1.0f, -1.0f), Vector3(1.0f, 0.0f, 0.0f) }, // 3
	{ Vector3(-1.0f, -1.0f,  1.0f), Vector3(0.0f, 0.0f, 1.0f) }, // 4
	{ Vector3(-1.0f,  1.0f,  1.0f), Vector3(0.0f, 1.0f, 1.0f) }, // 5
	{ Vector3(1.0f,  1.0f,  1.0f), Vector3(1.0f, 1.0f, 1.0f) }, // 6
	{ Vector3(1.0f, -1.0f,  1.0f), Vector3(1.0f, 0.0f, 1.0f) }  // 7
};

static u32 indices[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

Descriptor_Heap_Pool::Descriptor_Heap_Pool()
{
}

Descriptor_Heap_Pool::~Descriptor_Heap_Pool()
{
}

void Descriptor_Heap_Pool::allocate_cb_descriptor(Buffer &buffer)
{
	buffer.cb_descriptor = cbsrua_descriptor_heap.place_cb_descriptor(cbsrua_descriptor_count++, buffer);
}

void Descriptor_Heap_Pool::allocate_sr_descriptor(Buffer &buffer)
{
	buffer.sr_descriptor = cbsrua_descriptor_heap.place_sr_descriptor(cbsrua_descriptor_count++, buffer);
}

void Descriptor_Heap_Pool::allocate_ua_descriptor(Buffer &buffer)
{
	buffer.ua_descriptor = cbsrua_descriptor_heap.place_ua_descriptor(cbsrua_descriptor_count++, buffer);
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
	texture.sr_descriptor = cbsrua_descriptor_heap.place_sr_descriptor(cbsrua_descriptor_count++, texture);
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

	command_buffer.create(gpu_device, back_buffer_count);

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

	init_passes();
	pipeline_resource_storage.init(gpu_device, back_buffer_count, &descriptors_pool);

	init_buffers();

	command_buffer.back_buffer_depth_texture = &back_buffer_depth_texture;
	command_buffer.descriptors_pool = &descriptors_pool;
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
	frame_passes.push(&passes.box);
	Shader_Manager *shader_manager = &Engine::get_instance()->shader_manager;
	for (u32 i = 0; i < frame_passes.count; i++) {
		frame_passes[i]->init(gpu_device, shader_manager, &pipeline_resource_storage);
	}
}

void Render_System::init_buffers()
{
	vertex_buffer.create(gpu_device, GPU_HEAP_TYPE_DEFAULT, 8, sizeof(VertexP3C3));
	index_buffer.create(gpu_device, GPU_HEAP_TYPE_DEFAULT, 36, sizeof(u32));

	Buffer intermediate_vertex_buffer;
	Buffer intermediate_index_buffer;
	intermediate_vertex_buffer.create(gpu_device, GPU_HEAP_TYPE_UPLOAD, 8, sizeof(VertexP3C3));
	intermediate_index_buffer.create(gpu_device, GPU_HEAP_TYPE_UPLOAD, 36, sizeof(u32));

	void *temp_vertex_buffer = (void *)intermediate_vertex_buffer.map();
	memcpy(temp_vertex_buffer, (void *)vertices, sizeof(VertexP3C3) * 8);
	intermediate_vertex_buffer.unmap();
	
	void *temp_index_buffer = (void *)intermediate_index_buffer.map();
	memcpy(temp_index_buffer, (void *)indices, sizeof(u32) * 36);
	intermediate_index_buffer.unmap();

	Fence fence;
	fence.create(gpu_device);
	u64 fence_value = 0;

	Copy_Command_List copy_command_list;
	copy_command_list.create(gpu_device, 1);
	copy_command_list.reset(0);

	copy_command_list.get()->CopyResource(vertex_buffer.get(), intermediate_vertex_buffer.get());
	copy_command_list.get()->CopyResource(index_buffer.get(), intermediate_index_buffer.get());

	copy_command_list.close();
	copy_queue.execute_command_list(copy_command_list);
	auto new_fence_value = copy_queue.signal(fence_value, fence);
	fence.wait_for_gpu(new_fence_value);
}

void Render_System::resize(u32 window_width, u32 window_height)
{
	if (Engine::initialized()) {
	}
}

void Render_System::render()
{
	pipeline_resource_storage.begin_frame(back_buffer_index);

	auto *command_list = command_buffer.get_graphics_command_list();

	command_list->reset(back_buffer_index);

	Texture &back_buffer = back_buffer_textures[back_buffer_index];

	command_list->resource_barrier(Transition_Resource_Barrier(back_buffer, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET));
	command_list->clear_render_target_view(back_buffer.rt_descriptor, Color::LightSteelBlue);
	command_list->clear_depth_stencil_view(back_buffer_depth_texture.ds_descriptor);

	command_buffer.begin_frame(&back_buffer);
	
	Render_World *render_world = Engine::get_render_world();
	for (u32 i = 0; i < frame_passes.count; i++) {
		frame_passes[i]->render(&command_buffer, render_world, (void *)this);
	}
	
	command_list->resource_barrier(Transition_Resource_Barrier(back_buffer, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT));
	command_list->close();
	graphics_queue.execute_command_list(*command_list);

	swap_chain.present(sync_interval, present_flags);
	frame_fence_values[back_buffer_index] = graphics_queue.signal(frame_fence_value, frame_fence);

	back_buffer_index = swap_chain.get_current_back_buffer_index();
	
	frame_fence.wait_for_gpu(frame_fence_values[back_buffer_index]);
}

Size_u32 Render_System::get_window_size()
{
	return { window.width, window.height };
}

void Pipeline_Resource_Storage::init(Gpu_Device &device, u32 _back_buffer_count, Descriptor_Heap_Pool *_descriptor_heap_pool)
{
	assert(_back_buffer_count > 0);

	back_buffer_count = _back_buffer_count;
	descriptor_heap_pool = _descriptor_heap_pool;

	u32 heap_size = 0;
	for (u32 i = 0; i < cpu_buffers.count; i++) {
		heap_size += cpu_buffers[i].second;
	}

	buffers_heap.create(device, heap_size * back_buffer_count * 10000, GPU_HEAP_TYPE_UPLOAD, GPU_HEAP_CONTAIN_BUFFERS);
	
	for (u32 i = 0; i < cpu_buffers.count; i++) {
		u32 buffer_size = cpu_buffers[i].second;
		CPU_Buffer *cpu_buffer = cpu_buffers[i].first;
		cpu_buffers[i].first->create(device, buffers_heap, back_buffer_count, 1, buffer_size);
		for (u32 j = 0; j < back_buffer_count; j++) {
			descriptor_heap_pool->allocate_cb_descriptor(*cpu_buffer->buffers[j]);
		}
	}
}

void Pipeline_Resource_Storage::begin_frame(u32 _back_buffer_index)
{
	back_buffer_index = _back_buffer_index;
	for (u32 i = 0; i < cpu_buffers.count; i++) {
		cpu_buffers[i].first->begin_frame(back_buffer_index);
	}
}

CPU_Buffer *Pipeline_Resource_Storage::request_constant_buffer(u32 buffer_size)
{
	CPU_Buffer *buffer = new CPU_Buffer();
	cpu_buffers.push({ buffer, buffer_size });
	return buffer;
}

void CPU_Buffer::begin_frame(u32 _frame_index)
{
	frame_index = _frame_index;
}

void CPU_Buffer::write(void *data, u32 data_size)
{
	Buffer *buffer = get_frame_resource();
	u8 *ptr = buffer->map();
	memcpy((void *)ptr, data, data_size);
	buffer->unmap();
}

void CPU_Buffer::create(Gpu_Device &device, GPU_Heap &heap, u32 frames_in_flight, u32 number_items, u32 item_size)
{
	buffers.reserve(frames_in_flight);
	for (u32 i = 0; i < frames_in_flight; i++) {
		buffers[i] = new Buffer();
		buffers[i]->create(device, heap, number_items, item_size);
	}
}

Buffer *CPU_Buffer::get_frame_resource()
{
	return buffers[frame_index];
}

void Render_Command_Buffer::create(Gpu_Device &device, u32 frames_in_flight)
{
	graphics_command_list.create(device, frames_in_flight);
}

void Render_Command_Buffer::apply(Pipeline_State &pipeline_state, u32 flags)
{
	graphics_command_list.get()->SetGraphicsRootSignature(pipeline_state.root_signature->get());
	graphics_command_list.get()->SetPipelineState(pipeline_state.get());
	
	graphics_command_list.get()->SetDescriptorHeaps(1, descriptors_pool->cbsrua_descriptor_heap.get_address());
	
	graphics_command_list.set_primitive_type(pipeline_state.primitive_type);
	graphics_command_list.set_viewport(pipeline_state.viewport);
	graphics_command_list.set_clip_rect(pipeline_state.clip_rect);

	if (flags & RENDER_TARGET_BUFFER_BUFFER) {
		graphics_command_list.get()->OMSetRenderTargets(1, &back_buffer_texture->rt_descriptor.cpu_handle, FALSE, &back_buffer_depth_texture->ds_descriptor.cpu_handle);
	} else {
	}
}

void Render_Command_Buffer::begin_frame(Texture *back_buffer)
{
	back_buffer_texture = back_buffer;
}

void Render_Command_Buffer::draw(u32 index_count)
{
	graphics_command_list.get()->DrawIndexedInstanced(index_count, 1, 0, 0, 0);
}

Graphics_Command_List *Render_Command_Buffer::get_graphics_command_list()
{
	return &graphics_command_list;
}
