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

void Buffer::create(Gpu_Device &device, GPU_Heap &heap, u64 offset, Resource_State resource_state, const Buffer_Desc &buffer_desc)
{
    set_size(buffer_desc.count, buffer_desc.stride);

    D3D12_RESOURCE_DESC resource_desc;
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

    GPU_Resource::create(device, heap, offset, resource_state, resource_desc, (Clear_Value &)buffer_desc.clear_value);
}

void Buffer::create(Gpu_Device &device, GPU_Heap_Type heap_type, Resource_State resource_state, const Buffer_Desc &buffer_desc)
{
    set_size(buffer_desc.count, buffer_desc.stride);

    D3D12_RESOURCE_DESC resource_desc;
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

    GPU_Resource::create(device, heap_type, resource_state, resource_desc, (Clear_Value &)buffer_desc.clear_value);
}

Buffer_Desc::Buffer_Desc()
{
}

Buffer_Desc::Buffer_Desc(u32 size) : count(1), stride(size) 
{
}

Buffer_Desc::Buffer_Desc(u32 count, u32 stride) : count(count), stride(stride)
{
}

Buffer_Desc::~Buffer_Desc()
{
}

u32 Buffer_Desc::get_size()
{
    return count * stride;
}
