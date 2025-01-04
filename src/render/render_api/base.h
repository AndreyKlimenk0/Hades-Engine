#ifndef RENDER_API_CORE_H
#define RENDER_API_CORE_H

#include <d3d12.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include "../../libs/number_types.h"
#include "../../libs/color.h"
#include "../../libs/math/structures.h"

typedef u64 GPU_Address;
typedef ComPtr<ID3D12Device> Gpu_Device;

enum Clear_Value_Type {
	CLEAR_VALUE_UNKNOWN,
	CLEAR_VALUE_COLOR,
	CLEAR_VALUE_DEPTH_STENCIL
};

struct Clear_Value {
	Clear_Value();
	Clear_Value(Color &_color);
	Clear_Value(float _depht, u8 _stencil);
	~Clear_Value();

	Clear_Value_Type type;
	float depth;
	u8 stencil;
	Color color;
};

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

enum Resource_State {
	RESOURCE_STATE_UNKNOWN,
	RESOURCE_STATE_COMMON,
	RESOURCE_STATE_GENERIC_READ,
	RESOURCE_STATE_COPY_DEST,
	RESOURCE_STATE_COPY_SOURCE,
	RESOURCE_STATE_PRESENT,
	RESOURCE_STATE_RENDER_TARGET,
	RESOURCE_STATE_DEPTH_WRITE
};

bool check_tearing_support();
bool create_d3d12_gpu_device(Gpu_Device &device);

D3D12_RESOURCE_STATES to_d3d12_resource_state(const Resource_State &resource_state);

#endif