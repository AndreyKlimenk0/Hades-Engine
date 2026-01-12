#ifndef D3D12_DESCRIPTORS_H
#define D3D12_DESCRIPTORS_H

#include <d3d12.h>
#include <stdint.h>

#include "../render_api/render.h"
#include "../render_api/base_types.h"
#include "../../libs/number_types.h"

enum Descriptor_Type {
	DESCRIPTOR_TYPE_CBV_SRV_UAV = 0,
	DESCRIPTOR_TYPE_SAMPLER = 1,
	DESCRIPTOR_TYPE_RTV = 2,
	DESCRIPTOR_TYPE_DSV = 3
};

struct D3D12_CPU_Descriptor : CPU_Descriptor {
	D3D12_CPU_Descriptor();
	D3D12_CPU_Descriptor(Descriptor_Type type, u32 index, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle);
	~D3D12_CPU_Descriptor();

	Descriptor_Type type;
	u32 index_in_heap = UINT_MAX;
	D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;

	bool valid();
	u32 index();
};

struct D3D12_GPU_Descriptor : GPU_Descriptor {
	D3D12_GPU_Descriptor();
	D3D12_GPU_Descriptor(Descriptor_Type type, u32 index, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle);
	~D3D12_GPU_Descriptor();

	Descriptor_Type type;
	u32 index_in_heap = UINT_MAX;
	D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;

	bool valid();
	u32 index();
};
#endif
