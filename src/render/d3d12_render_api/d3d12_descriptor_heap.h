#ifndef D3D12_DESCRIPTOR_HEAP_H
#define D3D12_DESCRIPTOR_HEAP_H

#include <d3d12.h>
#include <wrl/client.h>

#include "d3d12_descriptors.h"
#include "d3d12_resources.h"

#include "../../libs/number_types.h"
#include "../../libs/structures/array.h"

using Microsoft::WRL::ComPtr;

struct Descriptor_Heap {
	Descriptor_Heap();
	virtual ~Descriptor_Heap();

	u32 increment_size = 0;
	u32 descriptor_heap_capacity = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;

	ComPtr<ID3D12Device> d3d12_device;
	ComPtr<ID3D12DescriptorHeap> d3d12_heap;

	void create(ComPtr<ID3D12Device> &device, u32 descriptors_number, D3D12_DESCRIPTOR_HEAP_TYPE descriptor_heap_type);
	ID3D12DescriptorHeap *get();
	D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_handle(u32 descriptor_index);
	D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_handle(u32 descriptor_index);
	D3D12_GPU_DESCRIPTOR_HANDLE get_base_gpu_handle();
};

struct CBSRUA_Descriptor_Heap : Descriptor_Heap {
	CBSRUA_Descriptor_Heap();
	~CBSRUA_Descriptor_Heap();

	void create(ComPtr<ID3D12Device> &device, u32 descriptors_number);
	D3D12_GPU_Descriptor place_cb_descriptor(u32 descriptor_index, D3D12_Resource *resource);
	D3D12_GPU_Descriptor place_sr_descriptor(u32 descriptor_index, D3D12_Resource *resource, u32 mipmap_level = 0);
	D3D12_GPU_Descriptor place_ua_descriptor(u32 descriptor_index, D3D12_Resource *resource, u32 mipmap_level = 0);
};

struct RT_Descriptor_Heap : Descriptor_Heap {
	RT_Descriptor_Heap();
	~RT_Descriptor_Heap();

	void create(ComPtr<ID3D12Device> &device, u32 descriptors_number);
	D3D12_CPU_Descriptor place_descriptor(u32 descriptor_index, D3D12_Resource *resource);
};

struct DS_Descriptor_Heap : Descriptor_Heap {
	DS_Descriptor_Heap();
	~DS_Descriptor_Heap();

	void create(ComPtr<ID3D12Device> &device, u32 descriptors_number);
	D3D12_CPU_Descriptor place_descriptor(u32 descriptor_index, D3D12_Resource *resource);
};

struct Sampler_Descriptor_Heap : Descriptor_Heap {
	Sampler_Descriptor_Heap();
	~Sampler_Descriptor_Heap();

	void create(ComPtr<ID3D12Device> &device, u32 descriptors_number);
	D3D12_GPU_Descriptor place_descriptor(u32 descriptor_index, D3D12_Sampler *sampler);
};

struct Descriptor_Heap_Pool {
	Descriptor_Heap_Pool();
	~Descriptor_Heap_Pool();

	Array<u32> rt_descriptor_indices;
	Array<u32> ds_descriptor_indices;
	Array<u32> cbsrua_descriptor_indices;
	Array<u32> sampler_descriptor_indices;

	RT_Descriptor_Heap rt_descriptor_heap;
	DS_Descriptor_Heap ds_descriptor_heap;
	CBSRUA_Descriptor_Heap cbsrua_descriptor_heap;
	Sampler_Descriptor_Heap sampler_descriptor_heap;
	D3D12_GPU_Descriptor base_sampler_descriptor;
	D3D12_GPU_Descriptor base_cbsrua_descriptor;

	D3D12_GPU_Descriptor allocate_cb_descriptor(D3D12_Resource *resource);
	D3D12_GPU_Descriptor allocate_sr_descriptor(D3D12_Resource *resource, u32 mipmap_level = 0);
	D3D12_GPU_Descriptor allocate_ua_descriptor(D3D12_Resource *resource, u32 mipmap_level = 0);
	D3D12_CPU_Descriptor allocate_rt_descriptor(D3D12_Resource *resource);
	D3D12_CPU_Descriptor allocate_ds_descriptor(D3D12_Resource *resource);
	D3D12_GPU_Descriptor allocate_sampler_descriptor(D3D12_Sampler *sampler);

	void allocate_pool(ComPtr<ID3D12Device> &device, u32 descriptors_count);

	void free(GPU_Descriptor *descriptor);
	void free(CPU_Descriptor *descriptor);
};

#endif
