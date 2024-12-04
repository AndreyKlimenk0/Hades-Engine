#ifndef RENDER_API_DESCRIPTOR_HEAP
#define RENDER_API_DESCRIPTOR_HEAP

#include <d3d12.h>

#include "base.h"
#include "resource.h"
#include "d3d12_object.h"
#include "../../libs/number_types.h"

enum Descriptor_Heap_Type {
	DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
	DESCRIPTOR_HEAP_TYPE_SAMPLER,
	DESCRIPTOR_HEAP_TYPE_RTV,
	DESCRIPTOR_HEAP_TYPE_DSV
};

struct Descriptor_Heap : D3D12_Object<ID3D12DescriptorHeap> {
	Descriptor_Heap();
	virtual ~Descriptor_Heap();

	u32 increment_size = 0;
	u32 descriptor_heap_capacity = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
	
	Gpu_Device gpu_device;

	void create(Gpu_Device &device, u32 descriptors_number, Descriptor_Heap_Type descriptor_heap_type);
	
	D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_handle(u32 descriptor_index);
	D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_handle(u32 descriptor_index);
	D3D12_GPU_DESCRIPTOR_HANDLE get_base_gpu_handle();
};

struct Shader_Descriptor_Heap : Descriptor_Heap {
	Shader_Descriptor_Heap();
	~Shader_Descriptor_Heap();

	void place_cb_decriptor(u32 descriptor_index, GPU_Resource &resource);
	void place_sr_decriptor(u32 descriptor_index, GPU_Resource &resource);
	void place_ua_decriptor(u32 descriptor_index, GPU_Resource &resource);
	void create(Gpu_Device &device, u32 descriptors_number);
};

struct Sampler_Descriptor_Heap : Descriptor_Heap {
	Sampler_Descriptor_Heap();
	~Sampler_Descriptor_Heap();

	void place_decriptor(u32 descriptor_index, GPU_Resource &resource);
	void create(Gpu_Device &device, u32 descriptors_number);
};

struct RT_Descriptor_Heap : Descriptor_Heap {
	RT_Descriptor_Heap();
	~RT_Descriptor_Heap();

	void create(Gpu_Device &device, u32 descriptors_number);
	void place_decriptor(u32 descriptor_index, GPU_Resource &resource);
};

struct DS_Descriptor_Heap : Descriptor_Heap {
	DS_Descriptor_Heap();
	~DS_Descriptor_Heap();

	void create(Gpu_Device &device, u32 descriptors_number);
	void place_decriptor(u32 descriptor_index, GPU_Resource &resource);
};

#endif