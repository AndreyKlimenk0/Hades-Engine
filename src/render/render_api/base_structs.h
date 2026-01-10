#ifndef RENDER_BASE_H
#define RENDER_BASE_H

#include "base_types.h"

#include "../../libs/str.h"
#include "../../libs/color.h"
#include "../../libs/number_types.h"
#include "../../libs/math/structures.h"
#include "../../libs/structures/array.h"

enum Clear_Value_Type {
	CLEAR_VALUE_UNKNOWN,
	CLEAR_VALUE_COLOR,
	CLEAR_VALUE_DEPTH_STENCIL
};

struct Clear_Value {
	Clear_Value();
	Clear_Value(const Color &_color);
	Clear_Value(float _depht, u8 _stencil);
	~Clear_Value();

	Clear_Value_Type type;
	float depth;
	u8 stencil;
	Color color;

	bool depth_stencil_set();
	bool color_set();
};

struct Buffer_Desc {
	Resource_Usage usage = RESOURCE_USAGE_DEFAULT;
	u32 stride = 0;
	u32 count = 1;
	void *data = NULL;
	String name = "Unknown";

	u64 size();
};

const u32 ALLOW_RENDER_TARGET = 0x1;
const u32 DEPTH_STENCIL_RESOURCE = 0x2;
const u32 ALLOW_UNORDERED_ACCESS = 0x4;
//const u32 DENY_SHADER_RESOURCE = 0x8,
//const u32 ALLOW_CROSS_ADAPTER = 0x10,
//const u32 ALLOW_SIMULTANEOUS_ACCESS = 0x20,
//const u32 VIDEO_DECODE_REFERENCE_ONLY = 0x40,
//const u32 VIDEO_ENCODE_REFERENCE_ONLY = 0x80,
//const u32 RAYTRACING_ACCELERATION_STRUCTURE = 0x100;

struct Texture_Desc {
	Texture_Dimension dimension = TEXTURE_DIMENSION_UNKNOWN;
	Resource_State resource_state = RESOURCE_STATE_COMMON;
	u32 width = 0;
	u32 height = 1;
	u32 depth = 1;
	u32 miplevels = 1;
	u32 flags = 0;
	void *data = NULL;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	Clear_Value clear_value;
	String name = "Unknown";

	u64 size();
};

struct Subresource_Footprint {
	Subresource_Footprint();
	~Subresource_Footprint();

	u32 subresource_index = 0;
	u32 row_count = 0;
	u64 row_size = 0;

	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	u32 width = 0;
	u32 height = 0;
	u32 depth = 0;
	u32 row_pitch = 0;
};

struct Resource_Footprint {
	Resource_Footprint();
	~Resource_Footprint();

	u64 total_size = 0;
	Array<Subresource_Footprint> subresource_footprints;
	Subresource_Footprint subresource_footprint(u32 index)
	{
		return subresource_footprints[index];
	}
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

	void reset();
};

struct Rasterization_Desc {
	Rasterization_Desc();
	~Rasterization_Desc();

	Fill_Type fill_type;
	Cull_Type cull_type;
};

struct Blending_Desc {
	Blending_Desc();
	~Blending_Desc();

	bool enable;
	Blend_Option src;
	Blend_Option dest;
	Blend_Operation blend_op;
	Blend_Option src_alpha;
	Blend_Option dest_alpha;
	Blend_Operation blend_op_alpha;
};

struct Depth_Stencil_Desc {
	Depth_Stencil_Desc();
	~Depth_Stencil_Desc();

	bool enable_depth_test;
	bool enable_stencil_test;
	u32 stencil_read_mask;
	u32 stencil_write_mask;
	Depth_Write depth_write;
	Comparison_Func depth_compare_func;
	Comparison_Func stencil_compare_func;
	Stencil_Operation stencil_failed;
	Stencil_Operation stencil_passed_depth_failed;
	Stencil_Operation stencil_and_depth_passed;
};

struct Root_Signature;

struct Bytecode_Ref {
	void *data = NULL;
	u32 size = 0;
};

struct Input_Layout {
	const char *semantic_name = NULL;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
};

struct Graphics_Pipeline_Desc {
	Root_Signature *root_signature = NULL;
	Array<Input_Layout> input_layouts;
	Bytecode_Ref vs_bytecode;
	Bytecode_Ref ps_bytecode;
	Primitive_Type primitive_type = PRIMITIVE_TYPE_TRIANGLE;
	Blending_Desc blending_desc;
	Rasterization_Desc rasterization_desc;
	Depth_Stencil_Desc depth_stencil_desc;
	DXGI_FORMAT depth_stencil_format;
	Array<DXGI_FORMAT> render_targets_formats;

	void add_render_target(DXGI_FORMAT format);
};

struct Compute_Pipeline_Desc {
	Root_Signature *root_signature = NULL;
	Bytecode_Ref cs_bytecode;
};
#endif
