#ifndef D3D12_DEVICE_H
#define D3D12_DEVICE_H

#include <dxgi1_4.h>
#include <d3d12.h>
#include <wrl/client.h>

#include "d3d12_descriptors.h"

#include "../render_api/render.h"
#include "../render_api/base_structs.h"
#include "../../libs/number_types.h"
#include "../../libs/math/structures.h"
#include "../../libs/structures/queue.h"

using Microsoft::WRL::ComPtr;

struct Resource_Desc;
struct D3D12_Resource;
struct D3D12_Buffer;
struct D3D12_Texture;
struct D3D12_Render_Device;
struct Descriptor_Heap_Pool;

const u32 HLSL_REGISTRE_COUNT = 20;
const u32 HLSL_SPACE_COUNT = 20;

const u32 SHADER_REGISTER_TYPES_NUMBER = 4;

struct Root_Parameter_Index {
	~Root_Parameter_Index();
	Root_Parameter_Index();

	u32 indices[SHADER_REGISTER_TYPES_NUMBER];

	void set_parameter_index(u32 parameter_index, Shader_Register register_type);
	u32 get_parameter_index(Shader_Register register_type);
};

struct Root_Parameter {
	Root_Parameter();
	Root_Parameter(D3D12_ROOT_PARAMETER_TYPE type);
	~Root_Parameter();

	Root_Parameter(const Root_Parameter &t);
	Root_Parameter &operator=(const Root_Parameter &t);

	Array<D3D12_DESCRIPTOR_RANGE1> d3d12_descriptor_ranges;
	D3D12_ROOT_PARAMETER1 d3d12_parameter;

	void add_32bit_constants(u32 shader_register, u32 register_space, u32 structure_size);
	void add_descriptor(u32 shader_register, u32 register_space);
	void add_descriptor_range(D3D12_DESCRIPTOR_RANGE_TYPE range_type, u32 shader_register, u32 register_space, u32 descriptors_number);
	void add_srv_descriptor_range(u32 shader_register, u32 register_space, u32 descriptors_number);
	void add_sampler_descriptor_range(u32 shader_register, u32 register_space, u32 descriptors_number);
};

struct D3D12_Root_Signature : Root_Signature {
	D3D12_Root_Signature(D3D12_Render_Device *render_device);
	~D3D12_Root_Signature();

	D3D12_Render_Device *render_device = NULL;

	Array<Root_Parameter> parameters;
	Root_Parameter_Index parameters_table[HLSL_REGISTRE_COUNT][HLSL_SPACE_COUNT];
	ComPtr<ID3D12RootSignature> d3d12_root_signature;

	void store_parameter_index(u32 parameter_index, u32 shader_register, u32 shader_space, Shader_Register register_type);
	u32 get_parameter_index(u32 shader_register, u32 shader_space, Shader_Register register_type);

	void compile(u32 access_flags = 0);

	void add_32bit_constants_parameter(u32 shader_register, u32 register_space, u32 struct_size);
	void add_constant_buffer_parameter(u32 shader_register, u32 register_space);
	void add_shader_resource_parameter(u32 shader_register, u32 register_space, u32 descriptors_number);
	void add_sampler_parameter(u32 shader_register, u32 register_space, u32 descriptors_number);

	ID3D12RootSignature *get();
};

struct D3D12_Pipeline_State : Pipeline_State {
	D3D12_Pipeline_State(ComPtr<ID3D12Device> &device, Compute_Pipeline_Desc *pipeline_desc);
	D3D12_Pipeline_State(ComPtr<ID3D12Device> &device, Graphics_Pipeline_Desc *pipeline_desc);
	~D3D12_Pipeline_State();

	ComPtr<ID3D12PipelineState> d3d12_pipeline;

	ID3D12PipelineState *get();
};

struct D3D12_Command_List : Graphics_Command_List {
	D3D12_Command_List(Command_List_Type command_list_type, D3D12_Render_Device *_render_device);
	~D3D12_Command_List();

	D3D12_Render_Device *render_device = NULL;
	D3D12_Root_Signature *last_set_root_signature = NULL;

	ComPtr<ID3D12CommandAllocator> command_allocator;
	ComPtr<ID3D12GraphicsCommandList> command_list;

	ID3D12GraphicsCommandList *get();

	void reset();
	void close();

	// Copy command list methods
	void copy(Buffer *dest, Buffer *source);
	void copy(D3D12_Resource *dest, D3D12_Resource *source);
	void copy_buffer_to_texture(Texture *texture, Buffer *buffer, Subresource_Footprint *subresource_footprint = NULL);
	void copy_buffer_to_texture(D3D12_Resource *texture, D3D12_Resource *buffer, Subresource_Footprint *subresource_footprint);
	
	void transition_resource_barrier(Buffer *buffer, Resource_State state_before, Resource_State state_after);
	void transition_resource_barrier(Texture *texture, Resource_State state_before, Resource_State state_after, u32 subresource = 0);

	// Compute command list methods
	void apply(Pipeline_State *pipeline_state);

	void set_compute_constants(u32 shader_register, u32 shader_space, u32 data_size, void *data);
	void set_compute_descriptor_table(u32 shader_register, u32 shader_space, Shader_Register register_type, GPU_Descriptor *base_descriptor);

	void dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z);

	// Graphics command list methods
	void set_graphics_root_signature(Root_Signature *root_signature);
	void set_primitive_type(Primitive_Type primitive_type);
	void set_viewport(Viewport viewport, bool setup_clip_rect = true);
	void set_clip_rect(Rect_u32 clip_rect);

	void clear_render_target_view(RTV_Descriptor *descriptor, const Color &color);
	void clear_depth_stencil_view(DSV_Descriptor *descriptor, float depth = 1.0f, u8 stencil = 0);

	void set_render_target(RTV_Descriptor *render_target_descriptor, DSV_Descriptor *depth_stencil_descriptor);

	void set_vertex_buffer(Buffer *buffer);
	void set_index_buffer(Buffer *buffer);

	void set_graphics_constant_buffer(u32 shader_register, u32 shader_space, Buffer *constant_buffer);

	void set_graphics_constants(u32 shader_register, u32 shader_space, u32 data_size, void *data);
	void set_graphics_descriptor_table(u32 shader_register, u32 shader_space, Shader_Register register_type, GPU_Descriptor *base_descriptor);

	void draw(u32 vertex_count);
	void draw_indexed(u32 index_count);
};

struct D3D12_Fence : Fence {
	D3D12_Fence(ComPtr<ID3D12Device> &device, u64 initial_expected_value);
	virtual ~D3D12_Fence();

	HANDLE handle;
	ComPtr<ID3D12Fence> d3d12_fence;

	bool wait_for_gpu();
	bool wait_for_gpu(u64 other_expected_value);
	u64 get_completed_value();
	u64 increment_expected_value();
	ID3D12Fence *get();
};

struct D3D12_Command_Queue : Command_Queue {
	D3D12_Command_Queue(ComPtr<ID3D12Device> &device, Command_List_Type command_list_type);
	~D3D12_Command_Queue();

	ComPtr<ID3D12CommandQueue> d3d12_command_queue;

	void signal(Fence *fence);
	void wait(Fence *fence);
	void execute_command_list(Command_List *command_list);
	ID3D12CommandQueue *get();
};

struct Resource_Allocation_Info {
	u64 size = 0;
	u64 alignment = 0;
};

struct D3D12_Render_Device : Render_Device {
	D3D12_Render_Device(u64 initial_expected_value, ComPtr<ID3D12Device> &_device);
	~D3D12_Render_Device();

	u64 frame_number = 0;

	Descriptor_Heap_Pool *descriptor_pool = NULL;

	ComPtr<ID3D12Device> device;

	Array<D3D12_Buffer *> buffers;
	Queue<Pair<u64, D3D12_Resource *>> resource_release_queue2;
	Queue<Pair<u64, ComPtr<ID3D12Resource>>> resource_release_queue;

	D3D12_Command_List *current_upload_command_list = NULL;
	Queue<Pair<u64, D3D12_Command_List *>> flight_command_lists;
	Queue<D3D12_Command_List *> completed_command_lists;

	D3D12_Fence *copy_fence = NULL;
	D3D12_Command_Queue *copy_queue = NULL;
	
	void finish_frame(u64 completed_frame);
	
	Buffer *create_buffer(Buffer_Desc *buffer_desc);
	Texture *create_texture(Texture_Desc *texture_desc);
	
	Fence *create_fence(u64 initial_expected_value);
	Sampler *create_sampler(Sampler_Filter filter, Address_Mode uvw);
	//GPU_Heap *create_gpu_heap(u64 size, GPU_Heap_Type heap_type, GPU_Heap_Content conten);
	
	Command_List *create_command_list(Command_List_Type type);
	Copy_Command_List *create_copy_command_list();
	Compute_Command_List *create_compute_command_list();
	Graphics_Command_List *create_graphics_command_list();
	
	Command_Queue *create_command_queue(Command_List_Type command_list_type, const char *name = NULL);
	Root_Signature *create_root_signature();
	
	Pipeline_State *create_pipeline_state(Compute_Pipeline_Desc *pipeline_desc);
	Pipeline_State *create_pipeline_state(Graphics_Pipeline_Desc *pipeline_desc);

	Fence *execute_uploading();

	//internal
	void safe_release(D3D12_Resource *resource, u64 resource_frame_number = 0);
	void safe_release(ComPtr<ID3D12Resource> &resource, u64 resource_frame_number = 0);
	
	D3D12_Command_List *upload_command_list();

	Resource_Allocation_Info resource_allocation_info(Resource_Desc *resource_desc);

	GPU_Descriptor *base_sampler_descriptor();
	GPU_Descriptor *base_shader_resource_descriptor();
};

D3D12_Render_Device *create_d3d12_render_device(u64 initial_expected_value);

struct D3D12_Swap_Chain : Swap_Chain {
	D3D12_Swap_Chain(bool allow_tearing, u32 buffer_count, u32 width, u32 height, HWND handle, Command_Queue *command_queue);
	virtual ~D3D12_Swap_Chain();

	u32 back_buffer_count = 0;
	D3D12_Texture *depth_buffer = NULL;
	Array<D3D12_Texture *> back_buffers;
	ComPtr<IDXGISwapChain3> dxgi_swap_chain;

	void resize(u32 width, u32 height);
	void present(u32 sync_interval, u32 flags);
	
	Texture *get_back_buffer();
	Texture *get_depth_stencil_buffer();
	
	u32 get_current_back_buffer_count();
	u32 get_current_back_buffer_index();
};

#endif