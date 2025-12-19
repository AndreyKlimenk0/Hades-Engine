#ifndef RENDER_DEVICE_H
#define RENDER_DEVICE_H

#include <limits.h>

#include "render_types.h"
#include "render_base.h"
#include "../../libs/str.h"
#include "../../libs/color.h"
#include "../../libs/number_types.h"

struct CPU_Descriptor {
	CPU_Descriptor();
	virtual ~CPU_Descriptor();
	
	virtual bool valid() = 0;
	virtual u32 index() = 0;
};

struct GPU_Descriptor {
	GPU_Descriptor();
	virtual ~GPU_Descriptor();
	
	virtual bool valid() = 0;
	virtual u32 index() = 0;
};

struct CB_Descriptor : GPU_Descriptor {
	using GPU_Descriptor::GPU_Descriptor;
};

struct SR_Descriptor : GPU_Descriptor {
	using GPU_Descriptor::GPU_Descriptor;
};

struct UA_Descriptor : GPU_Descriptor {
	using GPU_Descriptor::GPU_Descriptor;
};

struct Sampler_Descriptor : GPU_Descriptor {
	using GPU_Descriptor::GPU_Descriptor;
};

struct RT_Descriptor : CPU_Descriptor {
	using CPU_Descriptor::CPU_Descriptor;
};

struct DS_Descriptor : CPU_Descriptor {
	using CPU_Descriptor::CPU_Descriptor;
};

struct Buffer {
	Buffer();
	virtual ~Buffer();

	//virtual Buffer_Desc buffer_desc() = 0;
	virtual u64 size();
	virtual void write(void *data, u64 data_size, u64 alignment = 0) = 0;

	virtual CB_Descriptor *constant_buffer_descriptor() = 0;
	virtual SR_Descriptor *shader_resource_descriptor(u32 mipmap_level = 0) = 0;
	virtual UA_Descriptor *unordered_access_descriptor(u32 mipmap_level = 0) = 0;
};

struct Texture {
	Texture();
	virtual ~Texture();

	virtual Texture_Desc texture_desc() = 0;
	virtual SR_Descriptor *shader_resource_descriptor(u32 mipmap_level = 0) = 0;
	virtual UA_Descriptor *unordered_access_descriptor(u32 mipmap_level = 0) = 0;
	virtual DS_Descriptor *depth_stencil_descriptor() = 0;
	virtual RT_Descriptor *render_target_descriptor() = 0;
};

const u32 UNBOUNDED_DESCRIPTORS_NUMBER = UINT_MAX;

struct Root_Signature  {
	Root_Signature();
	virtual ~Root_Signature();
	
	virtual void compile(u32 access_flags = 0) = 0;

	virtual void add_32bit_constants_parameter(u32 shader_register, u32 register_space, u32 struct_size) = 0;
	
	virtual void add_constant_buffer_parameter(u32 shader_register, u32 register_space, u32 descriptors_number = 1) = 0;
	virtual void add_shader_resource_parameter(u32 shader_register, u32 register_space, u32 descriptors_number = 1) = 0;
	virtual void add_unordered_access_resource_parameter(u32 shader_register, u32 register_space, u32 descriptors_number = 1) = 0;
	virtual void add_sampler_parameter(u32 shader_register, u32 register_space);

	virtual u32 get_parameter_index(u32 shader_register, u32 shader_space, Shader_Register register_type) = 0;
};

struct Pipeline_State {
	Pipeline_State();
	virtual ~Pipeline_State();

	Pipeline_Type type = PIPELINE_TYPE_UNKNOWN;
	Primitive_Type primitive_type = PRIMITIVE_TYPE_UNKNOWN;
	Root_Signature *root_signature = NULL;
};

struct Subresource_Footprint {
	Subresource_Footprint();
	Subresource_Footprint(u32 subresource_index, u32 row_count, u64 row_size);
	~Subresource_Footprint();

	u32 subresource_index = 0;
	u32 row_count = 0;
	u64 row_size = 0;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT places_subresource_footprint;

	D3D12_SUBRESOURCE_FOOTPRINT d3d12_subresource_footprint();
};

struct Command_List {
	Command_List();
	virtual ~Command_List();

	virtual void close() = 0;
	virtual void reset() = 0;
};

struct Copy_Command_List : Command_List {
	Copy_Command_List();
	virtual ~Copy_Command_List();

	virtual void copy(Buffer *dest, Buffer *source) = 0;
	virtual void copy_buffer_to_texture(Texture *texture, Buffer *buffer, Subresource_Footprint &subresource_footprint) = 0;
};

struct Compute_Command_List : Copy_Command_List {
	Compute_Command_List();
	virtual ~Compute_Command_List();

	virtual void set_pipeline_state(Pipeline_State *pipeline_state) = 0;
	virtual void set_compute_root_signature(Root_Signature *root_signature) = 0;
	
	virtual void set_compute_constants(u32 parameter_index, void *data) = 0;
	virtual void set_compute_root_descriptor_table(u32 parameter_index, const GPU_Descriptor &base_descriptor) = 0;

	virtual void dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z = 1) = 0;
};

struct Graphics_Command_List : Compute_Command_List {
	Graphics_Command_List();
	virtual ~Graphics_Command_List();

	virtual void set_graphics_root_signature(Root_Signature *root_signature) = 0;

	virtual void set_primitive_type(Primitive_Type primitive_type) = 0;
	virtual void set_viewport(Viewport viewport) = 0;
	virtual void set_clip_rect(Rect_u32 clip_rect) = 0;
	
	virtual void clear_render_target_view(RT_Descriptor *descriptor, const Color &color) = 0;
	virtual void clear_depth_stencil_view(DS_Descriptor *descriptor, float depth = 1.0f, u8 stencil = 0) = 0;
	
	virtual void set_vertex_buffer(Buffer *buffer) = 0;
	virtual void set_index_buffer(Buffer *buffer) = 0;
	
	virtual void set_graphics_constants(u32 parameter_index, void *data) = 0;
	virtual void set_graphics_root_descriptor_table(u32 parameter_index, GPU_Descriptor *base_descriptor) = 0;
	virtual void set_render_target(RT_Descriptor *render_target_descriptor, DS_Descriptor *depth_stencil_descriptor) = 0;
	
	virtual void draw(u32 vertex_count) = 0;
	virtual void draw_indexed(u32 index_count) = 0;
};

struct Command_Queue {
	Command_Queue();
	~Command_Queue();

	//void wait(Fence &wait_fence);
	void flush_gpu();
	virtual void execute_command_list(Command_List *command_list) = 0;

	//void signal(Fence &fence);
};

const u32 AUTO_UPLOADING = 0x1;

struct GPU_Heap {
	HRESULT QueryInterface(REFIID riid, void **object);
};

struct Resource_Allocation {
	bool succeeded = false;
	u64 heap_offset = 0;
	GPU_Heap *heap = NULL;
};

struct Resource_Allocator {
	Resource_Allocator();
	~Resource_Allocator();

	virtual Resource_Allocation allocate(u64 size, u64 alignment = 0) = 0;
};

struct Render_API_Context {
	Command_Queue *upload_command_queue = NULL;
	Resource_Allocator *resource_allocator = NULL;
};

struct Render_Device {
	Render_Device();
	virtual ~Render_Device();

	static bool create();

	void set_context();

	virtual Buffer *create_buffer(Buffer_Desc *buffer_desc) = 0;
	virtual Texture *create_texture(Texture_Desc *texture_desc) = 0;
	virtual GPU_Heap *create_gpu_heap(u64 size, GPU_Heap_Type heap_type, GPU_Heap_Content conten) = 0;

	virtual Copy_Command_List *create_copy_command_list() = 0;
	virtual Compute_Command_List *create_compute_command_list() = 0;
	virtual Graphics_Command_List *create_graphics_command_list() = 0;

	virtual Root_Signature *create_root_signature() = 0;

	virtual Pipeline_State *create_pipeline_state(Compute_Pipeline_Desc *pipeline_desc) = 0;
	virtual Pipeline_State *create_pipeline_state(Graphics_Pipeline_Desc *pipeline_desc) = 0;

	virtual void finish_frame() = 0;

	virtual void wait_for_uploading() = 0;
};
#endif