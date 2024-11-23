#ifndef NEW_RENDER_API
#define NEW_RENDER_API

#include <d3d12.h>
#include <dxgi1_4.h>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include "../libs/color.h"
#include "../libs/number_types.h"

typedef ComPtr<ID3D12Resource> GPU_Resource;
typedef ComPtr<ID3D12PipelineState> GPU_Pipeline_State;
typedef ComPtr<ID3D12Fence> Fence;

//enum Resource_Barrier_Type {
//	RESOURCE_BARRIER_TYPE_TRANSITION,
//	RESOURCE_BARRIER_TYPE_ALIASING,
//	RESOURCE_BARRIER_TYPE_UAV
//};

enum Resource_State {
	RESOURCE_STATE_UNKNOWN,
	RESOURCE_STATE_PRESENT,
	RESOURCE_STATE_RENDER_TARGET,
};

struct Transition_Resource_Barrier {
	Transition_Resource_Barrier();
	Transition_Resource_Barrier(GPU_Resource resource, Resource_State state_before, Resource_State state_after);
	Transition_Resource_Barrier(u32 subresources, GPU_Resource resource, Resource_State state_before, Resource_State state_after);
	~Transition_Resource_Barrier();

	u32 subresource;
	GPU_Resource resource;
	Resource_State state_before;
	Resource_State state_after;
};

//template <typename T>
//struct D3D12_Object {
//	D3D12_Object();
//	virtual ~D3D12_Object();
//
//	ComPtr<T> object;
//	
//	T *get();
//	T **get_address();
//	T **release_and_get_address();
//};
//
//template<typename T>
//inline D3D12_Object<T>::D3D12_Object()
//{
//}
//
//template<typename T>
//inline D3D12_Object<T>::~D3D12_Object()
//{
//}
//
//template<typename T>
//inline T *D3D12_Object<T>::get()
//{
//	return object.Get();
//}
//
//template<typename T>
//inline T **D3D12_Object<T>::get_address()
//{
//	return object.GetAddressOf();
//}
//
//template<typename T>
//inline T **D3D12_Object<T>::release_and_get_address()
//{
//	return object.ReleaseAndGetAddressOf();
//}


struct Command_Allocator {
	ComPtr<ID3D12CommandAllocator> command_allocator;

	void reset();
};

struct Command_List {
	Command_List() = default;
	virtual ~Command_List() = default;

	virtual ID3D12CommandList *get_d3d12_command_list() = 0;
};

struct Graphics_Command_List : Command_List {
	Graphics_Command_List();
	~Graphics_Command_List();

	ComPtr<ID3D12GraphicsCommandList> command_list;

	void close();
	void reset(Command_Allocator &command_allocator);
	void clear_render_target_view(D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle, const Color &color);
	void resource_barrier(const Transition_Resource_Barrier &transition_resource_barrier);
	
	ID3D12CommandList *get_d3d12_command_list();
};

struct Command_Queue {
	ComPtr<ID3D12CommandQueue> d3d12_command_queue;

	void execute_command_list(Command_List &command_list);
	u64 signal(u64 &fence_value, const Fence &fence);
};

struct Descriptor_Heap {
	Descriptor_Heap();
	~Descriptor_Heap();

	u32 descriptor_count = 0;
	u32 increment_size = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
	ComPtr<ID3D12DescriptorHeap> heap;

	void release();
	D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_heap_descriptor_handle(u32 descriptor_index);
};

enum Command_List_Type {
	COMMAND_LIST_TYPE_DIRECT,
	COMMAND_LIST_TYPE_BUNDLE,
	COMMAND_LIST_TYPE_COMPUTE,
	COMMAND_LIST_TYPE_COPY
};

namespace d3d12 {
	enum Descriptr_Heap_Type {
		DESCRIPTOR_HEAP_TYPE_RTV,
	};

	struct Gpu_Device {
		ComPtr<ID3D12Device> device;

		bool init();
		void release();

		void create_fence(Fence &fence);
		void create_command_allocator(Command_List_Type command_list_type, Command_Allocator &command_allocator);
		
		void create_command_queue(Command_Queue &command_queue);
		//void create_command_list(const GPU_Pipeline_State &pipeline_state, Graphics_Command_List &command_list);
		void create_command_list(Command_Allocator &command_allocator, Graphics_Command_List &command_list);
		void create_rtv_descriptor_heap(u32 descriptor_count, Descriptor_Heap &descriptor_heap);
	};

	struct Swap_Chain {
		ComPtr<IDXGISwapChain3> dxgi_swap_chain;

		void init(bool allow_tearing, u32 buffer_count, u32 width, u32 height, HWND handle, Command_Queue &command_queue);
		void resize(u32 width, u32 height);
		void present(u32 sync_interval, u32 flags);
		void get_buffer(u32 buffer_index, GPU_Resource &resource);
		void release();

		u32 get_current_back_buffer_index();
	};
};


bool check_tearing_support();

#endif