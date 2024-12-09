#include <assert.h>

#include "resource.h"
#include "../../sys/utils.h"

GPU_Resource::GPU_Resource()
{
}

GPU_Resource::~GPU_Resource()
{
}

u8 *GPU_Resource::map()
{
    u8 *memory = NULL;
    d3d12_object->Map(0, NULL, (void **)&memory);
    return memory;
}

void GPU_Resource::unmap()
{
    d3d12_object->Unmap(0, NULL);
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

void Constant_Buffer::create(Gpu_Device &device, u32 size)
{
    count = 1;
    stride = size;

    D3D12_HEAP_PROPERTIES heap_properties = {};
    ZeroMemory(&heap_properties, sizeof(D3D12_HEAP_PROPERTIES));
    heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resource_desc = {};
    ZeroMemory(&resource_desc, sizeof(D3D12_RESOURCE_DESC));
    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment = 0;
    resource_desc.Width = size;
    resource_desc.Height = 1;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HR(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(release_and_get_address())));
}

void Buffer::create(Gpu_Device &device, D3D12_HEAP_TYPE heap_type, u32 number_items, u32 item_size)
{
    assert((heap_type == D3D12_HEAP_TYPE_DEFAULT) || (heap_type == D3D12_HEAP_TYPE_UPLOAD));
    
    count = number_items;
    stride = item_size;

    D3D12_HEAP_PROPERTIES heap_properties = {};
    ZeroMemory(&heap_properties, sizeof(D3D12_HEAP_PROPERTIES));
    heap_properties.Type = heap_type;

    D3D12_RESOURCE_DESC resource_desc = {};
    ZeroMemory(&resource_desc, sizeof(D3D12_RESOURCE_DESC));
    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment = 0;
    resource_desc.Width = get_size();
    resource_desc.Height = 1;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    auto resource_state = (heap_type == D3D12_HEAP_TYPE_DEFAULT) ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_GENERIC_READ;

    HR(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, resource_state, NULL, IID_PPV_ARGS(release_and_get_address())));
}
