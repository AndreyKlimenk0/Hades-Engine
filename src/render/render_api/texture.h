
#ifndef RENDER_API_TEXTURE_H
#define RENDER_API_TEXTURE_H

#include "resource.h"
#include "descriptor_heap.h"

#include "../../libs/color.h"
#include "../../libs/number_types.h"

//const u32 ALLOW_RENDER_TARGET = 0x1,
const u32 DEPTH_STENCIL_RESOURCE = 0x2;
//const u32 ALLOW_UNORDERED_ACCESS = 0x4,
//const u32 DENY_SHADER_RESOURCE = 0x8,
//const u32 ALLOW_CROSS_ADAPTER = 0x10,
//const u32 ALLOW_SIMULTANEOUS_ACCESS = 0x20,
//const u32 VIDEO_DECODE_REFERENCE_ONLY = 0x40,
//const u32 VIDEO_ENCODE_REFERENCE_ONLY = 0x80,
//const u32 RAYTRACING_ACCELERATION_STRUCTURE = 0x100;

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

struct Texture2D_Desc {
	u32 width = 0;
	u32 height = 0;
	u32 miplevels = 1;
	u32 flags = 0;
	DXGI_FORMAT format;
	Clear_Value clear_value;
};

struct Texture : GPU_Resource {
	Texture();
	~Texture();

	SR_Descriptor sr_descriptor;
	RT_Descriptor rt_descriptor;
	DS_Descriptor ds_descriptor;

	void create(Gpu_Device &device, Texture2D_Desc &desc);
};

#endif
