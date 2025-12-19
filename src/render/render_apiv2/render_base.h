#ifndef RENDER_BASE_H
#define RENDER_BASE_H

#include "render_types.h"

#include "../../libs/str.h"
#include "../../libs/color.h"
#include "../../libs/number_types.h"
#include "../../libs/math/structures.h"

struct Buffer_Desc {
	Resource_Usage usage = RESOURCE_USAGE_DEFAULT;
	u32 stride = 0;
	u32 count = 0;
	void *data = NULL;
	String name = "Unknown";

	u64 size();
};

struct Texture_Desc {
	Texture_Dimension dimension = TEXTURE_DIMENSION_UNKNOWN;
	Resource_State resource_state = RESOURCE_STATE_COMMON;
	u32 width = 0;
	u32 height = 1;
	u32 depth = 1;
	u32 miplevels = 1;
	u32 flags = 0;
	void *data = NULL;
	DXGI_FORMAT format;
	Clear_Value clear_value;
	String name = "Unknown";

	u64 size();
};

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

	bool depth_stencil_set();
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

enum Fill_Type {
	FILL_TYPE_WIREFRAME,
	FILL_TYPE_SOLID
};

enum Cull_Type {
	CULL_TYPE_UNKNOWN,
	CULL_TYPE_FRONT,
	CULL_TYPE_BACK
};

struct Rasterization_Desc {
	Rasterization_Desc();
	~Rasterization_Desc();

	Fill_Type fill_type;
	Cull_Type cull_type;
};

enum Blend_Option {
	BLEND_ZERO,
	BLEND_ONE,
	BLEND_SRC_COLOR,
	BLEND_INV_SRC_COLOR,
	BLEND_SRC_ALPHA,
	BLEND_INV_SRC_ALPHA,
	BLEND_DEST_ALPHA,
	BLEND_INV_DEST_ALPHA,
	BLEND_DEST_COLOR,
	BLEND_INV_DEST_COLOR,
	BLEND_SRC_ALPHA_SAT,
	BLEND_BLEND_FACTOR,
	BLEND_INV_BLEND_FACTOR,
	BLEND_SRC1_COLOR,
	BLEND_INV_SRC1_COLOR,
	BLEND_SRC1_ALPHA,
	BLEND_INV_SRC1_ALPHA
};

enum Blend_Operation {
	BLEND_OP_ADD,
	BLEND_OP_SUBTRACT,
	BLEND_OP_REV_SUBTRACT,
	BLEND_OP_MIN,
	BLEND_OP_MAX
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

enum Stencil_Operation {
	STENCIL_OP_KEEP,
	STENCIL_OP_ZERO,
	STENCIL_OP_REPLACE,
	STENCIL_OP_INCR_SAT,
	STENCIL_OP_DECR_SAT,
	STENCIL_OP_INVERT,
	STENCIL_OP_INCR,
	STENCIL_OP_DECR
};

enum Comparison_Func {
	COMPARISON_NEVER,
	COMPARISON_LESS,
	COMPARISON_EQUAL,
	COMPARISON_LESS_EQUAL,
	COMPARISON_GREATER,
	COMPARISON_NOT_EQUAL,
	COMPARISON_GREATER_EQUAL,
	COMPARISON_ALWAYS
};

enum Depth_Write {
	DEPTH_WRITE_ZERO,
	DEPTH_WRITE_ALL
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

struct Shader;
struct Root_Signature;

struct Graphics_Pipeline_Desc {
	Root_Signature *root_signature = NULL;
	Shader *vertex_shader = NULL;
	Shader *pixel_shader = NULL;
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
	Shader *compute_shader = NULL;
};
#endif
