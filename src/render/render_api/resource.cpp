#include "resource.h"

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

GPU_Address GPU_Resource::get_gpu_address()
{
    return d3d12_object->GetGPUVirtualAddress();
}

D3D12_RESOURCE_DESC GPU_Resource::d3d12_resource_desc()
{
    return d3d12_object->GetDesc();
}
