#include <assert.h>

#include "font.h"
#include "render_system.h"
#include "render_helpers.h"

#include "../sys/sys.h"
#include "../sys/vars.h"
#include "../sys/memory.h"
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
	VertexP3C3() = default;
	VertexP3C3(float x, float y, float z, float uvx, float uvy) : vertex(x, y, z), uv(uvx, uvy) {}
	~VertexP3C3() = default;
	Vector3 vertex;
	Vector2 uv;
};

static VertexP3C3 vertices[8] = {
	// Front face
	{-0.5f, -0.5f,  0.5f, 0.0f, 0.0f}, // 0 - Bottom-left
	{ 0.5f, -0.5f,  0.5f, 1.0f, 0.0f}, // 1 - Bottom-right
	{ 0.5f,  0.5f,  0.5f, 1.0f, 1.0f}, // 2 - Top-right
	{-0.5f,  0.5f,  0.5f, 0.0f, 1.0f}, // 3 - Top-left

	// Back face
	{-0.5f, -0.5f, -0.5f, 1.0f, 0.0f}, // 4 - Bottom-left
	{ 0.5f, -0.5f, -0.5f, 0.0f, 0.0f}, // 5 - Bottom-right
	{ 0.5f,  0.5f, -0.5f, 0.0f, 1.0f}, // 6 - Top-right
	{-0.5f,  0.5f, -0.5f, 1.0f, 1.0f}  // 7 - Top-left
};

//static VertexP3C3 vertices[8] = {
//	{ Vector3(-1.0f, -1.0f, -1.0f), Vector3(0.0f, 0.0f, 0.0f) }, // 0
//	{ Vector3(-1.0f,  1.0f, -1.0f), Vector3(0.0f, 1.0f, 0.0f) }, // 1
//	{ Vector3(1.0f,  1.0f, -1.0f), Vector3(1.0f, 1.0f, 0.0f) }, // 2
//	{ Vector3(1.0f, -1.0f, -1.0f), Vector3(1.0f, 0.0f, 0.0f) }, // 3
//	{ Vector3(-1.0f, -1.0f,  1.0f), Vector3(0.0f, 0.0f, 1.0f) }, // 4
//	{ Vector3(-1.0f,  1.0f,  1.0f), Vector3(0.0f, 1.0f, 1.0f) }, // 5
//	{ Vector3(1.0f,  1.0f,  1.0f), Vector3(1.0f, 1.0f, 1.0f) }, // 6
//	{ Vector3(1.0f, -1.0f,  1.0f), Vector3(1.0f, 0.0f, 1.0f) }  // 7
//};
//
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

CB_Descriptor Descriptor_Heap_Pool::allocate_cb_descriptor(GPU_Resource *resource)
{
	return cbsrua_descriptor_heap.place_cb_descriptor(cbsrua_descriptor_indices.pop(), *resource);
}

SR_Descriptor Descriptor_Heap_Pool::allocate_sr_descriptor(GPU_Resource *resource)
{
	return cbsrua_descriptor_heap.place_sr_descriptor(cbsrua_descriptor_indices.pop(), *resource);
}

UA_Descriptor Descriptor_Heap_Pool::allocate_ua_descriptor(GPU_Resource *resource, u32 mip_level)
{
	return cbsrua_descriptor_heap.place_ua_descriptor(cbsrua_descriptor_indices.pop(), *resource, mip_level);
}

RT_Descriptor Descriptor_Heap_Pool::allocate_rt_descriptor(GPU_Resource *resource)
{
	return rt_descriptor_heap.place_descriptor(rt_descriptor_indices.pop(), *resource);
}

DS_Descriptor Descriptor_Heap_Pool::allocate_ds_descriptor(GPU_Resource *resource)
{
	return ds_descriptor_heap.place_descriptor(rt_descriptor_indices.pop(), *resource);
}

Sampler_Descriptor Descriptor_Heap_Pool::allocate_sampler_descriptor(Sampler &sampler)
{
	return sampler_descriptor_heap.place_descriptor(sampler_descriptor_indices.pop(), sampler);
}

void Descriptor_Heap_Pool::allocate_pool(Gpu_Device &device, u32 descriptors_count)
{
	cbsrua_descriptor_heap.create(device, descriptors_count);
	rt_descriptor_heap.create(device, descriptors_count);
	ds_descriptor_heap.create(device, descriptors_count);
	sampler_descriptor_heap.create(device, descriptors_count);

	rt_descriptor_indices.resize(descriptors_count);
	ds_descriptor_indices.resize(descriptors_count);
	cbsrua_descriptor_indices.resize(descriptors_count);
	sampler_descriptor_indices.resize(descriptors_count);

	u32 index = descriptors_count;
	for (u32 i = 0; i < descriptors_count; i++) {
		index -= 1;
		rt_descriptor_indices.push(index);
		ds_descriptor_indices.push(index);
		cbsrua_descriptor_indices.push(index);
		sampler_descriptor_indices.push(index);
	}
}

void Descriptor_Heap_Pool::free(CB_Descriptor *descriptor)
{
	if (descriptor->valid()) {
		cbsrua_descriptor_indices.push(descriptor->index);
	}
}

void Descriptor_Heap_Pool::free(SR_Descriptor *descriptor)
{
	if (descriptor->valid()) {
		cbsrua_descriptor_indices.push(descriptor->index);
	}
}

void Descriptor_Heap_Pool::free(UA_Descriptor *descriptor)
{
	if (descriptor->valid()) {
		cbsrua_descriptor_indices.push(descriptor->index);
	}
}

void Descriptor_Heap_Pool::free(Sampler_Descriptor *descriptor)
{
	if (descriptor->valid()) {
		sampler_descriptor_indices.push(descriptor->index);
	}
}

void Descriptor_Heap_Pool::free(RT_Descriptor *descriptor)
{
	if (descriptor->valid()) {
		rt_descriptor_indices.push(descriptor->index);
	}
}

void Descriptor_Heap_Pool::free(DS_Descriptor *descriptor)
{
	if (descriptor->valid()) {
		ds_descriptor_indices.push(descriptor->index);
	}
}

void Render_System::init(Win32_Window *win32_window, Variable_Service *variable_service)
{
	assert(win32_window->width > 0);
	assert(win32_window->height > 0);

	init_vars(win32_window, variable_service);

	window_view_plane.update(60, window.width, window.height, 1.0f, 10000.0f);

	if (!create_d3d12_gpu_device(gpu_device)) {
	}

	frame_fence.create(gpu_device);
	frame_fence_values.reserve(back_buffer_count);
	back_buffer_textures.reserve(back_buffer_count);
	zero_memory(&frame_fence_values);

	copy_queue.create(gpu_device, COMMAND_LIST_TYPE_COPY);
	copy_queue.set_name(L"Render sys copy queue");

	compute_queue.create(gpu_device, COMMAND_LIST_TYPE_COMPUTE);
	compute_queue.set_name(L"Render sys compute queue");

	graphics_queue.create(gpu_device, COMMAND_LIST_TYPE_DIRECT);
	graphics_queue.set_name(L"Render sys graphics queue");

	command_buffer.create(gpu_device, back_buffer_count);
	
	//upload_fence.create(gpu_device);
	//upload_command_list.create(gpu_device, 1);

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
		back_buffer_textures[i].rt_descriptor = descriptors_pool.allocate_rt_descriptor(&back_buffer_textures[i]);
	}

	Texture2D_Desc depth_texture_desc;
	depth_texture_desc.width = window.width;
	depth_texture_desc.height = window.height;
	depth_texture_desc.miplevels = 1;
	depth_texture_desc.format = DXGI_FORMAT_D32_FLOAT;
	depth_texture_desc.flags = DEPTH_STENCIL_RESOURCE;
	depth_texture_desc.clear_value = Clear_Value(1.0f, 0);

	back_buffer_depth_texture.create(gpu_device, GPU_HEAP_TYPE_DEFAULT, RESOURCE_STATE_DEPTH_WRITE, depth_texture_desc);

	back_buffer_depth_texture.ds_descriptor = descriptors_pool.allocate_ds_descriptor(&back_buffer_depth_texture);

	init_passes();
	pipeline_resource_storage.init(gpu_device, back_buffer_count, &descriptors_pool);

	init_buffers();

	command_buffer.back_buffer_depth_texture = &back_buffer_depth_texture;
	command_buffer.descriptors_pool = &descriptors_pool;

	init_texture();
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
	frame_graphics_passes.push(&passes.box);
	Shader_Manager *shader_manager = &Engine::get_instance()->shader_manager;
	for (u32 i = 0; i < frame_graphics_passes.count; i++) {
		frame_graphics_passes[i]->init(gpu_device, shader_manager, &pipeline_resource_storage);
	}

	generate_mipmaps.init(gpu_device, shader_manager, &pipeline_resource_storage);
}

void Render_System::init_buffers()
{
	//vertex_buffer.create(gpu_device, GPU_HEAP_TYPE_DEFAULT, RESOURCE_STATE_COMMON, Buffer_Desc(8, sizeof(VertexP3C3)));
	//index_buffer.create(gpu_device, GPU_HEAP_TYPE_DEFAULT, RESOURCE_STATE_COMMON, Buffer_Desc(36, sizeof(u32)));

	//Buffer intermediate_vertex_buffer;
	//Buffer intermediate_index_buffer;
	//intermediate_vertex_buffer.create(gpu_device, GPU_HEAP_TYPE_UPLOAD, RESOURCE_STATE_GENERIC_READ, Buffer_Desc(8, sizeof(VertexP3C3)));
	//intermediate_index_buffer.create(gpu_device, GPU_HEAP_TYPE_UPLOAD, RESOURCE_STATE_GENERIC_READ, Buffer_Desc(36, sizeof(u32)));

	//void *temp_vertex_buffer = (void *)intermediate_vertex_buffer.map();
	//memcpy(temp_vertex_buffer, (void *)vertices, sizeof(VertexP3C3) * 8);
	//intermediate_vertex_buffer.unmap();
	//
	//void *temp_index_buffer = (void *)intermediate_index_buffer.map();
	//memcpy(temp_index_buffer, (void *)indices, sizeof(u32) * 36);
	//intermediate_index_buffer.unmap();

	//Fence fence;
	//fence.create(gpu_device);
	//u64 fence_value = 0;

	//Copy_Command_List copy_command_list;
	//copy_command_list.create(gpu_device, 1);
	//copy_command_list.reset(0);

	//copy_command_list.copy_resources(vertex_buffer, intermediate_vertex_buffer);
	//copy_command_list.copy_resources(index_buffer, intermediate_index_buffer);

	//copy_command_list.close();
	//copy_queue.execute_command_list(copy_command_list);
	//auto new_fence_value = copy_queue.signal(fence_value, fence);
	//fence.wait_for_gpu(new_fence_value);
}

void Render_System::init_texture()
{
	//String path;
	//const char *texture_file = "entity.png";
	////const char *texture_file = "entity501x501.png";
	//build_full_path_to_texture_file(texture_file, path);
	//u32 width;
	//u32 height;
	//s32 image_channels;
	//u8 *buffer = stbi_load(path, (s32 *)&width, (s32 *)&height, &image_channels, 4);
	//assert(buffer);

	//Texture2D_Desc texture_desc;
	//texture_desc.width = width;
	//texture_desc.height= height;
	//texture_desc.miplevels = find_max_mip_map_level(width, height);
	//texture_desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//texture_desc.flags = ALLOW_UNORDERED_ACCESS;

	//texture.create(gpu_device, GPU_HEAP_TYPE_DEFAULT, RESOURCE_STATE_COPY_DEST, texture_desc);

	//texture.sr_descriptor = descriptors_pool.allocate_sr_descriptor(&texture);

	//Copy_Command_List copy_command_list;
	//copy_command_list.create(gpu_device, 1);
	//copy_command_list.reset(0);

	//Subresource_Info subresource_info;
	//subresource_info.width = width;
	//subresource_info.height = height;
	//subresource_info.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//subresource_info.row_pitch = align_address<u32>(width * dxgi_format_size(DXGI_FORMAT_R8G8B8A8_UNORM), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);;

	//Buffer temp_buffer;
	//upload_bitmap_in_buffer(width, height, buffer, DXGI_FORMAT_R8G8B8A8_UNORM, gpu_device, temp_buffer);

	//copy_command_list.copy_texture(texture, temp_buffer, subresource_info);

	//Fence fence;
	//fence.create(gpu_device);
	//u64 fence_value = 0;

	//copy_command_list.close();
	//copy_queue.execute_command_list(copy_command_list);
	//auto new_fence_value = copy_queue.signal(fence_value, fence);
	//fence.wait_for_gpu(new_fence_value);

	//Array<Texture *> textures;
	//textures.push(&texture);

	//Compute_Command_List compute_command_list;
	//compute_command_list.create(gpu_device, 1);
	//compute_command_list.reset(0);
	//
	//generate_mipmaps.generate(&compute_command_list, textures, this);

	//compute_command_list.close();
	//compute_queue.execute_command_list(compute_command_list);
	//new_fence_value = compute_queue.signal(fence_value, fence);
	//fence.wait_for_gpu(new_fence_value);
}

void Render_System::resize(u32 window_width, u32 window_height)
{
	if (Engine::initialized()) {
	}
}

void flush(u64 fence_value, Fence *fence, Command_Queue *command_queue)
{
	u64 new_value = command_queue->signal(fence_value, *fence);
	fence->wait_for_gpu(new_value);
}

void Render_System::flush()
{
	graphics_queue.signal(frame_fence_value, frame_fence);
	frame_fence.wait_for_gpu(frame_fence_value);
}

void Render_System::render()
{
	Render_World *render_world = Engine::get_render_world();
	pipeline_resource_storage.begin_frame(back_buffer_index);

	GPU_Frame_Info frame_info;
	frame_info.view_matrix = render_world->rendering_view.view_matrix;
	frame_info.perspective_matrix = window_view_plane.perspective_matrix;
	frame_info.orthographic_matrix = window_view_plane.orthographic_matrix;
	frame_info.near_plane = window_view_plane.near_plane;
	frame_info.far_plane = window_view_plane.far_plane;

	frame_info.view_position = render_world->rendering_view.position;
	frame_info.view_direction = render_world->rendering_view.direction;
	frame_info.light_count = render_world->shader_lights.count;

	pipeline_resource_storage.update_frame_info_constant_buffer(&frame_info);

	auto *command_list = command_buffer.get_graphics_command_list();

	command_list->reset(back_buffer_index);

	Texture &back_buffer = back_buffer_textures[back_buffer_index];

	command_list->resource_barrier(Transition_Resource_Barrier(back_buffer, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET));
	command_list->clear_render_target_view(back_buffer.rt_descriptor, Color::LightSteelBlue);
	command_list->clear_depth_stencil_view(back_buffer_depth_texture.ds_descriptor);

	command_buffer.begin_frame(&back_buffer);
	
	for (u32 i = 0; i < frame_graphics_passes.count; i++) {
		frame_graphics_passes[i]->render(&command_buffer, render_world, (void *)this);
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

	Sampler point_sampler = Sampler(SAMPLER_FILTER_POINT, ADDRESS_MODE_WRAP);
	Sampler linear_sampler = Sampler(SAMPLER_FILTER_LINEAR, ADDRESS_MODE_WRAP);
	Sampler anisotropic_sampler = Sampler(SAMPLER_FILTER_ANISOTROPIC, ADDRESS_MODE_WRAP);
	
	point_sampler_descriptor = descriptor_heap_pool->allocate_sampler_descriptor(point_sampler);
	linear_sampler_descriptor = descriptor_heap_pool->allocate_sampler_descriptor(linear_sampler);
	anisotropic_sampler_descriptor = descriptor_heap_pool->allocate_sampler_descriptor(anisotropic_sampler);

	buffers_allocator.create(device, megabytes_to_bytes<u64>(1), GPU_HEAP_TYPE_UPLOAD, GPU_HEAP_CONTAIN_BUFFERS);

	frame_info_buffer.create(device, &buffers_allocator, back_buffer_count, Buffer_Desc(align_address<u32>(sizeof(GPU_Frame_Info), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)));
	for (u32 j = 0; j < back_buffer_count; j++) {
		frame_info_buffer.buffers[j]->cb_descriptor = descriptor_heap_pool->allocate_cb_descriptor(frame_info_buffer.buffers[j]);
	}

	for (u32 i = 0; i < cpu_buffers.count; i++) {
		u32 buffer_size = align_address<u32>(cpu_buffers_sizes[i], D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		CPU_Buffer *cpu_buffer = cpu_buffers[i];
		cpu_buffer->create(device, &buffers_allocator, back_buffer_count, Buffer_Desc(buffer_size));
		for (u32 j = 0; j < back_buffer_count; j++) {
			cpu_buffer->buffers[j]->cb_descriptor = descriptor_heap_pool->allocate_cb_descriptor(cpu_buffer->buffers[j]);
		}
	}

	cpu_buffers_sizes.clear();
}

void Pipeline_Resource_Storage::begin_frame(u32 _back_buffer_index)
{
	back_buffer_index = _back_buffer_index;
	for (u32 i = 0; i < cpu_buffers.count; i++) {
		cpu_buffers[i]->begin_frame(back_buffer_index);
	}
	frame_info_buffer.begin_frame(back_buffer_index);
}

CPU_Buffer *Pipeline_Resource_Storage::request_constant_buffer(u32 buffer_size)
{
	cpu_buffers_sizes.push(buffer_size);
	cpu_buffers.push(new CPU_Buffer());
	return cpu_buffers.last();
}

void Pipeline_Resource_Storage::update_frame_info_constant_buffer(GPU_Frame_Info *frame_info)
{
	frame_info_buffer.write(*frame_info);
}

void CPU_Buffer::begin_frame(u32 _frame_index)
{
	frame_index = _frame_index;
}

void CPU_Buffer::create(Gpu_Device &device, GPU_Allocator *allocator, u32 frames_in_flight, const Buffer_Desc &buffers_desc)
{
	buffers.reserve(frames_in_flight);
	for (u32 i = 0; i < frames_in_flight; i++) {
		u32 buffer_size = const_cast<Buffer_Desc &>(buffers_desc).get_size();
		buffers[i] = new Buffer();
		buffers[i]->create(device, allocator->heap, allocator->allocate(buffer_size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT), RESOURCE_STATE_GENERIC_READ, buffers_desc);
	}
}

Buffer *CPU_Buffer::get_frame_resource()
{
	return buffers[frame_index];
}

CB_Descriptor CPU_Buffer::get_cb_descriptor()
{
	Buffer *buffer = get_frame_resource();
	return buffer->cb_descriptor;
}

void Render_Command_Buffer::create(Gpu_Device &device, u32 frames_in_flight)
{
	graphics_command_list.create(device, frames_in_flight);
	compute_command_list.create(device, frames_in_flight);

	descriptors_pool = &Engine::get_render_system()->descriptors_pool;
	pipeline_resource_storage = &Engine::get_render_system()->pipeline_resource_storage;
}

void Render_Command_Buffer::setup_common_compute_pipeline_resources(Root_Signature *root_signature)
{
	GPU_Descriptor base_cbsrua_descriptor = descriptors_pool->cbsrua_descriptor_heap.get_base_gpu_descriptor();
	CB_Descriptor frame_info_buffer_descriptor = pipeline_resource_storage->frame_info_buffer.get_cb_descriptor();

	Compute_Command_List_Helper binding_helper = Compute_Command_List_Helper(&graphics_command_list, root_signature);
	binding_helper.set_root_descriptor_table(0, 10, &frame_info_buffer_descriptor);                         // Per frame info buffer
	binding_helper.set_root_descriptor_table(0, 10, static_cast<SR_Descriptor *>(&base_cbsrua_descriptor)); // Textures
	binding_helper.set_root_descriptor_table(0, 10, &pipeline_resource_storage->point_sampler_descriptor);  // Point sampler
	binding_helper.set_root_descriptor_table(1, 10, &pipeline_resource_storage->linear_sampler_descriptor); // Linear sampler
}

void Render_Command_Buffer::setup_common_graphics_pipeline_resources(Root_Signature *root_signature)
{
	GPU_Descriptor base_cbsrua_descriptor = descriptors_pool->cbsrua_descriptor_heap.get_base_gpu_descriptor();
	CB_Descriptor frame_info_buffer_descriptor = pipeline_resource_storage->frame_info_buffer.get_cb_descriptor();

	Graphics_Command_List_Helper binding_helper = Graphics_Command_List_Helper(&graphics_command_list, root_signature);
	binding_helper.set_root_descriptor_table(0, 10, &frame_info_buffer_descriptor);                         // Per frame info buffer
	binding_helper.set_root_descriptor_table(0, 10, static_cast<SR_Descriptor *>(&base_cbsrua_descriptor)); // Textures
	binding_helper.set_root_descriptor_table(0, 10, &pipeline_resource_storage->point_sampler_descriptor);  // Point sampler
	binding_helper.set_root_descriptor_table(1, 10, &pipeline_resource_storage->linear_sampler_descriptor); // Linear sampler
}

void Render_Command_Buffer::apply(Compute_Pipeline_State *pipeline_state, u32 flags)
{
	compute_command_list.set_compute_root_signature(*pipeline_state->root_signature);
	compute_command_list.set_pipeline_state(*pipeline_state);
	compute_command_list.set_descriptor_heaps(descriptors_pool->cbsrua_descriptor_heap, descriptors_pool->sampler_descriptor_heap);
	
	setup_common_compute_pipeline_resources(pipeline_state->root_signature);
}

void Render_Command_Buffer::apply(Graphics_Pipeline_State *pipeline_state, u32 flags)
{
	graphics_command_list.set_graphics_root_signature(*pipeline_state->root_signature);
	graphics_command_list.set_pipeline_state(*pipeline_state);
	graphics_command_list.set_descriptor_heaps(descriptors_pool->cbsrua_descriptor_heap, descriptors_pool->sampler_descriptor_heap);
	
	setup_common_graphics_pipeline_resources(pipeline_state->root_signature);

	graphics_command_list.set_primitive_type(pipeline_state->primitive_type);
	graphics_command_list.set_viewport(pipeline_state->viewport);
	graphics_command_list.set_clip_rect(pipeline_state->clip_rect);

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

void View_Plane::update(u32 fov_in_degrees, u32 width, u32 height, float _near_plane, float _far_plane)
{
	ratio = (float)width / (float)height;
	fov = degrees_to_radians((float)fov_in_degrees);
	near_plane = _near_plane;
	far_plane = _far_plane;
	perspective_matrix = XMMatrixPerspectiveFovLH(fov, ratio, near_plane, far_plane);
	orthographic_matrix = XMMatrixOrthographicOffCenterLH(0.0f, (float)width, (float)height, 0.0f, near_plane, far_plane);
}
