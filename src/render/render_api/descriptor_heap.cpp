#include <assert.h>

#include "descriptor_heap.h"
#include "../../sys/utils.h"

CPU_Descriptor::CPU_Descriptor()
{
}

CPU_Descriptor::CPU_Descriptor(u32 heap_index, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle) : index(heap_index), cpu_handle(cpu_handle)
{
}

CPU_Descriptor::~CPU_Descriptor()
{
}

GPU_Descriptor::GPU_Descriptor()
{
}

GPU_Descriptor::GPU_Descriptor(u32 heap_index, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) : CPU_Descriptor(heap_index, cpu_handle), gpu_handle(gpu_handle)
{
}

GPU_Descriptor::~GPU_Descriptor()
{
}

static D3D12_DESCRIPTOR_HEAP_TYPE descriptor_heap_type_to_d3d12(Descriptor_Heap_Type descriptor_heap_type)
{
    switch (descriptor_heap_type) {
        case DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        case DESCRIPTOR_HEAP_TYPE_SAMPLER:
            return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        case DESCRIPTOR_HEAP_TYPE_RTV:
            return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        case DESCRIPTOR_HEAP_TYPE_DSV:
            return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    }
    assert(false);
    return (D3D12_DESCRIPTOR_HEAP_TYPE)0;
}

Descriptor_Heap::Descriptor_Heap()
{
    ZeroMemory(&cpu_handle, sizeof(D3D12_CPU_DESCRIPTOR_HANDLE));
    ZeroMemory(&gpu_handle, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
}

Descriptor_Heap::~Descriptor_Heap()
{
}

void Descriptor_Heap::create(Gpu_Device &device, u32 descriptors_number, Descriptor_Heap_Type descriptor_heap_type)
{
    gpu_device = device;
    bool shader_visible = (descriptor_heap_type == DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) || (descriptor_heap_type == DESCRIPTOR_HEAP_TYPE_SAMPLER);

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
    ZeroMemory(&heap_desc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
    heap_desc.NumDescriptors = descriptors_number;
    heap_desc.Type = descriptor_heap_type_to_d3d12(descriptor_heap_type);
    heap_desc.Flags = shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HR(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(release_and_get_address())));

    descriptor_heap_capacity = descriptors_number;
    increment_size = device->GetDescriptorHandleIncrementSize(descriptor_heap_type_to_d3d12(descriptor_heap_type));
    cpu_handle = d3d12_object->GetCPUDescriptorHandleForHeapStart();
 
    if (shader_visible) {
        gpu_handle = d3d12_object->GetGPUDescriptorHandleForHeapStart();
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

CBSRUA_Descriptor_Heap::CBSRUA_Descriptor_Heap() : Descriptor_Heap()
{
}

CBSRUA_Descriptor_Heap::~CBSRUA_Descriptor_Heap()
{
}

CB_Descriptor CBSRUA_Descriptor_Heap::place_cb_descriptor(u32 descriptor_index, GPU_Resource &resource)
{
    D3D12_RESOURCE_DESC resource_desc = resource.d3d12_resource_desc();

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbuffer_view_desc;
    ZeroMemory(&cbuffer_view_desc, sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC));
    cbuffer_view_desc.BufferLocation = resource.get_gpu_address();
    cbuffer_view_desc.SizeInBytes = static_cast<u32>(resource_desc.Width);

    gpu_device->CreateConstantBufferView(&cbuffer_view_desc, get_cpu_handle(descriptor_index));

    return CB_Descriptor(descriptor_index, get_cpu_handle(descriptor_index), get_gpu_handle(descriptor_index));
}

SR_Descriptor CBSRUA_Descriptor_Heap::place_sr_descriptor(u32 descriptor_index, GPU_Resource &resource)
{
    assert(0 < resource.count);
    assert(0 < resource.stride);

    D3D12_RESOURCE_DESC resource_desc = resource.d3d12_resource_desc();

    D3D12_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
    ZeroMemory(&shader_resource_view_desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
    shader_resource_view_desc.Format = resource_desc.Format;
    shader_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    switch (resource_desc.Dimension) {
        case D3D12_RESOURCE_DIMENSION_BUFFER: {
            shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            shader_resource_view_desc.Buffer.FirstElement = 0;
            shader_resource_view_desc.Buffer.NumElements = resource.count;
            shader_resource_view_desc.Buffer.StructureByteStride = resource.stride;
            shader_resource_view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
            break;
        }
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D: {
            assert(resource_desc.DepthOrArraySize == 1);
            shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            shader_resource_view_desc.Texture2D.MostDetailedMip = 0;
            shader_resource_view_desc.Texture2D.MipLevels = resource_desc.MipLevels;
            shader_resource_view_desc.Texture2D.PlaneSlice = 0;
            shader_resource_view_desc.Texture2D.ResourceMinLODClamp = 0.0f;
            break;
        }
        default: {
            assert(false);
        }
    }
    gpu_device->CreateShaderResourceView(resource.get(), &shader_resource_view_desc, get_cpu_handle(descriptor_index));

    return SR_Descriptor(descriptor_index, get_cpu_handle(descriptor_index), get_gpu_handle(descriptor_index));
}

UA_Descriptor CBSRUA_Descriptor_Heap::place_ua_descriptor(u32 descriptor_index, GPU_Resource &resource, u32 mip_level)
{
    D3D12_RESOURCE_DESC resource_desc = resource.d3d12_resource_desc();

    D3D12_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc;
    ZeroMemory(&unordered_access_view_desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));
    unordered_access_view_desc.Format = resource_desc.Format;

    switch (resource_desc.Dimension) {
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D: {
            assert(resource_desc.DepthOrArraySize == 1);
            unordered_access_view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            unordered_access_view_desc.Texture2D.MipSlice = mip_level;
            break;
        }
        default: {
            assert(false);
        }
    }
    gpu_device->CreateUnorderedAccessView(resource.get(), NULL, &unordered_access_view_desc, get_cpu_handle(descriptor_index));

    return UA_Descriptor(descriptor_index, get_cpu_handle(descriptor_index), get_gpu_handle(descriptor_index));
}

void CBSRUA_Descriptor_Heap::create(Gpu_Device &device, u32 descriptors_number)
{
    Descriptor_Heap::create(device, descriptors_number, DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

Sampler_Descriptor_Heap::Sampler_Descriptor_Heap() : Descriptor_Heap()
{
}

Sampler_Descriptor_Heap::~Sampler_Descriptor_Heap()
{
}

Sampler_Descriptor Sampler_Descriptor_Heap::place_descriptor(u32 descriptor_index, Sampler &sampler)
{
    D3D12_SAMPLER_DESC sampler_desc = sampler.to_d3d12_sampler_desc();
    gpu_device->CreateSampler(&sampler_desc, get_cpu_handle(descriptor_index));
    return Sampler_Descriptor(descriptor_index, get_cpu_handle(descriptor_index), get_gpu_handle(descriptor_index));
}

void Sampler_Descriptor_Heap::create(Gpu_Device &device, u32 descriptors_number)
{
    Descriptor_Heap::create(device, descriptors_number, DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

RT_Descriptor_Heap::RT_Descriptor_Heap() : Descriptor_Heap()
{
}

RT_Descriptor_Heap::~RT_Descriptor_Heap()
{
}

RT_Descriptor RT_Descriptor_Heap::place_descriptor(u32 descriptor_index, GPU_Resource &resource)
{
    D3D12_RESOURCE_DESC resource_desc = resource.d3d12_resource_desc();

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
    gpu_device->CreateRenderTargetView(resource.get(), &render_target_view_desc, get_cpu_handle(descriptor_index));

    return RT_Descriptor(descriptor_index, get_cpu_handle(descriptor_index));
}

void RT_Descriptor_Heap::create(Gpu_Device &device, u32 descriptors_number)
{
    Descriptor_Heap::create(device, descriptors_number, DESCRIPTOR_HEAP_TYPE_RTV);
}

DS_Descriptor_Heap::DS_Descriptor_Heap() : Descriptor_Heap()
{
}

DS_Descriptor_Heap::~DS_Descriptor_Heap()
{
}

void DS_Descriptor_Heap::create(Gpu_Device &device, u32 descriptors_number)
{
    Descriptor_Heap::create(device, descriptors_number, DESCRIPTOR_HEAP_TYPE_DSV);
}

DS_Descriptor DS_Descriptor_Heap::place_descriptor(u32 descriptor_index, GPU_Resource &resource)
{
    D3D12_RESOURCE_DESC resource_desc = resource.d3d12_resource_desc();

    D3D12_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc;
    ZeroMemory(&depth_stencil_view_desc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
    depth_stencil_view_desc.Format = resource_desc.Format;

    switch (resource_desc.Dimension) {
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:{
            assert(resource_desc.DepthOrArraySize == 1);
            depth_stencil_view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            depth_stencil_view_desc.Texture2D.MipSlice = 0;
            break;
        }
        default: {
            assert(false);
        }
    }
    gpu_device->CreateDepthStencilView(resource.get(), &depth_stencil_view_desc, get_cpu_handle(descriptor_index));

    return DS_Descriptor(descriptor_index, get_cpu_handle(descriptor_index));
}
