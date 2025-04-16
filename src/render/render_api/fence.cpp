#include "fence.h"
#include "../../sys/utils.h"
#include "../../win32/win_helpers.h"

Fence::Fence()
{
	handle = create_event_handle();
}

Fence::~Fence()
{
	close_event_handle(handle);
}

void Fence::create(Gpu_Device &device, u64 initial_expected_value)
{
	expected_value = initial_expected_value;
	HR(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(release_and_get_address())));
}

bool Fence::wait_for_gpu()
{
	u64 completed_value = d3d12_object->GetCompletedValue();
	if (completed_value < expected_value) {
		d3d12_object->SetEventOnCompletion(expected_value, handle);
		WaitForSingleObject(handle, INFINITE);
		return true;
	}
	return false;
}

u64 Fence::increment_expected_value()
{
	return ++expected_value;
}
