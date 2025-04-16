#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <stdlib.h>

#include "../libs/number_types.h"
#include "../libs/structures/array.h"
#include "../libs/structures/queue.h"
#include "../libs/math/structures.h"

#include "gpu_data.h"
#include "render_resources.h"
#include "render_passes.h"
#include "render_api/base.h"
#include "render_api/fence.h"
#include "render_api/buffer.h"
#include "render_api/sampler.h"
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
	SR_Descriptor allocate_sr_descriptor(GPU_Resource *resource, u32 mipmap_level = 0);
	UA_Descriptor allocate_ua_descriptor(GPU_Resource *resource, u32 mipmap_level = 0);
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

struct Resource_Allocation {
	GPU_Heap *heap = NULL;
	u64 heap_offset = 0;
};

struct Resource_Allocator {
	u64 default_buffer_offset = 0;
	u64 upload_buffer_offset = 0;
	u64 texture_offset = 0;

	u64 default_heap_size = 0;
	u64 upload_heap_size = 0;
	u64 texture_heap_size = 0;
	
	GPU_Heap default_buffer_heap;
	GPU_Heap upload_buffer_heap;
	GPU_Heap texture_heap;

	void init(Render_System *render_sys);
	virtual Resource_Allocation allocate_buffer(Buffer_Type buffer_type, u64 allocation_size);
	virtual Resource_Allocation allocate_texture(u64 texture_size);
};

struct GPU_Buffer_Frame_Number_Pair {
	GPU_Buffer *buffer = NULL;
	u64 frame_number = ULLONG_MAX;
};

enum GPU_Resource_Type {
	BUFFER_RESOURCE,
	TEXTURE_RESOURCE
};

struct Copy_Command {
	Copy_Command() {}
	Copy_Command(GPU_Resource *dest_resource, GPU_Resource *source_resource) : resource_type(BUFFER_RESOURCE), dest_resource(dest_resource), source_resource(source_resource) {}
	Copy_Command(GPU_Resource *dest_resource, GPU_Resource *source_resource, Subresource_Footprint *subresource_footprint) : resource_type(TEXTURE_RESOURCE), dest_resource(dest_resource), source_resource(source_resource), subresource_footprint(*subresource_footprint) {}
	~Copy_Command() {}
	
	GPU_Resource_Type resource_type;
	GPU_Resource *dest_resource = NULL;
	GPU_Resource *source_resource = NULL;
	Subresource_Footprint subresource_footprint;
};

struct Copy_Manager {
	Copy_Manager();
	~Copy_Manager();
	
	Render_System *render_sys = NULL;
	Fence copy_fence;
	Array<Copy_Command> copy_commands;
	Copy_Command_List copy_command_list;
	
	void init(Render_System *render_system);
	void add_copy_command(Copy_Command *copy_command);
	void execute_copying();
};

struct Pipeline_Resource_Storage {
	Sampler_Descriptor point_sampler_descriptor;
	Sampler_Descriptor linear_sampler_descriptor;
	Sampler_Descriptor anisotropic_sampler_descriptor;

	u64 frame_count = 0;
	Buffer *frame_info_buffer = NULL;

	void init(Gpu_Device &device, u32 _back_buffer_count, Descriptor_Heap_Pool *descriptor_heap_pool);
	void update_frame_info_constant_buffer(GPU_Frame_Info *frame_info);
	void begin_frame(u64 frame_number);
	void end_frame(u64 frame_number);

	Array<Buffer *> buffers;
	Array<Texture *> textures;
	
	Resource_Allocator resource_allocator;
	Buffer *create_buffer(Buffer_Type buffer_type, Buffer_Desc *buffer_desc);
	Texture *create_texture(Texture_Desc *texture_desc);
};

const u32 RENDER_TARGET_BUFFER_BUFFER = 0x1;

enum Shader_Register {
	CONSTANT_BUFFER_REGISTER,
	SHADER_RESOURCE_REGISTER
};

struct Render_Command_Buffer {
	Pipeline_State *current_pipeline = NULL;

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

	void apply(Pipeline_State *pipeline_state, u32 flags = 0);
	void apply_compute_pipeline(Pipeline_State *pipeline_state, u32 flags = 0);
	void apply_graphics_pipeline(Pipeline_State *pipeline_state, u32 flags = 0);

	void bind_buffer(u32 shader_register, u32 shader_space, Shader_Register type, Buffer *buffer);
	
	void draw(u32 index_count);

	void begin_frame(Texture *back_buffer)
	{
		back_buffer_texture = back_buffer;
	}

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

	View_Plane window_view_plane;

	Fence frame_fence;
	Array<u64> frame_fence_values;

	Gpu_Device gpu_device;
	Swap_Chain swap_chain;

	Command_Queue copy_queue;
	Command_Queue compute_queue;
	Command_Queue graphics_queue;

	Resource_Allocator resource_allocator;
	Descriptor_Heap_Pool descriptors_pool;
	Pipeline_Resource_Storage pipeline_resource_storage;
	Copy_Manager copy_manager;

	Render_Command_Buffer command_buffer;

	Texture *back_buffer_depth_texture;
	Array<Texture> back_buffer_textures;

	Generate_Mipmaps generate_mipmaps;

	Array<Render_Pass *> passes;

	void init(Win32_Window *win32_window, Variable_Service *variable_service);
	void init_vars(Win32_Window *win32_window, Variable_Service *variable_service);
	void init_passes();

	void resize(u32 window_width, u32 window_height);

	void flush();
	void begin_frame();
	void end_frame(u64 frame_number);
	void render();

	Size_u32 get_window_size();
};
#endif
