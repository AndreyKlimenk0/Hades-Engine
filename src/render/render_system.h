#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <stdlib.h>

#include "../libs/number_types.h"
#include "../libs/structures/array.h"
#include "../libs/math/structures.h"

#include "gpu_data.h"
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

struct View_Plane {
	u32 width;
	u32 height;
	float ratio;
	float fov;
	float near_plane;
	float far_plane;

	Matrix4 perspective_matrix;
	Matrix4 orthographic_matrix;

	void update(u32 fov_in_degrees, u32 width, u32 height, float _near_plane, float _far_plane);
};

struct Box_Render_Pass {

};

struct Command_List_Allocator {

};

struct Descriptor_Heap_Pool {
	Descriptor_Heap_Pool();
	~Descriptor_Heap_Pool();

	Array<u32> rt_descriptor_indices;
	Array<u32> ds_descriptor_indices;
	Array<u32> cbsrua_descriptor_indices;
	Array<u32> sampler_descriptor_indices;

	RT_Descriptor_Heap rt_descriptor_heap;
	DS_Descriptor_Heap ds_descriptor_heap;
	CBSRUA_Descriptor_Heap cbsrua_descriptor_heap;
	Sampler_Descriptor_Heap sampler_descriptor_heap;

	CB_Descriptor allocate_cb_descriptor(GPU_Resource *resource);
	SR_Descriptor allocate_sr_descriptor(GPU_Resource *resource);
	UA_Descriptor allocate_ua_descriptor(GPU_Resource *resource, u32 mip_level = 0);
	RT_Descriptor allocate_rt_descriptor(GPU_Resource *resource);
	DS_Descriptor allocate_ds_descriptor(GPU_Resource *resource);
	Sampler_Descriptor allocate_sampler_descriptor(Sampler &sampler);

	void allocate_pool(Gpu_Device &device, u32 descriptors_count);

	void free(CB_Descriptor *descriptor);
	void free(SR_Descriptor *descriptor);
	void free(UA_Descriptor *descriptor);
	void free(RT_Descriptor *descriptor);
	void free(DS_Descriptor *descriptor);
	void free(Sampler_Descriptor *descriptor);
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

	template <typename T>
	void write(const T &data);

	void begin_frame(u32 _frame_index);
	void create(Gpu_Device &device, GPU_Allocator *allocator, u32 frames_in_flight, const Buffer_Desc &buffers_desc);
	Buffer *get_frame_resource();
	CB_Descriptor get_cb_descriptor();
};

template<typename T>
inline void CPU_Buffer::write(const T &data)
{
	Buffer *buffer = get_frame_resource();
	buffer->write(data, 256);
}

struct Pipeline_Resource_Storage {
	u32 back_buffer_count = 0;
	u32 back_buffer_index = 0;

	Sampler_Descriptor point_sampler_descriptor;
	Sampler_Descriptor linear_sampler_descriptor;
	Sampler_Descriptor anisotropic_sampler_descriptor;

	Descriptor_Heap_Pool *descriptor_heap_pool = NULL;

	CPU_Buffer frame_info_buffer;
	Array<u32> cpu_buffers_sizes;
	Array<CPU_Buffer *> cpu_buffers;

	GPU_Linear_Allocator buffers_allocator;

	void init(Gpu_Device &device, u32 _back_buffer_count, Descriptor_Heap_Pool *_descriptor_heap_pool);
	void begin_frame(u32 _back_buffer_index);

	CPU_Buffer *request_constant_buffer(u32 buffer_size);
	
	void update_frame_info_constant_buffer(GPU_Frame_Info *frame_info);
};

const u32 RENDER_TARGET_BUFFER_BUFFER = 0x1;

struct Render_Command_Buffer {
	Copy_Command_List copy_command_list;
	Compute_Command_List compute_command_list;
	Graphics_Command_List graphics_command_list;
	
	Texture *back_buffer_texture = NULL;
	Texture *back_buffer_depth_texture = NULL;
	Descriptor_Heap_Pool *descriptors_pool = NULL;
	Pipeline_Resource_Storage *pipeline_resource_storage = NULL;

	void create(Gpu_Device &device, u32 frames_in_flight);
	void setup_common_compute_pipeline_resources(Root_Signature *root_signature);
	void setup_common_graphics_pipeline_resources(Root_Signature *root_signature);

	void apply(Compute_Pipeline_State *pipeline, u32 flags = 0);
	void apply(Graphics_Pipeline_State *pipeline, u32 flags = 0);
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

	View_Plane window_view_plane;

	Fence frame_fence;
	Array<u64> frame_fence_values;

	Gpu_Device gpu_device;
	Swap_Chain swap_chain;

	Command_Queue copy_queue;
	Command_Queue compute_queue;
	Command_Queue graphics_queue;
	
	//bool execute_uploading = false;
	//u64 upload_fence_value = 0;
	//Fence upload_fence;
	//Compute_Command_List upload_command_list;

	Render_Command_Buffer command_buffer;

	Texture back_buffer_depth_texture;
	Array<Texture> back_buffer_textures;

	Descriptor_Heap_Pool descriptors_pool;
	Pipeline_Resource_Storage pipeline_resource_storage;

	struct Render_Passes {
		Box_Pass box;
	} passes;

	Generate_Mipmaps generate_mipmaps;
	Array<Graphics_Pass *> frame_graphics_passes;

	void init(Win32_Window *win32_window, Variable_Service *variable_service);
	void init_vars(Win32_Window *win32_window, Variable_Service *variable_service);
	void init_passes();
	void init_buffers();
	void init_texture();
	void resize(u32 window_width, u32 window_height);

	void flush();
	void render();

	Size_u32 get_window_size();
};
#endif
