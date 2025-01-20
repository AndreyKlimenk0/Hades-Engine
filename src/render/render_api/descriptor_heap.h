#ifndef RENDER_API_DESCRIPTOR_HEAP
#define RENDER_API_DESCRIPTOR_HEAP

#include <d3d12.h>

#include "base.h"
#include "sampler.h"
#include "resource.h"
#include "d3d12_object.h"
#include "../../libs/number_types.h"

struct CPU_Descriptor {
	CPU_Descriptor();
	CPU_Descriptor(u32 heap_index, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle);
	virtual ~CPU_Descriptor();

	u32 index = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
};

struct GPU_Descriptor : CPU_Descriptor {
	GPU_Descriptor();
	GPU_Descriptor(u32 heap_index, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle);
	virtual ~GPU_Descriptor();
	
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
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

struct CBSRUA_Descriptor_Heap : Descriptor_Heap {
	CBSRUA_Descriptor_Heap();
	~CBSRUA_Descriptor_Heap();

	void create(Gpu_Device &device, u32 descriptors_number);
	CB_Descriptor place_cb_descriptor(u32 descriptor_index, GPU_Resource &resource);
	SR_Descriptor place_sr_descriptor(u32 descriptor_index, GPU_Resource &resource);
	UA_Descriptor place_ua_descriptor(u32 descriptor_index, GPU_Resource &resource, u32 mip_level = 0);
};

struct Sampler_Descriptor_Heap : Descriptor_Heap {
	Sampler_Descriptor_Heap();
	~Sampler_Descriptor_Heap();

	void create(Gpu_Device &device, u32 descriptors_number);
	Sampler_Descriptor place_descriptor(u32 descriptor_index, Sampler &sampler);
};

struct RT_Descriptor_Heap : Descriptor_Heap {
	RT_Descriptor_Heap();
	~RT_Descriptor_Heap();

	void create(Gpu_Device &device, u32 descriptors_number);
	RT_Descriptor place_descriptor(u32 descriptor_index, GPU_Resource &resource);
};

struct DS_Descriptor_Heap : Descriptor_Heap {
	DS_Descriptor_Heap();
	~DS_Descriptor_Heap();

	void create(Gpu_Device &device, u32 descriptors_number);
	DS_Descriptor place_descriptor(u32 descriptor_index, GPU_Resource &resource);
};

#endif