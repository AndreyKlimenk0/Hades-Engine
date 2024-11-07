#ifndef NEW_RENDER_API
#define NEW_RENDER_API

#include <d3d12.h>
#include <dxgi1_4.h>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

typedef ComPtr<ID3D12CommandQueue> Command_Queue;
typedef ComPtr<ID3D12DescriptorHeap> Descriptor_Heap;
typedef ComPtr<ID3D12Resource> GPU_Resource;

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

namespace d3d12 {
	enum Descriptr_Heap_Type {
		DESCRIPTOR_HEAP_TYPE_RTV,
	};

	struct Gpu_Device {
		ComPtr<ID3D12Device> device;

		bool init();
		void release();

		void create_command_queue(Command_Queue &command_queue);
		void create_rtv_descriptor_heap(u32 descriptor_count, Descriptor_Heap &descriptor_heap);
	};

	struct Swap_Chain {
		ComPtr<IDXGISwapChain3> dxgi_swap_chain;

		void init(u32 buffer_count, u32 width, u32 height, HWND handle, const ComPtr<ID3D12CommandQueue> &command_queue);
		void resize(u32 width, u32 height);
		void get_buffer(u32 buffer_index, GPU_Resource &resource);
		void release();
	};
};

#endif
