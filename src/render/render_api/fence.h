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

	u64 expected_value;
	HANDLE handle;
	
	void create(Gpu_Device &device, u64 initial_expected_value = 0);
	bool wait_for_gpu();
	u64 increment_expected_value();
};

#endif