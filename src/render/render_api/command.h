#ifndef RENDER_API_COMMAND_H
#define RENDER_API_COMMAND_H

#include <d3d12.h>

#include "base.h"
#include "fence.h"
#include "resource.h"
#include "d3d12_object.h"
#include "pipeline_state.h"
#include "resource_barrier.h"
#include "descriptor_heap.h"

#include "../../libs/color.h"
#include "../../libs/number_types.h"
#include "../../libs/structures/array.h"
#include "../../libs/math/structures.h"

enum Command_List_Type {
	COMMAND_LIST_TYPE_DIRECT,
	COMMAND_LIST_TYPE_BUNDLE,
	COMMAND_LIST_TYPE_COMPUTE,
	COMMAND_LIST_TYPE_COPY
};

struct Command_Allocator : D3D12_Object<ID3D12CommandAllocator> {
	Command_Allocator();
	~Command_Allocator();

	void reset();
	void create(Gpu_Device &device, Command_List_Type command_list_type);
};

struct Command_List : D3D12_Object<ID3D12GraphicsCommandList> {
	Command_List();
	virtual ~Command_List();

	Array<Command_Allocator> command_allocators;

	void close();
	void reset(u32 command_allocator_index);
	
	void set_pipeline_state(Pipeline_State &pipeline_state);
	void set_descriptor_heaps(CBSRUA_Descriptor_Heap &cbsrua_descriptor_heap, Sampler_Descriptor_Heap &sampler_descriptor_heap);
	
	virtual void create(Gpu_Device &device, u32 number_command_allocators, Command_List_Type command_list_type);
	virtual ID3D12CommandList *get_d3d12_command_list() = 0;
};

struct Graphics_Command_List : Command_List {
	Graphics_Command_List();
	~Graphics_Command_List();

	void set_primitive_type(Primitive_Type primitive_type);
	void set_viewport(const Viewport &viewport);
	void set_clip_rect(const Rect_u32 &clip_rect);

	void clear_render_target_view(RT_Descriptor &descriptor, const Color &color);
	void clear_depth_stencil_view(DS_Descriptor &descriptor, float depth = 1.0f, u8 stencil = 0);
	void resource_barrier(const Resource_Barrier &resource_barrier);

	void set_vertex_buffer(GPU_Resource &resource);
	void set_index_buffer(GPU_Resource &resource);
	
	void set_root_descriptor_table(u32 parameter_index, GPU_Descriptor &descriptor);

	void create(Gpu_Device &device, u32 number_command_allocators);

	ID3D12CommandList *get_d3d12_command_list();
};

struct Subresource_Info {
	u32 width = 0;
	u32 height = 0;
	u32 depth = 1;
	u32 row_pitch = 0;
	DXGI_FORMAT format;
};

struct Copy_Command_List : Command_List {
	Copy_Command_List();
	~Copy_Command_List();

	void create(Gpu_Device &device, u32 number_command_allocators);
	ID3D12CommandList *get_d3d12_command_list();

	void copy_resources(GPU_Resource &dest, GPU_Resource &source);
	void copy_texture(GPU_Resource &dest, GPU_Resource &source, Subresource_Info &subresource_info);
};

struct Compute_Command_List : Command_List {
	Compute_Command_List();
	~Compute_Command_List();

	template <typename T>
	void set_constants(u32 parameter_index, const T &data);
	void set_root_descriptor_table(u32 parameter_index, GPU_Descriptor &descriptor);

	void set_root_signature(Root_Signature &root_signature);

	void dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z = 1);

	void create(Gpu_Device &device, u32 number_command_allocators);
	ID3D12CommandList *get_d3d12_command_list();
};

template<typename T>
inline void Compute_Command_List::set_constants(u32 parameter_index, const T &data)
{
	assert(sizeof(T) % 4 == 0);

	d3d12_object->SetComputeRoot32BitConstants(parameter_index, sizeof(T) / 4, (void *)&data, 0);
}

struct Command_Queue : D3D12_Object<ID3D12CommandQueue> {
	Command_Queue();
	~Command_Queue();

	void create(Gpu_Device &device, Command_List_Type command_list_type);
	void execute_command_list(Command_List &command_list);
	
	u64 signal(u64 &fence_value, Fence &fence);
};

#endif
