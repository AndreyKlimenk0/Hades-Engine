#ifndef RENDER_DEVICE_H
#define RENDER_DEVICE_H

#include <limits.h>

#include "base_types.h"
#include "base_structs.h"
#include "../../libs/str.h"
#include "../../libs/color.h"
#include "../../libs/number_types.h"

struct CPU_Descriptor {
	CPU_Descriptor() = default;
	virtual ~CPU_Descriptor() = default;
	
	virtual bool valid() = 0;
	virtual u32 index() = 0;
};

struct GPU_Descriptor {
	GPU_Descriptor() = default;
	virtual ~GPU_Descriptor() = default;
	
	virtual bool valid() = 0;
	virtual u32 index() = 0;
};

typedef GPU_Descriptor CBV_Descriptor;
typedef GPU_Descriptor SRV_Descriptor;
typedef GPU_Descriptor UAV_Descriptor;
typedef GPU_Descriptor Sampler_Descriptor;
typedef CPU_Descriptor RTV_Descriptor;
typedef CPU_Descriptor DSV_Descriptor;

struct Buffer {
	Buffer() = default;
	virtual ~Buffer() = default;

	virtual u64 size() = 0;
	virtual u64 gpu_virtual_address() = 0;
	virtual void request_write() = 0; // Call only for default buffer
	virtual void write(void *data, u64 data_size, u64 alignment = 0) = 0;

	virtual CBV_Descriptor *constant_buffer_descriptor() = 0;
	virtual SRV_Descriptor *shader_resource_descriptor(u32 mipmap_level = 0) = 0;
	virtual UAV_Descriptor *unordered_access_descriptor(u32 mipmap_level = 0) = 0;
};

struct Texture {
	Texture() = default;
	virtual ~Texture() = default;

	virtual u32 subresource_count() = 0;
	virtual Subresource_Footprint subresource_footprint(u32 subresource_index) = 0;

	virtual Texture_Desc get_texture_desc() = 0;
	virtual SRV_Descriptor *shader_resource_descriptor(u32 mipmap_level = 0) = 0;
	virtual UAV_Descriptor *unordered_access_descriptor(u32 mipmap_level = 0) = 0;
	virtual DSV_Descriptor *depth_stencil_descriptor() = 0;
	virtual RTV_Descriptor *render_target_descriptor() = 0;
};

struct Sampler {
	Sampler() = default;
	virtual ~Sampler() = default;

	virtual Sampler_Descriptor *sampler_descriptor() = 0;
};

const u32 ALLOW_INPUT_LAYOUT_ACCESS = 0x1;
const u32 ALLOW_VERTEX_SHADER_ACCESS = 0x2;
const u32 ALLOW_PIXEL_SHADER_ACCESS = 0x4;
const u32 ALLOW_HULL_SHADER_ACCESS = 0x8;
const u32 ALLOW_DOMAIN_SHADER_ACCESS = 0x10;
const u32 ALLOW_GEOMETRY_SHADER_ACCESS = 0x20;
const u32 ALLOW_AMPLIFICATION_SHADER_ACCESS = 0x40;
const u32 ALLOW_MESH_SHADER_ACCESS = 0x80;

const u32 UNBOUNDED_DESCRIPTORS_NUMBER = UINT_MAX;

struct Root_Signature  {
	Root_Signature() = default;
	virtual ~Root_Signature() = default;
	
	virtual void compile(u32 access_flags = 0) = 0;

	virtual void add_32bit_constants_parameter(u32 shader_register, u32 register_space, u32 struct_size) = 0;
	virtual void add_constant_buffer_parameter(u32 shader_register, u32 register_space) = 0;
	//virtual void add_constant_buffer_parameter(u32 shader_register, u32 register_space, u32 descriptors_number = 1) = 0;
	virtual void add_shader_resource_parameter(u32 shader_register, u32 register_space, u32 descriptors_number = 1) = 0;
	virtual void add_sampler_parameter(u32 shader_register, u32 register_space, u32 descriptors_number = 1) = 0;
	//virtual void add_unordered_access_resource_parameter(u32 shader_register, u32 register_space, u32 descriptors_number = 1) = 0;
	//virtual void add_sampler_parameter(u32 shader_register, u32 register_space) = 0;
};

struct Pipeline_State {
	Pipeline_State() = default;
	virtual ~Pipeline_State() = default;

	Pipeline_Type type = PIPELINE_TYPE_UNKNOWN;
	Primitive_Type primitive_type = PRIMITIVE_TYPE_UNKNOWN;
	Root_Signature *root_signature = NULL;
};

struct Command_List {
	Command_List() = default;
	virtual ~Command_List() = default;

	Command_List_Type type;

	virtual void close() = 0;
	virtual void reset() = 0;
};

struct Copy_Command_List : Command_List {
	Copy_Command_List() = default;
	virtual ~Copy_Command_List() = default;

	virtual void copy(Buffer *dest, Buffer *source) = 0;
	virtual void copy_buffer_to_texture(Texture *texture, Buffer *buffer, Subresource_Footprint *subresource_footprint = NULL) = 0;

	virtual void transition_resource_barrier(Buffer *buffer, Resource_State state_before, Resource_State state_after) = 0;
	virtual void transition_resource_barrier(Texture *texture, Resource_State state_before, Resource_State state_after, u32 subresource = 0) = 0;
};

struct Compute_Command_List : Copy_Command_List {
	Compute_Command_List() = default;
	virtual ~Compute_Command_List() = default;

	virtual void apply(Pipeline_State *pipeline_state) = 0;
	
	//virtual void set_compute_constants(u32 parameter_index, void *data) = 0;
	//virtual void set_compute_root_descriptor_table(u32 parameter_index, const GPU_Descriptor &base_descriptor) = 0;

	virtual void set_compute_constants(u32 shader_register, u32 shader_space, u32 data_size, void *data) = 0;
	virtual void set_compute_descriptor_table(u32 shader_register, u32 shader_space, Shader_Register register_type, GPU_Descriptor *base_descriptor) = 0;

	virtual void dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z = 1) = 0;
};

struct Graphics_Command_List : Compute_Command_List {
	Graphics_Command_List() = default;
	virtual ~Graphics_Command_List() = default;

	virtual void begin_event(const char *name) = 0;
	virtual void end_event() = 0;

	virtual void set_graphics_root_signature(Root_Signature *root_signature) = 0;

	virtual void set_primitive_type(Primitive_Type primitive_type) = 0;
	virtual void set_viewport(Viewport viewport, bool setup_clip_rect = true) = 0;
	virtual void set_clip_rect(Rect_u32 clip_rect) = 0;
	
	virtual void clear_render_target_view(RTV_Descriptor *descriptor, const Color &color) = 0;
	virtual void clear_depth_stencil_view(DSV_Descriptor *descriptor, float depth = 1.0f, u8 stencil = 0) = 0;
	
	virtual void set_render_target(RTV_Descriptor *render_target_descriptor, DSV_Descriptor *depth_stencil_descriptor) = 0;

	// ?
	void set_render_target(Texture *render_target_texture, Texture *depth_stencil_texture);
	void clear_render_target(Texture *texture, const Color &color);
	void clear_depth_stencil(Texture *texture, float depth = 1.0f, u8 stencil = 0);
	
	virtual void set_vertex_buffer(Buffer *buffer) = 0;
	virtual void set_index_buffer(Buffer *buffer) = 0;
	
	//virtual void set_graphics_constants(u32 parameter_index, void *data) = 0;
	//virtual void set_graphics_root_descriptor_table(u32 parameter_index, GPU_Descriptor *base_descriptor) = 0;
	//virtual void set_graphics_constatns(u32 shader_register, u32 shader_space, Shader_Register register_type) = 0;
	//SetGraphicsRootConstantBuffer
	virtual void set_graphics_constant_buffer(u32 shader_register, u32 shader_space, Buffer *constant_buffer) = 0;

	template <typename T>
	void set_graphics_constants(u32 shader_register, u32 shader_space, T *data);
	virtual void set_graphics_constants(u32 shader_register, u32 shader_space, u32 data_size, void *data) = 0;

	virtual void set_graphics_descriptor_table(u32 shader_register, u32 shader_space, Shader_Register register_type, GPU_Descriptor *base_descriptor) = 0;
	
	virtual void draw(u32 vertex_count) = 0;
	virtual void draw_indexed(u32 index_count) = 0;
	virtual void draw_indexed(u32 index_count, u32 index_offset, u32 vertex_offset) = 0;
};

template <typename T>
inline void Graphics_Command_List::set_graphics_constants(u32 shader_register, u32 shader_space, T *data)
{
	set_graphics_constants(shader_register, shader_space, sizeof(T), (void *)data);
}

inline void Graphics_Command_List::set_render_target(Texture *render_target_texture, Texture *depth_stencil_texture)
{
	set_render_target(render_target_texture ? render_target_texture->render_target_descriptor() : NULL, depth_stencil_texture->depth_stencil_descriptor());
}

inline void Graphics_Command_List::clear_render_target(Texture *texture, const Color &color)
{
	clear_render_target_view(texture->render_target_descriptor(), color);
}
inline void Graphics_Command_List::clear_depth_stencil(Texture *texture, float depth, u8 stencil)
{
	clear_depth_stencil_view(texture->depth_stencil_descriptor(), depth, stencil);
}

struct Fence {
	Fence() = default;
	virtual ~Fence() = default;

	u64 expected_value = 0;

	virtual bool wait_for_gpu() = 0;
	virtual bool wait_for_gpu(u64 other_expected_value) = 0;
	virtual u64 get_completed_value() = 0;
	virtual u64 increment_expected_value() = 0;
};

struct Command_Queue {
	Command_Queue() = default;
	~Command_Queue() = default;

	virtual void signal(Fence *fence) = 0;
	virtual void wait(Fence *fence) = 0;
	virtual void execute_command_list(Command_List *command_list) = 0;
};

struct GPU_Heap {
	HRESULT QueryInterface(REFIID riid, void **object);
};

struct Render_Device {
	Render_Device() = default;
	virtual ~Render_Device() = default;

	virtual void finish_frame(u64 completed_frame) = 0;
	
	virtual Fence *create_fence(u64 initial_expected_value = 0) = 0;

	virtual Buffer *create_buffer(Buffer_Desc *buffer_desc) = 0;
	virtual Texture *create_texture(Texture_Desc *texture_desc) = 0;
	virtual Sampler *create_sampler(Sampler_Filter filter, Address_Mode uvw) = 0;
	
	//virtual GPU_Heap *create_gpu_heap(u64 size, GPU_Heap_Type heap_type, GPU_Heap_Content conten) = 0;

	virtual Command_List *create_command_list(Command_List_Type type) = 0;

	virtual Copy_Command_List *create_copy_command_list() = 0;
	virtual Compute_Command_List *create_compute_command_list() = 0;
	virtual Graphics_Command_List *create_graphics_command_list() = 0;

	virtual Command_Queue *create_command_queue(Command_List_Type command_list_type, const char *name = NULL) = 0;

	virtual Root_Signature *create_root_signature() = 0;

	virtual Pipeline_State *create_pipeline_state(Compute_Pipeline_Desc *pipeline_desc) = 0;
	virtual Pipeline_State *create_pipeline_state(Graphics_Pipeline_Desc *pipeline_desc) = 0;

	virtual Fence *execute_uploading() = 0;

	virtual GPU_Descriptor *base_sampler_descriptor() = 0;
	virtual GPU_Descriptor *base_shader_resource_descriptor() = 0;
};

Render_Device *create_render_device(u64 initial_expected_value);

struct Swap_Chain {
	Swap_Chain() = default;
	virtual ~Swap_Chain() = default;

	virtual void resize(u32 width, u32 height) = 0;
	virtual void present(u32 sync_interval, u32 flags) = 0;
	
	virtual Texture *get_back_buffer() = 0;
	virtual Texture *get_depth_stencil_buffer() = 0;

	virtual u32 get_current_back_buffer_index() = 0;
};

Swap_Chain *create_swap_chain(bool allow_tearing, u32 buffer_count, u32 width, u32 height, HWND handle, Command_Queue *command_queue);

#endif