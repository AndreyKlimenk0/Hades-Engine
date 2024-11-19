#ifndef NEW_RENDER_API
#define NEW_RENDER_API

#include <d3d12.h>
#include <dxgi1_4.h>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

typedef ComPtr<ID3D12CommandQueue> Command_Queue;
typedef ComPtr<ID3D12Resource> GPU_Resource;
typedef ComPtr<ID3D12PipelineState> GPU_Pipeline_State;
typedef ComPtr<ID3D12Fence> Fence;
typedef ComPtr<ID3D12CommandAllocator> Command_Allocator;

struct Graphics_Command_List {
	Graphics_Command_List();
	~Graphics_Command_List();

	ComPtr<ID3D12CommandAllocator> command_allocator;
	ComPtr<ID3D12GraphicsCommandList> command_list;

	void set_command_allocator(Command_Allocator &_command_allocator);
	void close();
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
	D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_heap_descriptro_handle(u32 descriptor_index);
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
		void create_command_list(const GPU_Pipeline_State &pipeline_state, Graphics_Command_List &command_list);
		void create_command_list(Graphics_Command_List &command_list);
		void create_rtv_descriptor_heap(u32 descriptor_count, Descriptor_Heap &descriptor_heap);
	};

	struct Swap_Chain {
		ComPtr<IDXGISwapChain3> dxgi_swap_chain;

		void init(bool allow_tearing, u32 buffer_count, u32 width, u32 height, HWND handle, const ComPtr<ID3D12CommandQueue> &command_queue);
		void resize(u32 width, u32 height);
		void present(u32 sync_interval, u32 flags);
		void get_buffer(u32 buffer_index, GPU_Resource &resource);
		void release();

		u32 get_current_back_buffer_index();
	};
};


bool check_tearing_support();

#endif
