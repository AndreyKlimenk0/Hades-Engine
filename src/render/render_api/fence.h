#ifndef RENDER_API_FENCE_H
#define RENDER_API_FENCE_H

#include <d3d12.h>
#include <windows.h>

#include "base.h"
#include "d3d12_object.h"
#include "../../libs/number_types.h"

struct Fence : D3D12_Object<ID3D12Fence> {
	Fence();
	~Fence();

	HANDLE fence_handle;
	
	void create(Gpu_Device &device, u32 initial_value = 0);
	void wait_for_gpu(u32 fence_value);
};

#endif