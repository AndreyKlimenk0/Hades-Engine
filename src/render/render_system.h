#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <stdlib.h>

#include "../libs/number_types.h"
#include "../libs/structures/array.h"
#include "../libs/math/structures.h"

#include "render_passes.h"
#include "render_api/base.h"
#include "render_api/fence.h"
#include "render_api/buffer.h"
#include "render_api/sampler.h"
#include "render_api/texture.h"
#include "render_api/resource.h"
#include "render_api/descriptor_heap.h"
#include "render_api/command.h"
#include "render_api/swap_chain.h"

struct Win32_Window;
struct Variable_Service;

struct View {
	float ratio;
	float fov;
	float near_plane;
	float far_plane;

	Matrix4 perspective_matrix;
	Matrix4 orthogonal_matrix;

	void update_proje0ction_matries(u32 fov_in_degrees, u32 width, u32 height, float _near_plane, float _far_plane);
};

struct Projection_Plane {
	u32 width;
	u32 height;
	float ratio;
	float fov_y_ratio;
	float near_plane;
	float far_plane;
};

struct Projection_Matries {
	Matrix4 perspective_matrix;
	Matrix4 orthogonal_matrix;

	void update();
};

struct Box_Render_Pass {

};

struct Command_List_Allocator {

};

struct Descriptor_Heap_Pool {
	Descriptor_Heap_Pool();
	~Descriptor_Heap_Pool();

	u32 rt_descriptor_count = 0;
	u32 ds_descriptor_count = 0;
	u32 cbsrua_descriptor_count = 0;
	u32 samper_descriptor_count = 0;

	RT_Descriptor_Heap rt_descriptor_heap;
	DS_Descriptor_Heap ds_descriptor_heap;
	CBSRUA_Descriptor_Heap cbsrua_descriptor_heap;
	Sampler_Descriptor_Heap sampler_descriptor_heap;

	void allocate_cb_descriptor(Buffer &buffer);
	void allocate_sr_descriptor(Buffer &buffer);
	void allocate_ua_descriptor(Buffer &buffer);

	void allocate_rt_descriptor(Texture &texture);
	void allocate_ds_descriptor(Texture &texture);
	void allocate_sr_descriptor(Texture &texture);

	Sampler_Descriptor allocate_sampler_descriptor(Sampler &sampler);

	void allocate_pool(Gpu_Device &device, u32 descriptors_count);
};

struct GPU_Allocator {
	GPU_Allocator() = default;
	virtual ~GPU_Allocator() = default;
	
	GPU_Heap heap;
	
	virtual u64 allocate(u64 size, u64 alignment = 0) = 0;
};

struct GPU_Linear_Allocator : GPU_Allocator {
	u64 heap_size = 0;
	u64 heap_offset = 0;

	void create(Gpu_Device &device, u64 heap_size_in_bytes, GPU_Heap_Type heap_type, GPU_Heap_Content content);
	u64 allocate(u64 size, u64 alignment = 0);
};

struct CPU_Buffer {
	u32 frame_index;
	Array<Buffer *> buffers;

	void begin_frame(u32 _frame_index);
	void write(void *data, u32 data_size);
	void create(Gpu_Device &device, GPU_Allocator *allocator, u32 frames_in_flight, const Buffer_Desc &buffers_desc);
	Buffer *get_frame_resource();
};

struct Pipeline_Resource_Storage {
	u32 back_buffer_count = 0;
	u32 back_buffer_index = 0;

	Sampler_Descriptor point_sampler_descriptor;
	Sampler_Descriptor linear_sampler_descriptor;
	Sampler_Descriptor anisotropic_sampler_descriptor;

	Descriptor_Heap_Pool *descriptor_heap_pool = NULL;

	Array<u32> cpu_buffers_sizes;
	Array<CPU_Buffer *> cpu_buffers;

	GPU_Linear_Allocator buffers_allocator;

	void init(Gpu_Device &device, u32 _back_buffer_count, Descriptor_Heap_Pool *_descriptor_heap_pool);
	void begin_frame(u32 _back_buffer_index);

	CPU_Buffer *request_constant_buffer(u32 buffer_size);
};

const u32 RENDER_TARGET_BUFFER_BUFFER = 0x1;

struct Render_Command_Buffer {
	Copy_Command_List copy_command_list;
	Graphics_Command_List graphics_command_list;
	
	Texture *back_buffer_texture = NULL;
	Texture *back_buffer_depth_texture = NULL;
	Descriptor_Heap_Pool *descriptors_pool = NULL;

	void create(Gpu_Device &device, u32 frames_in_flight);
	void apply(Pipeline_State &pipeline_state, u32 flags = 0);
	void begin_frame(Texture *back_buffer);
	void draw(u32 index_count);

	Graphics_Command_List *get_graphics_command_list();
};

struct Render_System {
	struct Window {
		bool vsync = false;
		bool windowed = true;
		u32 width = 0;
		u32 height = 0;
	} window;

	u32 sync_interval = 0;
	u32 present_flags = 0;
	u32 back_buffer_count = 0;
	u32 back_buffer_index = 0;
	u64 frame_fence_value = 0;

	Fence frame_fence;
	Array<u64> frame_fence_values;

	Gpu_Device gpu_device;
	Swap_Chain swap_chain;

	Command_Queue copy_queue;
	Command_Queue compute_queue;
	Command_Queue graphics_queue;
	
	Render_Command_Buffer command_buffer;

	Texture texture;

	Texture back_buffer_depth_texture;
	Array<Texture> back_buffer_textures;

	Descriptor_Heap_Pool descriptors_pool;
	Pipeline_Resource_Storage pipeline_resource_storage;

	struct Render_Passes {
		Box_Pass box;
	} passes;

	//Test data
	Buffer vertex_buffer;
	Buffer index_buffer;

	Array<Render_Pass *> frame_passes;

	void init(Win32_Window *win32_window, Variable_Service *variable_service);
	void init_vars(Win32_Window *win32_window, Variable_Service *variable_service);
	void init_passes();
	void init_buffers();
	void init_texture();
	void resize(u32 window_width, u32 window_height);

	void render();

	Size_u32 get_window_size();
};
#endif
