#include <assert.h>

#include <d3d12.h>
#include <wrl/client.h>

#include "render_device.h"

#include "../../sys/sys.h"
#include "../../sys/utils.h"

#include "../../libs/number_types.h"
#include "../../libs/structures/queue.h"

using Microsoft::WRL::ComPtr;
struct D3D12_Render_Device;

struct D3D12_CPU_Descriptor: CPU_Descriptor {
	D3D12_CPU_Descriptor(u32 index, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle);
	~D3D12_CPU_Descriptor();

	u32 index_in_heap = UINT_MAX;
	D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;

	bool valid();
	u32 index();
};

D3D12_CPU_Descriptor::~D3D12_CPU_Descriptor()
{
}

D3D12_CPU_Descriptor::D3D12_CPU_Descriptor(u32 index, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle) : index_in_heap(index), cpu_handle(cpu_handle)
{
}

bool D3D12_CPU_Descriptor::valid()
{
	return (index_in_heap != UINT_MAX) && (cpu_handle.ptr != 0);
}

u32 D3D12_CPU_Descriptor::index()
{
	return index_in_heap;
}

struct D3D12_GPU_Descriptor : GPU_Descriptor {
	D3D12_GPU_Descriptor(u32 index, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle);
	~D3D12_GPU_Descriptor();

	u32 index_in_heap = UINT_MAX;
	D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;

	bool valid();
	u32 index();
};

D3D12_GPU_Descriptor::D3D12_GPU_Descriptor(u32 index, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) : index_in_heap(index), cpu_handle(cpu_handle), gpu_handle(gpu_handle)
{
}

D3D12_GPU_Descriptor::~D3D12_GPU_Descriptor()
{
}

bool D3D12_GPU_Descriptor::valid()
{
	return (index_in_heap != UINT_MAX) && (cpu_handle.ptr != 0) && (gpu_handle.ptr != 0);
}

u32 D3D12_GPU_Descriptor::index()
{
	return index_in_heap;
}

enum Resource_Type {
	RESOURCE_TYPE_BUFFER,
	RESOURCE_TYPE_TEXTURE
};

struct Resource_Desc {
	Resource_Desc(Buffer_Desc *buffer_desc);
	Resource_Desc(Texture_Desc *texture_desc);
	~Resource_Desc();

	Resource_Type type;

	union {
		Buffer_Desc buffer_desc;
		Texture_Desc texture_desc;
	};

	String &resource_name();
	D3D12_RESOURCE_DESC to_d3d12_resource_desc();
	D3D12_RESOURCE_STATES to_d312_resource_state();
};

Resource_Desc::Resource_Desc(Buffer_Desc *buffer_desc)
{
}

Resource_Desc::Resource_Desc(Texture_Desc *texture_desc)
{
}

Resource_Desc::~Resource_Desc()
{
}

String &Resource_Desc::resource_name()
{
	// TODO: insert return statement here
}

D3D12_RESOURCE_DESC Resource_Desc::to_d3d12_resource_desc()
{
	return D3D12_RESOURCE_DESC();
}

D3D12_RESOURCE_STATES Resource_Desc::to_d312_resource_state()
{
	return D3D12_RESOURCE_STATES();
}

struct D3D12_Resource {
	D3D12_Resource(ComPtr<ID3D12Device> &device, Resource_Desc &resource_desc);
	D3D12_Resource(ComPtr<ID3D12Device> &device, ComPtr<ID3D12Heap> &heap, u64 heap_offset, Resource_Desc &resource_desc);
	virtual ~D3D12_Resource();

	void *mapped_memory = NULL;
	u32 count = 0;
	u32 stride = 0;
	u64 total_size = 0;
	ComPtr<ID3D12Resource> d3d12_resource;

	void *map();
	void unmap();

	u64 size();
	u64 gpu_address();
	ID3D12Resource *get();
	D3D12_RESOURCE_DESC d3d12_resource_desc();
};

D3D12_Resource::D3D12_Resource(ComPtr<ID3D12Device> &device, Resource_Desc &resource_desc)
{
}

D3D12_Resource::D3D12_Resource(ComPtr<ID3D12Device> &device, ComPtr<ID3D12Heap> &heap, u64 heap_offset, Resource_Desc &resource_desc)
{
}

D3D12_Resource::~D3D12_Resource()
{
	unmap();
}

void *D3D12_Resource::map()
{
	if (mapped_memory == NULL) {
		HR(d3d12_resource->Map(0, NULL, &mapped_memory));
	}
	return mapped_memory;
}

void D3D12_Resource::unmap()
{
	if (mapped_memory != NULL) {
		d3d12_resource->Unmap(0, NULL);
	}
}

u64 D3D12_Resource::size()
{
	return total_size;
}

struct D3D12_Sampler : Sampler {
	D3D12_Sampler();
	~D3D12_Sampler();

	Sampler_Filter filter;
	Address_Mode u;
	Address_Mode v;
	Address_Mode w;

	D3D12_SAMPLER_DESC d3d12_sampler_desc();
};

D3D12_Sampler::D3D12_Sampler()
{
}

D3D12_Sampler::~D3D12_Sampler()
{
}

static D3D12_FILTER to_d3d12_filter(Sampler_Filter filter)
{
	switch (filter) {
		case SAMPLER_FILTER_POINT:
			return D3D12_FILTER_MIN_MAG_MIP_POINT;
		case SAMPLER_FILTER_LINEAR:
			return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		case SAMPLER_FILTER_ANISOTROPIC:
			return D3D12_FILTER_ANISOTROPIC;
	}
	assert(false);
	return (D3D12_FILTER)0;
}

static D3D12_TEXTURE_ADDRESS_MODE to_d3d12_texture_address_mode(Address_Mode address_mode)
{
	switch (address_mode) {
		case ADDRESS_MODE_WRAP:
			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		case ADDRESS_MODE_MIRROR:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		case ADDRESS_MODE_CLAMP:
			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		case ADDRESS_MODE_BORDER:
			return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	}
	assert(false);
	return (D3D12_TEXTURE_ADDRESS_MODE)0;
}

D3D12_SAMPLER_DESC D3D12_Sampler::d3d12_sampler_desc()
{
	D3D12_SAMPLER_DESC d3d12_sampler_desc;
	ZeroMemory(&d3d12_sampler_desc, sizeof(D3D12_SAMPLER_DESC));
	d3d12_sampler_desc.Filter = to_d3d12_filter(filter);
	d3d12_sampler_desc.AddressU = to_d3d12_texture_address_mode(u);
	d3d12_sampler_desc.AddressV = to_d3d12_texture_address_mode(v);
	d3d12_sampler_desc.AddressW = to_d3d12_texture_address_mode(w);
	d3d12_sampler_desc.MipLODBias = 0;
	d3d12_sampler_desc.MaxAnisotropy = filter == SAMPLER_FILTER_ANISOTROPIC ? 16 : 0;
	d3d12_sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3d12_sampler_desc.BorderColor[0] = 0;
	d3d12_sampler_desc.BorderColor[1] = 0;
	d3d12_sampler_desc.BorderColor[2] = 0;
	d3d12_sampler_desc.BorderColor[3] = 0;
	d3d12_sampler_desc.MinLOD = 0;
	d3d12_sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
	return d3d12_sampler_desc;
}

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
	D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_handle(u32 descriptor_index);
	D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_handle(u32 descriptor_index);
	D3D12_GPU_DESCRIPTOR_HANDLE get_base_gpu_handle();
};

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
	bool shader_visible = (descriptor_heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) || (descriptor_heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
	ZeroMemory(&heap_desc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	heap_desc.NumDescriptors = descriptors_number;
	heap_desc.Type = descriptor_heap_type;
	heap_desc.Flags = shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HR(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(d3d12_heap.ReleaseAndGetAddressOf())));

	descriptor_heap_capacity = descriptors_number;
	increment_size = device->GetDescriptorHandleIncrementSize(descriptor_heap_type);
	cpu_handle = d3d12_heap->GetCPUDescriptorHandleForHeapStart();

	if (shader_visible) {
		gpu_handle = d3d12_heap->GetGPUDescriptorHandleForHeapStart();
	}
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

struct CBSRUA_Descriptor_Heap : Descriptor_Heap {
	CBSRUA_Descriptor_Heap();
	~CBSRUA_Descriptor_Heap();

	void create(ComPtr<ID3D12Device> &device, u32 descriptors_number);
	D3D12_GPU_Descriptor place_cb_descriptor(u32 descriptor_index, D3D12_Resource *resource);
	D3D12_GPU_Descriptor place_sr_descriptor(u32 descriptor_index, D3D12_Resource *resource, u32 mipmap_level = 0);
	D3D12_GPU_Descriptor place_ua_descriptor(u32 descriptor_index, D3D12_Resource *resource, u32 mipmap_level = 0);
};

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

	return D3D12_GPU_Descriptor(descriptor_index, get_cpu_handle(descriptor_index), get_gpu_handle(descriptor_index));
}

inline DXGI_FORMAT to_shader_resource_view_format(DXGI_FORMAT format)
{
	switch (format) {
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_TYPELESS:
			return DXGI_FORMAT_R32_FLOAT;
	}
	return format;
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
	return D3D12_GPU_Descriptor(descriptor_index, get_cpu_handle(descriptor_index), get_gpu_handle(descriptor_index));
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
	return D3D12_GPU_Descriptor(descriptor_index, get_cpu_handle(descriptor_index), get_gpu_handle(descriptor_index));
}

struct Sampler_Descriptor_Heap : Descriptor_Heap {
	Sampler_Descriptor_Heap();
	~Sampler_Descriptor_Heap();

	void create(ComPtr<ID3D12Device> &device, u32 descriptors_number);
	D3D12_GPU_Descriptor place_descriptor(u32 descriptor_index, D3D12_Sampler *sampler);
};

void Sampler_Descriptor_Heap::create(ComPtr<ID3D12Device> &device, u32 descriptors_number)
{
	Descriptor_Heap::create(device, descriptors_number, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

D3D12_GPU_Descriptor Sampler_Descriptor_Heap::place_descriptor(u32 descriptor_index, D3D12_Sampler *sampler)
{
	D3D12_SAMPLER_DESC sampler_desc = sampler->d3d12_sampler_desc();
	d3d12_device->CreateSampler(&sampler_desc, get_cpu_handle(descriptor_index));
	return D3D12_GPU_Descriptor(descriptor_index, get_cpu_handle(descriptor_index), get_gpu_handle(descriptor_index));
}

struct RT_Descriptor_Heap : Descriptor_Heap {
	RT_Descriptor_Heap();
	~RT_Descriptor_Heap();

	void create(ComPtr<ID3D12Device> &device, u32 descriptors_number);
	D3D12_CPU_Descriptor place_descriptor(u32 descriptor_index, D3D12_Resource *resource);
};

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
	return D3D12_CPU_Descriptor(descriptor_index, get_cpu_handle(descriptor_index));
}

struct DS_Descriptor_Heap : Descriptor_Heap {
	DS_Descriptor_Heap();
	~DS_Descriptor_Heap();

	void create(ComPtr<ID3D12Device> &device, u32 descriptors_number);
	D3D12_CPU_Descriptor place_descriptor(u32 descriptor_index, D3D12_Resource *resource);
};

void RT_Descriptor_Heap::create(ComPtr<ID3D12Device> &device, u32 descriptors_number)
{
	Descriptor_Heap::create(device, descriptors_number, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

inline DXGI_FORMAT to_depth_stencil_view_format(DXGI_FORMAT format)
{
	switch (format) {
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_TYPELESS:
			return DXGI_FORMAT_D32_FLOAT;
	}
	return format;
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
	return D3D12_CPU_Descriptor(descriptor_index, get_cpu_handle(descriptor_index));
}

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

	D3D12_GPU_Descriptor allocate_cb_descriptor(D3D12_Resource *resource);
	D3D12_GPU_Descriptor allocate_sr_descriptor(D3D12_Resource *resource, u32 mipmap_level = 0);
	D3D12_GPU_Descriptor allocate_ua_descriptor(D3D12_Resource *resource, u32 mipmap_level = 0);
	D3D12_CPU_Descriptor allocate_rt_descriptor(D3D12_Resource *resource);
	D3D12_CPU_Descriptor allocate_ds_descriptor(D3D12_Resource *resource);
	D3D12_GPU_Descriptor allocate_sampler_descriptor(D3D12_Sampler *sampler);

	void allocate_pool(ComPtr<ID3D12Device> &device, u32 descriptors_count);

	void free(CB_Descriptor *descriptor);
	void free(SR_Descriptor *descriptor);
	void free(UA_Descriptor *descriptor);
	void free(RT_Descriptor *descriptor);
	void free(DS_Descriptor *descriptor);
	void free(Sampler_Descriptor *descriptor);
};

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
	rt_descriptor_heap.create(device, descriptors_count);
	ds_descriptor_heap.create(device, descriptors_count);
	sampler_descriptor_heap.create(device, descriptors_count);

	rt_descriptor_indices.resize(descriptors_count);
	ds_descriptor_indices.resize(descriptors_count);
	cbsrua_descriptor_indices.resize(descriptors_count);
	sampler_descriptor_indices.resize(descriptors_count);

	u32 index = descriptors_count;
	for (u32 i = 0; i < descriptors_count; i++) {
		index -= 1;
		rt_descriptor_indices.push(index);
		ds_descriptor_indices.push(index);
		cbsrua_descriptor_indices.push(index);
		sampler_descriptor_indices.push(index);
	}
}

void Descriptor_Heap_Pool::free(CB_Descriptor *descriptor)
{
	if (descriptor->valid()) {
		cbsrua_descriptor_indices.push(descriptor->index());
	}
}

void Descriptor_Heap_Pool::free(SR_Descriptor *descriptor)
{
	if (descriptor->valid()) {
		cbsrua_descriptor_indices.push(descriptor->index());
	}
}

void Descriptor_Heap_Pool::free(UA_Descriptor *descriptor)
{
	if (descriptor->valid()) {
		cbsrua_descriptor_indices.push(descriptor->index());
	}
}

void Descriptor_Heap_Pool::free(Sampler_Descriptor *descriptor)
{
	if (descriptor->valid()) {
		sampler_descriptor_indices.push(descriptor->index());
	}
}

void Descriptor_Heap_Pool::free(RT_Descriptor *descriptor)
{
	if (descriptor->valid()) {
		rt_descriptor_indices.push(descriptor->index());
	}
}

void Descriptor_Heap_Pool::free(DS_Descriptor *descriptor)
{
	if (descriptor->valid()) {
		ds_descriptor_indices.push(descriptor->index());
	}
}


template <typename... Args>
void set_name(ID3D12Object *objec, Args... args)
{
	char *formatted_string = format(args);
	wchar_t *wstring = to_wstring(name);

	object->SetName(wstring);

	free_string(formatted_string);
	free_string(wstring);
}

static D3D12_HEAP_TYPE to_d3d12_heap_type(Resource_Usage type)
{
	switch (type) {
		case RESOURCE_USAGE_DEFAULT:
			return D3D12_HEAP_TYPE_DEFAULT;
		case RESOURCE_USAGE_UPLOAD:
			return D3D12_HEAP_TYPE_UPLOAD;
		case RESOURCE_USAGE_READBACK:
			return D3D12_HEAP_TYPE_READBACK;
	}
	assert(false);
	return (D3D12_HEAP_TYPE)0;
}

inline D3D12_RESOURCE_DIMENSION to_d3d12_resource_dimension(Texture_Dimension texture_dimension)
{
	switch (texture_dimension) {
		case TEXTURE_DIMENSION_UNKNOWN:
			return D3D12_RESOURCE_DIMENSION_UNKNOWN;
		case TEXTURE_DIMENSION_1D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		case TEXTURE_DIMENSION_2D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		case TEXTURE_DIMENSION_3D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	}
	assert(false);
	return static_cast<D3D12_RESOURCE_DIMENSION>(0);
}

D3D12_RESOURCE_STATES to_d3d12_resource_state(const Resource_State &resource_state)
{
	switch (resource_state) {
		case RESOURCE_STATE_COMMON:
			return D3D12_RESOURCE_STATE_COMMON;
		case RESOURCE_STATE_GENERIC_READ:
			return D3D12_RESOURCE_STATE_GENERIC_READ;
		case RESOURCE_STATE_COPY_DEST:
			return D3D12_RESOURCE_STATE_COPY_DEST;
		case RESOURCE_STATE_COPY_SOURCE:
			return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case RESOURCE_STATE_RENDER_TARGET:
			return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case RESOURCE_STATE_PRESENT:
			return D3D12_RESOURCE_STATE_PRESENT;
		case RESOURCE_STATE_DEPTH_WRITE:
			return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
			return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		case RESOURCE_STATE_ALL_SHADER_RESOURCE:
			return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}
	assert(false);
	return (D3D12_RESOURCE_STATES)0;
}


struct Working_Buffer {
	u64 frame_number = 0;
	ComPtr<ID3D12Resource> buffer;
};

struct D3D12_Base_Buffer : D3D12_Resource {
	D3D12_Base_Buffer(ComPtr<ID3D12Device> &device, Resource_Desc &resource_desc);
	D3D12_Base_Buffer(ComPtr<ID3D12Device> &device, ComPtr<ID3D12Heap> &heap, u64 heap_offset, Resource_Desc &resource_desc);
	~D3D12_Base_Buffer();

	D3D12_GPU_Descriptor constant_buffer_descriptor;
	D3D12_GPU_Descriptor shader_resource_descriptor;
	D3D12_GPU_Descriptor unordered_access_descriptor;
};

struct D3D12_Buffer : Buffer {
	D3D12_Buffer(D3D12_Render_Device *render_device);
	D3D12_Buffer(D3D12_Render_Device *render_device, ComPtr<ID3D12Resource> &default_buffer);
	~D3D12_Buffer();

	Buffer_Desc buffer_desc;
	ComPtr<ID3D12Resource> default_buffer;
	Queue<Working_Buffer> working_buffers;
	Queue<ComPtr<ID3D12Resource>> completed_buffers;

	void finish_frame(u64 frame_number);
	void write(void *data, u64 data_size, u64 alignment = 0);

	Buffer_Desc get_buffer_desc();
	ComPtr<ID3D12Resource> &current_buffer();
	ComPtr<ID3D12Resource> &current_upload_buffer();
};

D3D12_Buffer::D3D12_Buffer(D3D12_Render_Device *render_device, ComPtr<ID3D12Resource> &default_buffer) : render_device(render_device), default_buffer(default_buffer)
{
}

D3D12_Buffer::~D3D12_Buffer()
{
}

void D3D12_Buffer::finish_frame(u64 frame_number)
{
	while (!working_buffers.empty() && (working_buffers.front().frame_number <= frame_number)) {
		ComPtr<ID3D12Resource> upload_buffer = working_buffers.front().buffer;
		set_name(upload_buffer.Get(), "[Upload Buffer] name: {}, frame_number: {} completed", buffer_desc.name, render_device->frame_number);
		completed_buffers.push(upload_buffer);
		working_buffers.pop();
	}
}

void D3D12_Buffer::write(void *data, u64 data_size, u64 alignment)
{
	assert(data);
	assert(data_size > 0);

	Resource_Usage usage;
	if (usage == RESOURCE_USAGE_DEFAULT) {
		Resource_Desc resource_desc = Resource_Desc(&buffer_desc);
		
		ComPtr<ID3D12Resource> upload_buffer;
		render_device->create_resource(&resource_desc, upload_buffer);

		void *ptr = NULL;
		HR(upload_buffer->Map(0, NULL, &ptr));
		memcpy(ptr, data, data_size);
		upload_buffer->Unmap(0, NULL);

		//D3D12_Copy_Command_List *copy_command_list = render_device->get_upload_list();
		//copy_command_list->copy(default_buffer, upload_buffer);

		render_device->safe_release(upload_buffer);
	} else if (usage == RESOURCE_USAGE_UPLOAD) {
		ComPtr<ID3D12Resource> upload_buffer;
		if (!completed_buffers.empty()) {
			upload_buffer = completed_buffers.front();
			set_name(upload_buffer.Get(), "[Upload Buffer] name: {}, frame_number: {}", buffer_desc.name, render_device->frame_number);
			completed_buffers.pop();

			working_buffers.push({ render_device->frame_number, upload_buffer });
		} else {
			Resource_Desc resource_desc = Resource_Desc(&buffer_desc);

			render_device->create_resource(&resource_desc, upload_buffer);
			set_name(upload_buffer.Get(), "[Upload Buffer] name: {}, frame_number: {}", buffer_desc.name, render_device->frame_number);
			
			working_buffers.push({ render_device->frame_number, upload_buffer });
		}
		void *ptr = NULL;
		HR(upload_buffer->Map(0, NULL, &ptr));
		memcpy(ptr, data, data_size);
		upload_buffer->Unmap(0, NULL);

		//D3D12_Copy_Command_List *copy_command_list = render_device->get_upload_list();
		//copy_command_list->copy(default_buffer, upload_buffer);
	}
}

struct D3D12_Texture : Texture {
	D3D12_Texture(D3D12_Render_Device *render_device, ComPtr<ID3D12Resource> &texture);
	~D3D12_Texture();

	D3D12_Render_Device *render_device = NULL;
	ComPtr<ID3D12Resource> resource;

	D3D12_CPU_Descriptor depth_stencil_descriptor;
	D3D12_CPU_Descriptor render_target_descriptor;
	Array<D3D12_GPU_Descriptor> shader_resource_descriptors;
	Array<D3D12_GPU_Descriptor> unordered_access_descriptors;

	Texture_Desc texture_desc();
	
	SR_Descriptor *shader_resource_descriptor(u32 mipmap_level = 0);
	UA_Descriptor *unordered_access_descriptor(u32 mipmap_level = 0);
	DS_Descriptor *depth_stencil_descriptor();
	RT_Descriptor *render_target_descriptor();
};

struct D3D12_Pipeline_State : Pipeline_State {
	D3D12_Pipeline_State();
	~D3D12_Pipeline_State();

	ComPtr<ID3D12PipelineState> pipeline;
};

struct D3D12_Root_Signature;

struct D3D12_Command_List : Graphics_Command_List {
	D3D12_Command_List(Render_Device *render_device, ComPtr<ID3D12CommandAllocator> &command_allocator, D3D12_COMMAND_LIST_TYPE type, ComPtr<ID3D12GraphicsCommandList> &command_list);
	~D3D12_Command_List();

	Render_Device *render_device = NULL;
	D3D12_Root_Signature *bind_root_signature = NULL;

	ComPtr<ID3D12CommandAllocator> command_allocator;
	ComPtr<ID3D12GraphicsCommandList> command_list;

	void reset();
	void close();

	// Copy command list methods
	void copy(Buffer *dest, Buffer *source);
	void copy_buffer_to_texture(Texture *texture, Buffer *buffer, Subresource_Footprint *subresource_footprint);

	// Compute command list methods
	void set_pipeline_state(Pipeline_State *pipeline_state);
	void set_compute_root_signature(Root_Signature *root_signature);

	void set_compute_constatns(u32 shader_register, u32 shader_space, Shader_Register register_type);
	void set_compute_descriptor_table(u32 shader_register, u32 shader_space, Shader_Register register_type, GPU_Descriptor *base_descriptor);
	
	void dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z = 1);

	// Graphics command list methods
	void set_graphics_root_signature(Root_Signature *root_signature);
	void set_primitive_type(Primitive_Type primitive_type);
	void set_viewport(Viewport viewport);
	void set_clip_rect(Rect_u32 clip_rect);
	
	void clear_render_target_view(RT_Descriptor *descriptor, const Color &color);
	void clear_depth_stencil_view(DS_Descriptor *descriptor, float depth = 1.0f, u8 stencil = 0);
	
	void set_vertex_buffer(Buffer *buffer);
	void set_index_buffer(Buffer *buffer);
	
	void set_graphics_constatns(u32 shader_register, u32 shader_space, Shader_Register register_type);
	void set_graphics_descriptor_table(u32 shader_register, u32 shader_space, Shader_Register register_type, GPU_Descriptor *base_descriptor);
	
	void set_render_target(RT_Descriptor *render_target_descriptor, DS_Descriptor *depth_stencil_descriptor);
	
	void draw(u32 vertex_count);
	void draw_indexed(u32 index_count);

	Command_List_Type command_list_type();
};


D3D12_Command_List::D3D12_Command_List(Render_Device *render_device, ComPtr<ID3D12CommandAllocator> &command_allocator, D3D12_COMMAND_LIST_TYPE type, ComPtr<ID3D12GraphicsCommandList> &command_list)
{
}

D3D12_Command_List::~D3D12_Command_List()
{
}

void D3D12_Command_List::reset()
{

}

void D3D12_Command_List::close()
{

}

void D3D12_Command_List::copy(Buffer *dest, Buffer *source)
{
	D3D12_Buffer *_dest = (D3D12_Buffer *)dest;
	D3D12_Buffer *_source = (D3D12_Buffer *)source;

	command_list->CopyResource(_dest->current_buffer().Get(), _source->current_buffer().Get());
}

void D3D12_Command_List::copy_buffer_to_texture(Texture *texture, Buffer *buffer, Subresource_Footprint *subresource_footprint)
{
	D3D12_Buffer *internal_buffer = (D3D12_Buffer *)buffer;
	D3D12_Texture *internal_texture = (D3D12_Texture *)texture;

	D3D12_TEXTURE_COPY_LOCATION dest_texture_copy_location;
	ZeroMemory(&dest_texture_copy_location, sizeof(D3D12_TEXTURE_COPY_LOCATION));
	dest_texture_copy_location.pResource = internal_buffer->current_buffer().Get();
	dest_texture_copy_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dest_texture_copy_location.SubresourceIndex = subresource_footprint.subresource_index;

	D3D12_TEXTURE_COPY_LOCATION source_texture_copy_location;
	ZeroMemory(&source_texture_copy_location, sizeof(D3D12_TEXTURE_COPY_LOCATION));
	source_texture_copy_location.pResource = internal_texture->resource.Get();
	source_texture_copy_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	source_texture_copy_location.PlacedFootprint.Footprint = subresource_footprint.d3d12_subresource_footprint();

	command_list->CopyTextureRegion(&dest_texture_copy_location, 0, 0, 0, &source_texture_copy_location, NULL);
}

void D3D12_Command_List::set_pipeline_state(Pipeline_State *pipeline_state)
{
	D3D12_Pipeline_State *internal_pipeline_state = (D3D12_Pipeline_State *)pipeline_state;
	command_list->SetPipelineState(internal_pipeline_state->pipeline.Get());
}

void D3D12_Command_List::set_compute_root_signature(Root_Signature *root_signature)
{
	D3D12_Root_Signature *internal_root_signature = (D3D12_Root_Signature *)root_signature;
	command_list->SetComputeRootSignature(internal_root_signature->d3d12_root_signature.Get());
}

void D3D12_Command_List::set_compute_constatns(u32 shader_register, u32 shader_space, Shader_Register register_type)
{
}

void D3D12_Command_List::set_compute_descriptor_table(u32 shader_register, u32 shader_space, Shader_Register register_type, GPU_Descriptor *base_descriptor)
{
	u32 parameter_index = bind_root_signature->get_parameter_index(shader_register, shader_space, register_type);
	command_list->SetComputeRootDescriptorTable(parameter_index, base_descriptor.gpu_handle);
}

void  D3D12_Command_List::dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z = 1)
{}
void  D3D12_Command_List::set_graphics_root_signature(Root_Signature *root_signature)
{}
void  D3D12_Command_List::set_primitive_type(Primitive_Type primitive_type)
{}
void  D3D12_Command_List::set_viewport(Viewport viewport)
{}
void  D3D12_Command_List::set_clip_rect(Rect_u32 clip_rect)
{}
void  D3D12_Command_List::clear_render_target_view(RT_Descriptor *descriptor, const Color &color)
{}
void  D3D12_Command_List::clear_depth_stencil_view(DS_Descriptor *descriptor, float depth = 1.0f, u8 stencil = 0)
{}
void  D3D12_Command_List::set_vertex_buffer(Buffer *buffer)
{}
void  D3D12_Command_List::set_index_buffer(Buffer *buffer)
{}
void  D3D12_Command_List::set_graphics_constatns(u32 shader_register, u32 shader_space, Shader_Register register_type)
{}
void  D3D12_Command_List::set_graphics_descriptor_table(u32 shader_register, u32 shader_space, Shader_Register register_type, GPU_Descriptor *base_descriptor)
{}
void  D3D12_Command_List::set_render_target(RT_Descriptor *render_target_descriptor, DS_Descriptor *depth_stencil_descriptor)
{}
void  D3D12_Command_List::draw_indexed(u32 index_count)
{
}
Command_List_Type D3D12_Command_List::command_list_type()
{
}

struct D3D12_Command_Queue : Command_Queue {
	D3D12_Command_Queue();
	~D3D12_Command_Queue();

	void close_and_execute_command_list(Command_List *command_list);
};

static const char *str_shader_register_types[] = {
	"Constant Buffer",
	"Shader Resource",
	"Unordered Access",
	"Sampler"
};

const u32 HLSL_REGISTRE_COUNT = 20;
const u32 HLSL_SPACE_COUNT = 20;

const u32 SHADER_REGISTER_TYPES_NUMBER = 4;

struct Root_Parameter {
	~Root_Parameter();
	Root_Parameter();

	u32 indices[SHADER_REGISTER_TYPES_NUMBER];

	void set_parameter_index(u32 parameter_index, Shader_Register register_type);
	u32 get_parameter_index(Shader_Register register_type);
};

Root_Parameter::Root_Parameter()
{
	memset((void *)indices, 0xff, sizeof(u32) * SHADER_REGISTER_TYPES_NUMBER);
}

Root_Parameter::~Root_Parameter()
{
}

void Root_Parameter::set_parameter_index(u32 parameter_index, Shader_Register register_type)
{
	assert(static_cast<u32>(register_type) < SHADER_REGISTER_TYPES_NUMBER);
	indices[static_cast<u32>(register_type)] = parameter_index;
}

u32 Root_Parameter::get_parameter_index(Shader_Register register_type)
{
	assert(static_cast<u32>(register_type) < SHADER_REGISTER_TYPES_NUMBER);
	return indices[static_cast<u32>(register_type)];
}

struct Temp_Root_Parameter {
	Temp_Root_Parameter(D3D12_ROOT_PARAMETER_TYPE type);
	~Temp_Root_Parameter();

	Temp_Root_Parameter(const Temp_Root_Parameter &t);
	Temp_Root_Parameter &operator=(const Temp_Root_Parameter &t);

	Array<D3D12_DESCRIPTOR_RANGE1> ranges;
	D3D12_ROOT_PARAMETER1 parameter;

	void add_constants(u32 shader_register, u32 register_space, u32 structure_size);

	void add_srv_descriptor_range(u32 shader_register, u32 register_space, u32 descriptors_number);
	void add_descriptor_range(D3D12_DESCRIPTOR_RANGE_TYPE range_type, u32 shader_register, u32 register_space, u32 descriptors_number);
};

struct D3D12_Root_Signature : Root_Signature {
	D3D12_Root_Signature(D3D12_Render_Device *render_device);
	~D3D12_Root_Signature();

	D3D12_Render_Device *render_device = NULL;

	Array<Temp_Root_Parameter> parameters;
	Root_Parameter parameters_table[HLSL_REGISTRE_COUNT][HLSL_SPACE_COUNT];
	ComPtr<ID3D12RootSignature> d3d12_root_signature;

	void store_parameter_index(u32 parameter_index, u32 shader_register, u32 shader_space, Shader_Register register_type);
	u32 get_parameter_index(u32 shader_register, u32 shader_space, Shader_Register register_type);

	void compile(u32 access_flags = 0);
	void add_shader_resource_parameter(u32 shader_register, u32 register_space, u32 descriptors_number);
};

D3D12_Root_Signature::D3D12_Root_Signature(D3D12_Render_Device *render_device) : render_device(render_device)
{
}

D3D12_Root_Signature::~D3D12_Root_Signature()
{
}

void D3D12_Root_Signature::store_parameter_index(u32 parameter_index, u32 shader_register, u32 shader_space, Shader_Register register_type)
{
	assert(shader_register <= HLSL_REGISTRE_COUNT);
	assert(shader_space <= HLSL_SPACE_COUNT);

	u32 temp = parameters_table[shader_register][shader_space].get_parameter_index(register_type);
	if (temp != UINT_MAX) {
		error("A parameter index has already been set for the shader register(register = {}, space = {}, type = {}).", shader_register, shader_space, str_shader_register_types[static_cast<u32>(register_type)]);
	} else {
		parameters_table[shader_register][shader_space].set_parameter_index(parameter_index, register_type);
	}
}

u32 D3D12_Root_Signature::get_parameter_index(u32 shader_register, u32 shader_space, Shader_Register register_type)
{
	assert(shader_register <= HLSL_REGISTRE_COUNT);
	assert(shader_space <= HLSL_SPACE_COUNT);

	u32 parameter_index = parameters_table[shader_register][shader_space].get_parameter_index(register_type);
	if (parameter_index == UINT_MAX) {
		error("A parameter index has not been set yet for the shader register(register = {}, space = {}, type = {}).", shader_register, shader_space, str_shader_register_types[static_cast<u32>(register_type)]);
	}
	return parameter_index;
}

//static D3D12_DESCRIPTOR_RANGE1 *make_descriptor_range(D3D12_DESCRIPTOR_RANGE_TYPE range_type, u32 shader_register, u32 register_space, u32 descriptors_number)
//{
//	D3D12_DESCRIPTOR_RANGE1 *descriptor_range = new D3D12_DESCRIPTOR_RANGE1();
//	ZeroMemory(descriptor_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
//	descriptor_range->RangeType = range_type;
//	descriptor_range->NumDescriptors = descriptors_number;
//	descriptor_range->BaseShaderRegister = shader_register;
//	descriptor_range->RegisterSpace = register_space;
//	descriptor_range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
//	descriptor_range->Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
//	return descriptor_range;
//}
//
//static D3D12_ROOT_PARAMETER1 make_descriptor_table_root_parameter(u32 descriptor_ranges_number, D3D12_DESCRIPTOR_RANGE1 *descriptor_range)
//{
//	D3D12_ROOT_PARAMETER1 parameter;
//	ZeroMemory(&parameter, sizeof(D3D12_ROOT_PARAMETER1));
//	parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
//	parameter.DescriptorTable.NumDescriptorRanges = descriptor_ranges_number;
//	parameter.DescriptorTable.pDescriptorRanges = descriptor_range;
//	parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
//	return parameter;
//}

const u32 ALLOW_INPUT_LAYOUT_ACCESS = 0x1;
const u32 ALLOW_VERTEX_SHADER_ACCESS = 0x2;
const u32 ALLOW_PIXEL_SHADER_ACCESS = 0x4;
const u32 ALLOW_HULL_SHADER_ACCESS = 0x8;
const u32 ALLOW_DOMAIN_SHADER_ACCESS = 0x10;
const u32 ALLOW_GEOMETRY_SHADER_ACCESS = 0x20;
const u32 ALLOW_AMPLIFICATION_SHADER_ACCESS = 0x40;
const u32 ALLOW_MESH_SHADER_ACCESS = 0x80;

void D3D12_Root_Signature::compile(u32 access_flags)
{
	Array<D3D12_ROOT_PARAMETER1> d3d12_root_paramaters;
	for (u32 i = 0; i < parameters.count; i++) {
		d3d12_root_paramaters.push(parameters[i].parameter);
	}

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
	ZeroMemory(&root_signature_desc, sizeof(D3D12_VERSIONED_ROOT_SIGNATURE_DESC));
	root_signature_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	root_signature_desc.Desc_1_1.NumParameters = d3d12_root_paramaters.count;
	root_signature_desc.Desc_1_1.pParameters = d3d12_root_paramaters.items;

	if (access_flags & ALLOW_INPUT_LAYOUT_ACCESS) {
		root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}
	if (!(access_flags & ALLOW_VERTEX_SHADER_ACCESS)) {
		root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
	}
	if (!(access_flags & ALLOW_PIXEL_SHADER_ACCESS)) {
		root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
	}
	if (!(access_flags & ALLOW_HULL_SHADER_ACCESS)) {
		root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
	}
	if (!(access_flags & ALLOW_DOMAIN_SHADER_ACCESS)) {
		root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	}
	if (!(access_flags & ALLOW_GEOMETRY_SHADER_ACCESS)) {
		root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	}
	if (!(access_flags & ALLOW_AMPLIFICATION_SHADER_ACCESS)) {
		root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
	}
	if (!(access_flags & ALLOW_MESH_SHADER_ACCESS)) {
		root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
	}

	ComPtr<ID3DBlob> signature_blob;
	ComPtr<ID3DBlob> errors;
	D3D12SerializeVersionedRootSignature(&root_signature_desc, &signature_blob, &errors);
	HR(render_device->device->CreateRootSignature(0, signature_blob->GetBufferPointer(), signature_blob->GetBufferSize(), IID_PPV_ARGS(d3d12_root_signature.ReleaseAndGetAddressOf())));
}

void D3D12_Root_Signature::add_shader_resource_parameter(u32 shader_register, u32 register_space, u32 descriptors_number)
{
	//D3D12_DESCRIPTOR_RANGE1 *descriptor_range = make_descriptor_range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, shader_register, register_space, descriptors_number);
	//ranges.push(descriptor_range);

	//D3D12_ROOT_PARAMETER1 root_parameter = make_descriptor_table_root_parameter(1, descriptor_range);
	Temp_Root_Parameter root_parameter = Temp_Root_Parameter(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);
	root_parameter.add_srv_descriptor_range(shader_register, register_space, descriptors_number);

	u32 parameter_index = parameters.push(root_parameter);
	store_parameter_index(parameter_index, shader_register, register_space, SHADER_RESOURCE_REGISTER);
}

Temp_Root_Parameter::Temp_Root_Parameter(D3D12_ROOT_PARAMETER_TYPE type)
{
	ZeroMemory(&parameter, sizeof(D3D12_ROOT_PARAMETER1));
	parameter.ParameterType = type;
	parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
}

Temp_Root_Parameter::~Temp_Root_Parameter()
{
}

Temp_Root_Parameter::Temp_Root_Parameter(const Temp_Root_Parameter &other)
{
	*this = other;
}

Temp_Root_Parameter &Temp_Root_Parameter::operator=(const Temp_Root_Parameter &other)
{
	if (this != &other) {
		ranges = other.ranges;
		parameter = other.parameter;
		if (other.parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
			parameter.DescriptorTable.NumDescriptorRanges = ranges.count;
			parameter.DescriptorTable.pDescriptorRanges = ranges.items;
		}
	}
	return *this;
}

void Temp_Root_Parameter::add_constants(u32 shader_register, u32 register_space, u32 structure_size)
{
	assert((structure_size % 4) == 0);
	assert(parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS);

	parameter.Constants.ShaderRegister = shader_register;
	parameter.Constants.RegisterSpace = register_space;
	parameter.Constants.Num32BitValues = structure_size / 4;
}

void Temp_Root_Parameter::add_srv_descriptor_range(u32 shader_register, u32 register_space, u32 descriptors_number)
{
	add_descriptor_range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, shader_register, register_space, descriptors_number);
}

void Temp_Root_Parameter::add_descriptor_range(D3D12_DESCRIPTOR_RANGE_TYPE range_type, u32 shader_register, u32 register_space, u32 descriptors_number)
{
	assert(parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);

	D3D12_DESCRIPTOR_RANGE1 descriptor_range;
	ZeroMemory(&descriptor_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
	descriptor_range.RangeType = range_type;
	descriptor_range.NumDescriptors = descriptors_number;
	descriptor_range.BaseShaderRegister = shader_register;
	descriptor_range.RegisterSpace = register_space;
	descriptor_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	descriptor_range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

	ranges.push(descriptor_range);

	parameter.DescriptorTable.NumDescriptorRanges = ranges.count;
	parameter.DescriptorTable.pDescriptorRanges = ranges.items;
}

struct D3D12_Render_Device : Render_Device {
	D3D12_Render_Device();
	~D3D12_Render_Device();

	u64 frame_number = 0;

	Resource_Allocator *resource_allocator = NULL;
	ComPtr<ID3D12Device> device;

	Queue<Working_Buffer> resource_release_queue;

	Buffer *create_buffer(Buffer_Desc *buffer_desc);
	Texture *create_texture(Texture_Desc *texture_desc);

	Command_List *create_command_list(D3D12_COMMAND_LIST_TYPE type);

	Copy_Command_List *create_copy_command_list();
	Compute_Command_List *create_compute_command_list();
	Graphics_Command_List *create_graphics_command_list();

	void finish_frame(u64 completed_frame);

	//internal
	void safe_release(ComPtr<ID3D12Resource> &resource);
	void create_resource(Resource_Desc *resource_desc, ComPtr<ID3D12Resource> &resource);
};

void D3D12_Render_Device::finish_frame(u64 completed_frame)
{
	while (!resource_release_queue.empty() && (resource_release_queue.front().frame_number <= completed_frame)) {
		ComPtr<ID3D12Resource> buffer = resource_release_queue.front().buffer;
		//set_name(buffer.Get(), "Upload buffer [name {}] [frame {} completed]", debug_name, _frame_number)
		set_name(buffer.Get(), "Upload buffer", frame_number);
		resource_release_queue.pop();
	}
	frame_number++;
}

D3D12_Render_Device::D3D12_Render_Device()
{
}

D3D12_Render_Device::~D3D12_Render_Device()
{
}

Buffer *D3D12_Render_Device::create_buffer(Buffer_Desc *buffer_desc)
{
	assert(buffer_desc->count > 0);
	assert(buffer_desc->stride > 0);

	D3D12_RESOURCE_DESC resource_desc;
	ZeroMemory(&resource_desc, sizeof(D3D12_RESOURCE_DESC));
	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resource_desc.Alignment = 0;
	resource_desc.Width = buffer_desc->size();
	resource_desc.Height = 1;
	resource_desc.DepthOrArraySize = 1;
	resource_desc.MipLevels = 1;
	resource_desc.Format = DXGI_FORMAT_UNKNOWN;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.SampleDesc.Quality = 0;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heap_properties;
	ZeroMemory(&heap_properties, sizeof(D3D12_HEAP_PROPERTIES));
	heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_Resource default_buffer;
	HR(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(default_buffer.ReleaseAndGetAddressOf())));
	set_name(default_buffer.Get(), "[Default Buffer] name: {}", buffer_desc->name);

	if ((buffer_desc->usage == RESOURCE_USAGE_DEFAULT) && buffer_desc->data) {
		heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_Resource upload_buffer;
		HR(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(upload_buffer.ReleaseAndGetAddressOf())));
		set_name(upload_buffer.Get(), "[Upload Buffer] name: {}, frame_number: {}", buffer_desc->name, frame_number);

		void *ptr = NULL;
		HR(upload_buffer->Map(0, NULL, &ptr));
		memcpy(ptr, buffer_desc->data, buffer_desc->size());
		upload_buffer->Unmap(0, NULL);

		D3D12_Copy_Command_List *copy_command_list = get_upload_list();
		copy_command_list->copy(default_buffer, upload_buffer);

		safe_release(upload_buffer);
	}
	return (buffer_desc->usage == RESOURCE_USAGE_DEFAULT) ? new D3D12_Buffer(this, default_buffer) : new D3D12_Buffer(this);
}

Texture *D3D12_Render_Device::create_texture(Texture_Desc *texture_desc)
{
	D3D12_RESOURCE_DESC resource_desc;
	ZeroMemory(&resource_desc, sizeof(D3D12_RESOURCE_DESC));
	resource_desc.Dimension = to_d3d12_resource_dimension(texture_desc->dimension);
	resource_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resource_desc.Width = static_cast<u64>(texture_desc->width);
	resource_desc.Height = texture_desc->height;
	resource_desc.DepthOrArraySize = texture_desc->depth;
	resource_desc.MipLevels = texture_desc->miplevels;
	resource_desc.Format = texture_desc->format;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.SampleDesc.Quality = 0;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resource_desc.Flags = static_cast<D3D12_RESOURCE_FLAGS>(texture_desc->flags);

	D3D12_HEAP_PROPERTIES heap_properties;
	ZeroMemory(&heap_properties, sizeof(D3D12_HEAP_PROPERTIES));
	heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_Resource texture;
	HR(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(texture.ReleaseAndGetAddressOf())));
	set_name(texture.Get(), "[Texture] name: {}", texture_desc->name);

	if (texture_desc->data) {
		heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_Resource upload_buffer;
		HR(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(upload_buffer.ReleaseAndGetAddressOf())));
		set_name(upload_buffer.Get(), "[Upload Buffer] name: {}, frame_number: {}", texture_desc->name, frame_number);

		void *ptr = NULL;
		HR(upload_buffer->Map(0, NULL, &ptr));
		memcpy(ptr, texture_desc->data, texture_desc->size());
		upload_buffer->Unmap(0, NULL);

		D3D12_Copy_Command_List *copy_command_list = get_upload_list();
		copy_command_list->copy_buffer_to_texture(texture, upload_buffer);

		safe_release(upload_buffer);
	}
	return new D3D12_Texture(this, texture);
}

Command_List *D3D12_Render_Device::create_command_list(D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12CommandAllocator> command_allocator;
	ComPtr<ID3D12GraphicsCommandList> command_list;

	HR(device->CreateCommandAllocator(type, IID_PPV_ARGS(command_allocator.ReleaseAndGetAddressOf())));
	HR(device->CreateCommandList(0, type, command_allocator.Get(), NULL, IID_PPV_ARGS(command_list.ReleaseAndGetAddressOf())));

	D3D12_Command_List *new_command_list = new D3D12_Command_List(this, command_allocator, type, command_list);
	new_command_list->close();
	return new_command_list;
}

Copy_Command_List *D3D12_Render_Device::create_copy_command_list()
{
	return (Copy_Command_List *)create_command_list(D3D12_COMMAND_LIST_TYPE_COPY);
}

Compute_Command_List *D3D12_Render_Device::create_compute_command_list()
{
	return (Compute_Command_List *)create_command_list(D3D12_COMMAND_LIST_TYPE_COMPUTE);
}

Graphics_Command_List *D3D12_Render_Device::create_graphics_command_list()
{
	return (Graphics_Command_List *)create_command_list(D3D12_COMMAND_LIST_TYPE_DIRECT);
}

void D3D12_Render_Device::safe_release(ComPtr<ID3D12Resource> &resource)
{
	resource_release_queue.push({ frame_number, resource });
}