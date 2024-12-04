#ifndef RENDER_API_CORE_H
#define RENDER_API_CORE_H

#include <d3d12.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include "../../libs/number_types.h"

typedef u64 GPU_Address;
typedef ComPtr<ID3D12Device> Gpu_Device;

bool check_tearing_support();
bool create_d3d12_gpu_device(Gpu_Device &device);

#endif