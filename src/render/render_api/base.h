#ifndef RENDER_API_CORE_H
#define RENDER_API_CORE_H

#include <d3d12.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include "../../libs/number_types.h"
#include "../../libs/math/structures.h"

typedef u64 GPU_Address;
typedef ComPtr<ID3D12Device> Gpu_Device;

struct Viewport {
	Viewport();
	Viewport(const Size_f32 &size);
	~Viewport();

	float x = 0.0f;
	float y = 0.0f;
	float width = 0.0f;
	float height = 0.0f;
	float min_depth = 0.0f;
	float max_depth = 1.0f;
};

enum Primitive_Type {
	PRIMITIVE_TYPE_UNKNOWN,
	PRIMITIVE_TYPE_POINT,
	PRIMITIVE_TYPE_LINE,
	PRIMITIVE_TYPE_TRIANGLE,
	PRIMITIVE_TYPE_PATCH
};

bool check_tearing_support();
bool create_d3d12_gpu_device(Gpu_Device &device);

#endif