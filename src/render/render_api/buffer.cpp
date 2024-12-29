#include <assert.h>
#include "buffer.h"
#include "../../sys/utils.h"

static D3D12_HEAP_TYPE _to_d3d12_heap_type(GPU_Heap_Type type)
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

Buffer::Buffer()
{
}

Buffer::~Buffer()
{
}

u8 *Buffer::map()
{
    u8 *memory = NULL;
    d3d12_object->Map(0, NULL, (void **)&memory);
    return memory;
}

void Buffer::unmap()
{
    d3d12_object->Unmap(0, NULL);
}

void Buffer::create(Gpu_Device &device, GPU_Heap &heap, u32 number_items, u32 item_size)
{
    set_size(number_items, item_size);

    D3D12_RESOURCE_DESC resource_desc = {};
    ZeroMemory(&resource_desc, sizeof(D3D12_RESOURCE_DESC));
    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    resource_desc.Width = get_size();
    resource_desc.Height = 1;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    auto resource_state = (heap.d3d12_heap_desc().Properties.Type == D3D12_HEAP_TYPE_DEFAULT) ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_GENERIC_READ;

    device->CreatePlacedResource(heap.get(), heap.offset, &resource_desc, resource_state, NULL, IID_PPV_ARGS(release_and_get_address()));
    heap.offset += D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
}

void Buffer::create(Gpu_Device &device, GPU_Heap_Type heap_type, u32 number_items, u32 item_size)
{
    set_size(number_items, item_size);

    D3D12_HEAP_PROPERTIES heap_properties = {};
    ZeroMemory(&heap_properties, sizeof(D3D12_HEAP_PROPERTIES));
    heap_properties.Type = _to_d3d12_heap_type(heap_type);

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

    auto resource_state = (heap_type == GPU_HEAP_TYPE_DEFAULT) ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_GENERIC_READ;

    HR(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, resource_state, NULL, IID_PPV_ARGS(release_and_get_address())));
}

