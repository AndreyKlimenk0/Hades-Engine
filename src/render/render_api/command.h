#ifndef RENDER_API_COMMAND_H
#define RENDER_API_COMMAND_H

#include <d3d12.h>

#include "base.h"
#include "fence.h"
#include "d3d12_object.h"
#include "pipeline_state.h"
#include "resource_barrier.h"
#include "descriptor_heap.h"

#include "../../libs/color.h"
#include "../../libs/number_types.h"

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

	void close();
	void create(Gpu_Device &device, Command_List_Type command_list_type, Command_Allocator &command_allocator);
	virtual ID3D12CommandList *get_d3d12_command_list() = 0;
};

struct Graphics_Command_List : Command_List {
	Graphics_Command_List();
	~Graphics_Command_List();

	void reset(Command_Allocator &command_allocator);
	void reset(Command_Allocator &command_allocator, Pipeline_State &pipeline_state);
	void clear_render_target_view(D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle, const Color &color);
	void clear_depth_stencil_view(DS_Descriptor &descriptor, float depth = 1.0f, u8 stencil = 0);
	void resource_barrier(const Resource_Barrier &resource_barrier);

	void create(Gpu_Device &device, Command_Allocator &command_allocator);

	ID3D12CommandList *get_d3d12_command_list();
};

struct Copy_Command_List : Command_List {
	Copy_Command_List();
	~Copy_Command_List();

	void reset(Command_Allocator &command_allocator);
	void create(Gpu_Device &device, Command_Allocator &command_allocator);
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