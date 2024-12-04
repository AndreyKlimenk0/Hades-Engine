#include "fence.h"
#include "../../sys/utils.h"
#include "../../win32/win_helpers.h"

Fence::Fence()
{
	fence_handle = create_event_handle();
}

Fence::~Fence()
{
	close_event_handle(fence_handle);
}

void Fence::create(Gpu_Device &device, u32 initial_value)
{
	HR(device->CreateFence(initial_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(release_and_get_address())));
}

void Fence::wait_for_gpu(u32 fence_value)
{
	if (d3d12_object->GetCompletedValue() < fence_value) {
		d3d12_object->SetEventOnCompletion(fence_value, fence_handle);
		WaitForSingleObject(fence_handle, INFINITE);
	}
}
