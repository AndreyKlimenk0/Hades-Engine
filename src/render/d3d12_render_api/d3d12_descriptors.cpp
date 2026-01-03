#include "d3d12_descriptors.h"


D3D12_CPU_Descriptor::D3D12_CPU_Descriptor()
{
}

D3D12_CPU_Descriptor::D3D12_CPU_Descriptor(Descriptor_Type type, u32 index, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle) : type(type), index_in_heap(index), cpu_handle(cpu_handle)
{
}

D3D12_CPU_Descriptor::~D3D12_CPU_Descriptor()
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

D3D12_GPU_Descriptor::D3D12_GPU_Descriptor()
{
}

D3D12_GPU_Descriptor::D3D12_GPU_Descriptor(Descriptor_Type type, u32 index, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) : type(type), index_in_heap(index), cpu_handle(cpu_handle), gpu_handle(gpu_handle)
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


