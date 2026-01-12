#include <assert.h>

#include "to_d3d12_types.h"
#include "d3d12_descriptor_heap.h"

#include "../../sys/utils.h"

Descriptor_Heap::Descriptor_Heap()
{
	ZeroMemory(&cpu_handle, sizeof(D3D12_CPU_DESCRIPTOR_HANDLE));
	ZeroMemory(&gpu_handle, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
}

Descriptor_Heap::~Descriptor_Heap()
{
}

void Descriptor_Heap::create(ComPtr<ID3D12Device> &device, u32 descriptors_number, D3D12_DESCRIPTOR_HEAP_TYPE descriptor_heap_type)
{
	d3d12_device = device;
	bool shader_visible = (descriptor_heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) || (descriptor_heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
	ZeroMemory(&heap_desc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	heap_desc.NumDescriptors = descriptors_number;
	heap_desc.Type = descriptor_heap_type;
	heap_desc.Flags = shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HR(d3d12_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(d3d12_heap.ReleaseAndGetAddressOf())));

	descriptor_heap_capacity = descriptors_number;
	increment_size = d3d12_device->GetDescriptorHandleIncrementSize(descriptor_heap_type);
	cpu_handle = d3d12_heap->GetCPUDescriptorHandleForHeapStart();

	if (shader_visible) {
		gpu_handle = d3d12_heap->GetGPUDescriptorHandleForHeapStart();
	}
}

ID3D12DescriptorHeap *Descriptor_Heap::get()
{
	return d3d12_heap.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE Descriptor_Heap::get_cpu_handle(u32 descriptor_index)
{
	assert(0 < increment_size);
	assert(descriptor_index < descriptor_heap_capacity);

	return { cpu_handle.ptr + (increment_size * descriptor_index) };
}

D3D12_GPU_DESCRIPTOR_HANDLE Descriptor_Heap::get_gpu_handle(u32 descriptor_index)
{
	assert(0 < increment_size);
	assert(descriptor_index < descriptor_heap_capacity);

	return { gpu_handle.ptr + (increment_size * descriptor_index) };
}

D3D12_GPU_DESCRIPTOR_HANDLE Descriptor_Heap::get_base_gpu_handle()
{
	return get_gpu_handle(0);
}

CBSRUA_Descriptor_Heap::CBSRUA_Descriptor_Heap() : Descriptor_Heap()
{
}

CBSRUA_Descriptor_Heap::~CBSRUA_Descriptor_Heap()
{
}

void CBSRUA_Descriptor_Heap::create(ComPtr<ID3D12Device> &device, u32 descriptors_number)
{
	Descriptor_Heap::create(device, descriptors_number, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

D3D12_GPU_Descriptor CBSRUA_Descriptor_Heap::place_cb_descriptor(u32 descriptor_index, D3D12_Resource *resource)
{
	assert(resource->size() <= UINT_MAX);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbuffer_view_desc;
	ZeroMemory(&cbuffer_view_desc, sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC));
	cbuffer_view_desc.BufferLocation = resource->gpu_address();
	cbuffer_view_desc.SizeInBytes = static_cast<u32>(resource->size());

	d3d12_device->CreateConstantBufferView(&cbuffer_view_desc, get_cpu_handle(descriptor_index));

	return D3D12_GPU_Descriptor(DESCRIPTOR_TYPE_CBV_SRV_UAV, descriptor_index, get_cpu_handle(descriptor_index), get_gpu_handle(descriptor_index));
}

D3D12_GPU_Descriptor CBSRUA_Descriptor_Heap::place_sr_descriptor(u32 descriptor_index, D3D12_Resource *resource, u32 mipmap_level)
{
	assert(0 < resource->count);
	assert(0 < resource->stride);

	D3D12_RESOURCE_DESC resource_desc = resource->d3d12_resource_desc();

	D3D12_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
	ZeroMemory(&shader_resource_view_desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
	shader_resource_view_desc.Format = to_shader_resource_view_format(resource_desc.Format);
	shader_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	switch (resource_desc.Dimension) {
		case D3D12_RESOURCE_DIMENSION_BUFFER: {
			shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			shader_resource_view_desc.Buffer.FirstElement = 0;
			shader_resource_view_desc.Buffer.NumElements = resource->count;
			shader_resource_view_desc.Buffer.StructureByteStride = resource->stride;
			shader_resource_view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			break;
		}
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D: {
			assert(resource_desc.DepthOrArraySize == 1);
			shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			shader_resource_view_desc.Texture2D.MostDetailedMip = mipmap_level;
			shader_resource_view_desc.Texture2D.MipLevels = resource_desc.MipLevels;
			shader_resource_view_desc.Texture2D.PlaneSlice = 0;
			shader_resource_view_desc.Texture2D.ResourceMinLODClamp = 0.0f;
			break;
		}
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D: {
			shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			shader_resource_view_desc.Texture3D.MostDetailedMip = mipmap_level;
			shader_resource_view_desc.Texture3D.MipLevels = resource_desc.MipLevels;
			shader_resource_view_desc.Texture3D.ResourceMinLODClamp = 0.0f;
			break;
		}
		default: {
			assert(false);
		}
	}
	d3d12_device->CreateShaderResourceView(resource->get(), &shader_resource_view_desc, get_cpu_handle(descriptor_index));
	return D3D12_GPU_Descriptor(DESCRIPTOR_TYPE_CBV_SRV_UAV, descriptor_index, get_cpu_handle(descriptor_index), get_gpu_handle(descriptor_index));
}

D3D12_GPU_Descriptor CBSRUA_Descriptor_Heap::place_ua_descriptor(u32 descriptor_index, D3D12_Resource *resource, u32 mipmap_level)
{
	D3D12_RESOURCE_DESC resource_desc = resource->d3d12_resource_desc();

	D3D12_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc;
	ZeroMemory(&unordered_access_view_desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));
	unordered_access_view_desc.Format = resource_desc.Format;

	switch (resource_desc.Dimension) {
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D: {
			assert(resource_desc.DepthOrArraySize == 1);
			unordered_access_view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			unordered_access_view_desc.Texture2D.MipSlice = mipmap_level;
			break;
		}
		default: {
			assert(false);
		}
	}
	d3d12_device->CreateUnorderedAccessView(resource->get(), NULL, &unordered_access_view_desc, get_cpu_handle(descriptor_index));
	return D3D12_GPU_Descriptor(DESCRIPTOR_TYPE_CBV_SRV_UAV, descriptor_index, get_cpu_handle(descriptor_index), get_gpu_handle(descriptor_index));
}

Sampler_Descriptor_Heap::Sampler_Descriptor_Heap()
{
}

Sampler_Descriptor_Heap::~Sampler_Descriptor_Heap()
{
}

void Sampler_Descriptor_Heap::create(ComPtr<ID3D12Device> &device, u32 descriptors_number)
{
	Descriptor_Heap::create(device, descriptors_number, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

D3D12_GPU_Descriptor Sampler_Descriptor_Heap::place_descriptor(u32 descriptor_index, D3D12_Sampler *sampler)
{
	D3D12_SAMPLER_DESC sampler_desc = sampler->d3d12_sampler_desc();
	d3d12_device->CreateSampler(&sampler_desc, get_cpu_handle(descriptor_index));
	return D3D12_GPU_Descriptor(DESCRIPTOR_TYPE_SAMPLER, descriptor_index, get_cpu_handle(descriptor_index), get_gpu_handle(descriptor_index));
}

RT_Descriptor_Heap::RT_Descriptor_Heap()
{
}

RT_Descriptor_Heap::~RT_Descriptor_Heap()
{
}

void RT_Descriptor_Heap::create(ComPtr<ID3D12Device> &device, u32 descriptors_number)
{
	Descriptor_Heap::create(device, descriptors_number, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

D3D12_CPU_Descriptor RT_Descriptor_Heap::place_descriptor(u32 descriptor_index, D3D12_Resource *resource)
{
	D3D12_RESOURCE_DESC resource_desc = resource->d3d12_resource_desc();

	D3D12_RENDER_TARGET_VIEW_DESC render_target_view_desc;
	ZeroMemory(&render_target_view_desc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));
	render_target_view_desc.Format = resource_desc.Format;

	switch (resource_desc.Dimension) {
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D: {
			assert(resource_desc.DepthOrArraySize == 1);
			render_target_view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			render_target_view_desc.Texture2D.MipSlice = 0;
			render_target_view_desc.Texture2D.PlaneSlice = 0;
			break;
		}
		default: {
			assert(false);
		}
	}
	d3d12_device->CreateRenderTargetView(resource->get(), &render_target_view_desc, get_cpu_handle(descriptor_index));
	return D3D12_CPU_Descriptor(DESCRIPTOR_TYPE_RTV, descriptor_index, get_cpu_handle(descriptor_index));
}

DS_Descriptor_Heap::DS_Descriptor_Heap()
{
}

DS_Descriptor_Heap::~DS_Descriptor_Heap()
{
}

void DS_Descriptor_Heap::create(ComPtr<ID3D12Device> &device, u32 descriptors_number)
{
	Descriptor_Heap::create(device, descriptors_number, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

D3D12_CPU_Descriptor DS_Descriptor_Heap::place_descriptor(u32 descriptor_index, D3D12_Resource *resource)
{
	D3D12_RESOURCE_DESC resource_desc = resource->d3d12_resource_desc();

	D3D12_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc;
	ZeroMemory(&depth_stencil_view_desc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
	depth_stencil_view_desc.Format = to_depth_stencil_view_format(resource_desc.Format);

	switch (resource_desc.Dimension) {
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D: {
			assert(resource_desc.DepthOrArraySize == 1);
			depth_stencil_view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			depth_stencil_view_desc.Texture2D.MipSlice = 0;
			break;
		}
		default: {
			assert(false);
		}
	}
	d3d12_device->CreateDepthStencilView(resource->get(), &depth_stencil_view_desc, get_cpu_handle(descriptor_index));
	return D3D12_CPU_Descriptor(DESCRIPTOR_TYPE_DSV, descriptor_index, get_cpu_handle(descriptor_index));
}

Descriptor_Heap_Pool::Descriptor_Heap_Pool()
{
}

Descriptor_Heap_Pool::~Descriptor_Heap_Pool()
{
}

D3D12_GPU_Descriptor Descriptor_Heap_Pool::allocate_cb_descriptor(D3D12_Resource *resource)
{
	return cbsrua_descriptor_heap.place_cb_descriptor(cbsrua_descriptor_indices.pop(), resource);
}

D3D12_GPU_Descriptor Descriptor_Heap_Pool::allocate_sr_descriptor(D3D12_Resource *resource, u32 mipmap_level)
{
	return cbsrua_descriptor_heap.place_sr_descriptor(cbsrua_descriptor_indices.pop(), resource, mipmap_level);
}

D3D12_GPU_Descriptor Descriptor_Heap_Pool::allocate_ua_descriptor(D3D12_Resource *resource, u32 mipmap_level)
{
	return cbsrua_descriptor_heap.place_ua_descriptor(cbsrua_descriptor_indices.pop(), resource, mipmap_level);
}

D3D12_CPU_Descriptor Descriptor_Heap_Pool::allocate_rt_descriptor(D3D12_Resource *resource)
{
	return rt_descriptor_heap.place_descriptor(rt_descriptor_indices.pop(), resource);
}

D3D12_CPU_Descriptor Descriptor_Heap_Pool::allocate_ds_descriptor(D3D12_Resource *resource)
{
	return ds_descriptor_heap.place_descriptor(rt_descriptor_indices.pop(), resource);
}

D3D12_GPU_Descriptor Descriptor_Heap_Pool::allocate_sampler_descriptor(D3D12_Sampler *sampler)
{
	return sampler_descriptor_heap.place_descriptor(sampler_descriptor_indices.pop(), sampler);
}

void Descriptor_Heap_Pool::allocate_pool(ComPtr<ID3D12Device> &device, u32 descriptors_count)
{
	cbsrua_descriptor_heap.create(device, descriptors_count);
	sampler_descriptor_heap.create(device, descriptors_count);
	rt_descriptor_heap.create(device, descriptors_count);
	ds_descriptor_heap.create(device, descriptors_count);

	base_sampler_descriptor = D3D12_GPU_Descriptor(DESCRIPTOR_TYPE_SAMPLER, 0, sampler_descriptor_heap.cpu_handle, sampler_descriptor_heap.gpu_handle);
	base_cbsrua_descriptor = D3D12_GPU_Descriptor(DESCRIPTOR_TYPE_CBV_SRV_UAV, 0, cbsrua_descriptor_heap.cpu_handle, cbsrua_descriptor_heap.gpu_handle);

	cbsrua_descriptor_indices.resize(descriptors_count);
	sampler_descriptor_indices.resize(descriptors_count);
	rt_descriptor_indices.resize(descriptors_count);
	ds_descriptor_indices.resize(descriptors_count);

	u32 index = descriptors_count;
	for (u32 i = 0; i < descriptors_count; i++) {
		index -= 1;
		cbsrua_descriptor_indices.push(index);
		rt_descriptor_indices.push(index);
		ds_descriptor_indices.push(index);
		sampler_descriptor_indices.push(index);
	}
}

void Descriptor_Heap_Pool::free(GPU_Descriptor *descriptor)
{
	Array<u32> *descriptor_indices[4];
	descriptor_indices[DESCRIPTOR_TYPE_CBV_SRV_UAV] = &cbsrua_descriptor_indices;
	descriptor_indices[DESCRIPTOR_TYPE_SAMPLER] = &sampler_descriptor_indices;
	descriptor_indices[DESCRIPTOR_TYPE_RTV] = &rt_descriptor_indices;
	descriptor_indices[DESCRIPTOR_TYPE_DSV] = &ds_descriptor_indices;

	D3D12_GPU_Descriptor *d3d12_descriptor = static_cast<D3D12_GPU_Descriptor *>(descriptor);

	if (descriptor->valid()) {
		assert(static_cast<u8>(d3d12_descriptor->type) < 4);
		descriptor_indices[d3d12_descriptor->type]->push(d3d12_descriptor->index_in_heap);
	}
}

void Descriptor_Heap_Pool::free(CPU_Descriptor *descriptor)
{
	Array<u32> *descriptor_indices[4];
	descriptor_indices[DESCRIPTOR_TYPE_CBV_SRV_UAV] = &cbsrua_descriptor_indices;
	descriptor_indices[DESCRIPTOR_TYPE_SAMPLER] = &sampler_descriptor_indices;
	descriptor_indices[DESCRIPTOR_TYPE_RTV] = &rt_descriptor_indices;
	descriptor_indices[DESCRIPTOR_TYPE_DSV] = &ds_descriptor_indices;

	D3D12_CPU_Descriptor *d3d12_descriptor = static_cast<D3D12_CPU_Descriptor *>(descriptor);

	if (descriptor->valid()) {
		assert(static_cast<u8>(d3d12_descriptor->type) < 4);
		descriptor_indices[d3d12_descriptor->type]->push(d3d12_descriptor->index_in_heap);
	}
}