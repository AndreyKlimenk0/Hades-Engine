#include <assert.h>

#include "font.h"
#include "render_system.h"
#include "render_helpers.h"

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

SR_Descriptor Descriptor_Heap_Pool::allocate_sr_descriptor(GPU_Resource *resource, u32 mipmap_level)
{
	return cbsrua_descriptor_heap.place_sr_descriptor(cbsrua_descriptor_indices.pop(), *resource, mipmap_level);
}

UA_Descriptor Descriptor_Heap_Pool::allocate_ua_descriptor(GPU_Resource *resource, u32 mipmap_level)
{
	return cbsrua_descriptor_heap.place_ua_descriptor(cbsrua_descriptor_indices.pop(), *resource, mipmap_level);
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
	copy_queue.set_debug_name("Copy Queue");

	compute_queue.create(gpu_device, COMMAND_LIST_TYPE_COMPUTE);
	compute_queue.set_debug_name("Compute Queue");

	graphics_queue.create(gpu_device, COMMAND_LIST_TYPE_DIRECT);
	graphics_queue.set_debug_name("Graphics Queue");

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
		back_buffer_textures[i].descriptor_pool = &descriptors_pool;
		swap_chain.get_buffer(i, back_buffer_textures[i]);
	}
	
	pipeline_resource_storage.init(gpu_device, back_buffer_count, &descriptors_pool);

	Texture_Desc depth_texture_desc;
	depth_texture_desc.dimension = TEXTURE_DIMENSION_2D;
	depth_texture_desc.width = window.width;
	depth_texture_desc.height = window.height;
	depth_texture_desc.format = DXGI_FORMAT_D32_FLOAT;
	depth_texture_desc.flags = DEPTH_STENCIL_RESOURCE;
	depth_texture_desc.clear_value = Clear_Value(1.0f, 0);
	depth_texture_desc.resource_state = RESOURCE_STATE_DEPTH_WRITE;

	back_buffer_depth_texture = pipeline_resource_storage.create_texture(&depth_texture_desc);

	init_passes();

	command_buffer.back_buffer_depth_texture = back_buffer_depth_texture;
	command_buffer.descriptors_pool = &descriptors_pool;

	copy_manager.init(this);
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
	//frame_graphics_passes.push(&passes.box);
	//Shader_Manager *shader_manager = &Engine::get_instance()->shader_manager;
	//for (u32 i = 0; i < frame_graphics_passes.count; i++) {
	//	frame_graphics_passes[i]->init(gpu_device, shader_manager, &pipeline_resource_storage);
	//}

	Shader_Manager *sm = &Engine::get_instance()->shader_manager;
	generate_mipmaps.init(gpu_device, sm, &pipeline_resource_storage);

	auto pass = new Box_Pass();
	pass->init("Box_Pass", gpu_device, sm, &pipeline_resource_storage);
	
	passes.push(pass);
}

void Render_System::resize(u32 window_width, u32 window_height)
{
	if (Engine::initialized()) {
	}
}

void flush(Fence *fence, Command_Queue *command_queue)
{
	command_queue->signal(*fence);
	fence->wait_for_gpu();
}

void Render_System::flush()
{
	Fence fence;
	fence.create(gpu_device, 1);
	graphics_queue.signal(fence);
	fence.wait_for_gpu();
}

void Render_System::begin_frame()
{
	frame_fence.increment_expected_value();
	pipeline_resource_storage.begin_frame(frame_fence.expected_value);
}

void Render_System::end_frame(u64 frame_number)
{
	pipeline_resource_storage.end_frame(frame_number);
}

void wait_for_gpu(Fence *fence, u64 expected_value)
{
	u64 completed_value = fence->get()->GetCompletedValue();
	if (completed_value < expected_value) {
		fence->get()->SetEventOnCompletion(expected_value, fence->handle);
		WaitForSingleObject(fence->handle, INFINITE);
	}
}

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
	frame_info.light_count = render_world->shader_lights.count;

	render_world->render();

	copy_manager.execute_copying();

	pipeline_resource_storage.update_frame_info_constant_buffer(&frame_info);

	auto *command_list = command_buffer.get_graphics_command_list();

	//print("Begin render Frame", frame_fence.expected_value);
	//print("Reset command allocator", back_buffer_index);
	command_list->reset(back_buffer_index);

	Texture &back_buffer = back_buffer_textures[back_buffer_index];

	command_list->resource_barrier(Transition_Resource_Barrier(back_buffer, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET));
	command_list->clear_render_target_view(back_buffer.get_render_target_descriptor(), Color::LightSteelBlue);
	command_list->clear_depth_stencil_view(back_buffer_depth_texture->get_depth_stencil_descriptor());

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

	//frame_fence.expected_value = frame_fence_values[back_buffer_index];
	//print("Get new expected value", frame_fence.expected_value);
	//frame_fence.wait_for_gpu();
	wait_for_gpu(&frame_fence, frame_fence.expected_value - 1);
	
	end_frame(frame_fence.expected_value - 1);

	//print("End render Frame", frame_fence_values[back_buffer_index]);
	//end_frame(frame_fence_values[back_buffer_index]);
	//print("----------------------------------------------------------");
	
	//graphics_queue.signal(frame_fence);
	//print("Save expected value", frame_fence.expected_value);
	//frame_fence_values[back_buffer_index] = frame_fence.expected_value;

	//back_buffer_index = swap_chain.get_current_back_buffer_index();

	//frame_fence.expected_value = frame_fence_values[back_buffer_index];
	//print("Get new expected value", frame_fence.expected_value);
	//frame_fence.wait_for_gpu();

	//print("End render Frame", frame_fence_values[back_buffer_index]);
	//end_frame(frame_fence_values[back_buffer_index]);
	//print("----------------------------------------------------------");
}

Size_u32 Render_System::get_window_size()
{
	return { window.width, window.height };
}

void Pipeline_Resource_Storage::init(Gpu_Device &device, u32 _back_buffer_count, Descriptor_Heap_Pool *descriptor_heap_pool)
{
	assert(_back_buffer_count > 0);

	Sampler point_sampler = Sampler(SAMPLER_FILTER_POINT, ADDRESS_MODE_WRAP);
	Sampler linear_sampler = Sampler(SAMPLER_FILTER_LINEAR, ADDRESS_MODE_WRAP);
	Sampler anisotropic_sampler = Sampler(SAMPLER_FILTER_ANISOTROPIC, ADDRESS_MODE_WRAP);

	resource_allocator.init(Engine::get_render_system());

	point_sampler_descriptor = descriptor_heap_pool->allocate_sampler_descriptor(point_sampler);
	linear_sampler_descriptor = descriptor_heap_pool->allocate_sampler_descriptor(linear_sampler);
	anisotropic_sampler_descriptor = descriptor_heap_pool->allocate_sampler_descriptor(anisotropic_sampler);

	Buffer_Desc buffer_desc = Buffer_Desc(sizeof(GPU_Frame_Info), CONSTANT_BUFFER_ALIGNMENT);
	//Buffer_Desc buffer_desc = Buffer_Desc(2560);
	frame_info_buffer = create_buffer(BUFFER_TYPE_UPLOAD, &buffer_desc);
}

void Pipeline_Resource_Storage::update_frame_info_constant_buffer(GPU_Frame_Info *frame_info)
{
	frame_info_buffer->write(frame_info, sizeof(GPU_Frame_Info), 256);
}

void Pipeline_Resource_Storage::begin_frame(u64 frame_number)
{
	frame_count = frame_number;
	for (u32 i = 0; i < buffers.count; i++) {
		buffers[i]->begin_frame(frame_number);
	}
}

void Pipeline_Resource_Storage::end_frame(u64 frame_number)
{
	for (u32 i = 0; i < buffers.count; i++) {
		buffers[i]->end_frame(frame_number);
	}
}

Buffer *Pipeline_Resource_Storage::create_buffer(Buffer_Type buffer_type, Buffer_Desc *buffer_desc)
{
	Render_System *render_sys = Engine::get_render_system();
	auto buffer = new Buffer();
	buffer->create(buffer_type, buffer_desc, &resource_allocator, &render_sys->descriptors_pool, &render_sys->copy_manager);
	buffer->begin_frame(frame_count);
	buffers.push(buffer);
	return buffer;
}

Texture *Pipeline_Resource_Storage::create_texture(Texture_Desc *texture_desc)
{
	Render_System *render_sys = Engine::get_render_system();
	auto texture = new Texture();
	texture->create(texture_desc, &resource_allocator, &render_sys->descriptors_pool);
	return texture;
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
	CB_Descriptor frame_info_buffer_descriptor = pipeline_resource_storage->frame_info_buffer->get_constant_buffer_descriptor();

	Compute_Command_List_Helper binding_helper = Compute_Command_List_Helper(&graphics_command_list, root_signature);
	binding_helper.set_root_descriptor_table(0, 10, &frame_info_buffer_descriptor);                         // Per frame info buffer
	binding_helper.set_root_descriptor_table(0, 10, static_cast<SR_Descriptor *>(&base_cbsrua_descriptor)); // Textures
	binding_helper.set_root_descriptor_table(0, 10, &pipeline_resource_storage->point_sampler_descriptor);  // Point sampler
	binding_helper.set_root_descriptor_table(1, 10, &pipeline_resource_storage->linear_sampler_descriptor); // Linear sampler
}

void Render_Command_Buffer::setup_common_graphics_pipeline_resources(Root_Signature *root_signature)
{
	GPU_Descriptor base_cbsrua_descriptor = descriptors_pool->cbsrua_descriptor_heap.get_base_gpu_descriptor();
	CB_Descriptor frame_info_buffer_descriptor = pipeline_resource_storage->frame_info_buffer->get_constant_buffer_descriptor();

	Graphics_Command_List_Helper binding_helper = Graphics_Command_List_Helper(&graphics_command_list, root_signature);
	binding_helper.set_root_descriptor_table(0, 10, &frame_info_buffer_descriptor);                         // Per frame info buffer
	binding_helper.set_root_descriptor_table(0, 10, static_cast<SR_Descriptor *>(&base_cbsrua_descriptor)); // Textures
	binding_helper.set_root_descriptor_table(0, 10, &pipeline_resource_storage->point_sampler_descriptor);  // Point sampler
	binding_helper.set_root_descriptor_table(1, 10, &pipeline_resource_storage->linear_sampler_descriptor); // Linear sampler
}

void Render_Command_Buffer::apply(Pipeline_State *pipeline_state, u32 flags)
{
	assert(pipeline_state->type != PIPELINE_TYPE_UNKNOWN);

	current_pipeline = pipeline_state;
	if (pipeline_state->type == PIPELINE_TYPE_COMPUTE) {
		apply_compute_pipeline(pipeline_state, flags);
	} else if (pipeline_state->type == PIPELINE_TYPE_GRAPHICS) {
		apply_graphics_pipeline(pipeline_state, flags);
	}
}

void Render_Command_Buffer::apply_compute_pipeline(Pipeline_State *pipeline_state, u32 flags)
{
	compute_command_list.set_compute_root_signature(*pipeline_state->root_signature);
	compute_command_list.set_pipeline_state(*pipeline_state);
	compute_command_list.set_descriptor_heaps(descriptors_pool->cbsrua_descriptor_heap, descriptors_pool->sampler_descriptor_heap);

	setup_common_compute_pipeline_resources(pipeline_state->root_signature);
}

void Render_Command_Buffer::apply_graphics_pipeline(Pipeline_State *pipeline_state, u32 flags)
{
	graphics_command_list.set_graphics_root_signature(*pipeline_state->root_signature);
	graphics_command_list.set_pipeline_state(*pipeline_state);
	graphics_command_list.set_descriptor_heaps(descriptors_pool->cbsrua_descriptor_heap, descriptors_pool->sampler_descriptor_heap);

	setup_common_graphics_pipeline_resources(pipeline_state->root_signature);

	graphics_command_list.set_primitive_type(pipeline_state->primitive_type);
	graphics_command_list.set_viewport(pipeline_state->viewport);
	graphics_command_list.set_clip_rect(pipeline_state->clip_rect);

	if (flags & RENDER_TARGET_BUFFER_BUFFER) {
		RT_Descriptor render_target_descriptor = back_buffer_texture->get_render_target_descriptor();
		DS_Descriptor ds_descriptor = back_buffer_depth_texture->get_depth_stencil_descriptor();
		graphics_command_list.get()->OMSetRenderTargets(1, &render_target_descriptor.cpu_handle, FALSE, &ds_descriptor.cpu_handle);
	} else {
	}
}

void Render_Command_Buffer::bind_buffer(u32 shader_register, u32 shader_space, Shader_Register type, Buffer *buffer)
{
	Root_Signature *root_signature = current_pipeline->root_signature;

	if (current_pipeline->type == PIPELINE_TYPE_COMPUTE) {

	
	} else if (current_pipeline->type == PIPELINE_TYPE_GRAPHICS) {
		switch (type) {
			case SHADER_RESOURCE_REGISTER: {
				SR_Descriptor descriptor = buffer->get_shader_resource_descriptor();
				graphics_command_list.set_graphics_root_descriptor_table(root_signature->get_parameter_index(shader_register, shader_space, ROOT_PARAMETER_SHADER_RESOURCE), descriptor);
				break;
			}
			case CONSTANT_BUFFER_REGISTER: {
				CB_Descriptor descriptor = buffer->get_constant_buffer_descriptor();
				graphics_command_list.set_graphics_root_descriptor_table(root_signature->get_parameter_index(shader_register, shader_space, ROOT_PARAMETER_CONSTANT_BUFFER), descriptor);
				break;
			}
		}
	}
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