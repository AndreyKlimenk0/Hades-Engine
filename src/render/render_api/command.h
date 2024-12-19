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
	void reset(u32 command_allocator_index, Pipeline_State &pipeline_state);
	
	virtual void create(Gpu_Device &device, u32 number_command_allocators, Command_List_Type command_list_type);
	virtual ID3D12CommandList *get_d3d12_command_list() = 0;
};

struct Graphics_Command_List : Command_List {
	Graphics_Command_List();
	~Graphics_Command_List();

	void clear_render_target_view(RT_Descriptor &descriptor, const Color &color);
	void clear_depth_stencil_view(DS_Descriptor &descriptor, float depth = 1.0f, u8 stencil = 0);
	void resource_barrier(const Resource_Barrier &resource_barrier);

	void set_vertex_buffer(GPU_Resource &resource);
	void set_index_buffer(GPU_Resource &resource);

	void create(Gpu_Device &device, u32 number_command_allocators);

	ID3D12CommandList *get_d3d12_command_list();
};

struct Copy_Command_List : Command_List {
	Copy_Command_List();
	~Copy_Command_List();

	void reset(Command_Allocator &command_allocator);
	void create(Gpu_Device &device, u32 number_command_allocators);
	ID3D12CommandList *get_d3d12_command_list();
};

struct Command_Queue : D3D12_Object<ID3D12CommandQueue> {
	Command_Queue();
	~Command_Queue();

	void create(Gpu_Device &device, Command_List_Type command_list_type);
	void execute_command_list(Command_List &command_list);
	
	u64 signal(u64 &fence_value, Fence &fence);
};

#endif