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

GPU_Heap::GPU_Heap()
{
}

GPU_Heap::~GPU_Heap()
{
}

void GPU_Heap::create(Gpu_Device &device, u32 heap_size, GPU_Heap_Type heap_type, GPU_Heap_Content content)
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

void GPU_Resource::set_size(u32 number_items, u32 item_size)
{
    count = number_items;
    stride = item_size;
}

u32 GPU_Resource::get_size()
{
    return count * stride;
}

GPU_Address GPU_Resource::get_gpu_address()
{
    return d3d12_object->GetGPUVirtualAddress();
}

D3D12_RESOURCE_DESC GPU_Resource::d3d12_resource_desc()
{
    return d3d12_object->GetDesc();
}