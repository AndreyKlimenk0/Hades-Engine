#ifndef RENDER_API_GPU_RESOURCE_H
#define RENDER_API_GPU_RESOURCE_H

#include <d3d12.h>

#include "base.h"
#include "d3d12_object.h"
#include "../../libs/number_types.h"

struct GPU_Resource : D3D12_Object<ID3D12Resource> {
	GPU_Resource();
	~GPU_Resource();

	u32 count = 0;
	u32 stride = 0;

	u8 *map();
	void unmap();

	u64 get_gpu_address();
	D3D12_RESOURCE_DESC d3d12_resource_desc();
};

#endif