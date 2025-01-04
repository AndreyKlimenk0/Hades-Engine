#include <assert.h>

#include "resource.h"
#include "../../sys/utils.h"

static D3D12_HEAP_TYPE to_d3d12_heap_type(GPU_Heap_Type type)
{
    switch (type) {
        case GPU_HEAP_TYPE_DEFAULT:
            return D3D12_HEAP_TYPE_DEFAULT;
        case GPU_HEAP_TYPE_UPLOAD:
            return D3D12_HEAP_TYPE_UPLOAD;
        case GPU_HEAP_TYPE_READBACK:
            return D3D12_HEAP_TYPE_READBACK;
    }
    assert(false);
    return (D3D12_HEAP_TYPE)0;
}

static D3D12_HEAP_FLAGS to_d3d12_heap_flags(GPU_Heap_Content content)
{
    switch (content) {
        case GPU_HEAP_CONTAIN_BUFFERS:
            return D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
        case GPU_HEAP_CONTAIN_BUFFERS_AND_TEXTURES:
            return D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
        case GPU_HEAP_CONTAIN_RT_DS_TEXTURES:
            return D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
    }
    assert(false);
    return (D3D12_HEAP_FLAGS)0;
}

static D3D12_CLEAR_VALUE to_d3d12_clear_value(Clear_Value &clear_value, DXGI_FORMAT format)
{
    D3D12_CLEAR_VALUE d3d12_clear_value;
    ZeroMemory(&d3d12_clear_value, sizeof(D3D12_CLEAR_VALUE));
    d3d12_clear_value.Format = format;
    if (clear_value.type == CLEAR_VALUE_COLOR) {
        clear_value.color.store(d3d12_clear_value.Color);
    } else if (clear_value.type == CLEAR_VALUE_DEPTH_STENCIL) {
        d3d12_clear_value.DepthStencil.Depth = clear_value.depth;
        d3d12_clear_value.DepthStencil.Stencil = clear_value.stencil;
    }
    return d3d12_clear_value;
}

GPU_Heap::GPU_Heap()
{
}

GPU_Heap::~GPU_Heap()
{
}

void GPU_Heap::create(Gpu_Device &device, u64 heap_size, GPU_Heap_Type heap_type, GPU_Heap_Content content)
{
    D3D12_HEAP_PROPERTIES heap_properties;
    ZeroMemory(&heap_properties, sizeof(D3D12_HEAP_PROPERTIES));
    heap_properties.Type = to_d3d12_heap_type(heap_type);
    heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_HEAP_DESC heap_desc;
    ZeroMemory(&heap_desc, sizeof(D3D12_HEAP_DESC));
    heap_desc.SizeInBytes = heap_size;
    heap_desc.Properties = heap_properties;
    heap_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    heap_desc.Flags = to_d3d12_heap_flags(content);

    HR(device->CreateHeap(&heap_desc, IID_PPV_ARGS(release_and_get_address())));
}

D3D12_HEAP_DESC GPU_Heap::d3d12_heap_desc()
{
    return d3d12_object->GetDesc();
}

GPU_Resource::GPU_Resource()
{
}

GPU_Resource::~GPU_Resource()
{
}

void GPU_Resource::create(Gpu_Device &device, GPU_Heap_Type heap_type, Resource_State resource_state, D3D12_RESOURCE_DESC &resource_desc, Clear_Value &clear_value)
{
    D3D12_HEAP_PROPERTIES heap_properties;
    ZeroMemory(&heap_properties, sizeof(D3D12_HEAP_PROPERTIES));
    heap_properties.Type = to_d3d12_heap_type(heap_type);

    D3D12_CLEAR_VALUE *d3d12_clear_value = NULL;
    D3D12_CLEAR_VALUE temp = to_d3d12_clear_value(clear_value, resource_desc.Format);
    if ((resource_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) || (resource_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) {
        d3d12_clear_value = &temp;
    }

    HR(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, to_d3d12_resource_state(resource_state), d3d12_clear_value, IID_PPV_ARGS(release_and_get_address())));
}

void GPU_Resource::create(Gpu_Device &device, GPU_Heap heap, u64 offset, Resource_State resource_state, D3D12_RESOURCE_DESC &resource_desc, Clear_Value &clear_value)
{
    HR(device->CreatePlacedResource(heap.get(), offset, &resource_desc, to_d3d12_resource_state(resource_state), NULL, IID_PPV_ARGS(release_and_get_address())));
}

void GPU_Resource::set_size(u32 _count, u32 _stride)
{
    count = _count;
    stride = _stride;
}

u32 GPU_Resource::get_size()
{
    return stride * count;
}

GPU_Address GPU_Resource::get_gpu_address()
{
    return d3d12_object->GetGPUVirtualAddress();
}

D3D12_RESOURCE_DESC GPU_Resource::d3d12_resource_desc()
{
    return d3d12_object->GetDesc();
}