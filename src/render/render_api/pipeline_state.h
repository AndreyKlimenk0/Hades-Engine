#ifndef RENDER_API_PIPELINE_STATE
#define RENDER_API_PIPELINE_STATE

#include <d3d12.h>
#include <dxgiformat.h>

#include "base.h"
#include "d3d12_object.h"
#include "root_signature.h"
#include "../../libs/number_types.h"
#include "../../libs/structures/array.h"

struct Shader;

enum Primitive_Type {
	PRIMITIVE_TYPE_UNKNOWN,
	PRIMITIVE_TYPE_POINT,
	PRIMITIVE_TYPE_LINE,
	PRIMITIVE_TYPE_TRIANGLE,
	PRIMITIVE_TYPE_PATCH
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

struct Render_Pipeline_Desc {
	u32 layout_offset = 0;
	Root_Signature *root_signature = NULL;
	Shader *vertex_shader = NULL;
	Shader *pixel_shader = NULL;
	Primitive_Type primitive_type = PRIMITIVE_TYPE_TRIANGLE;
	Blending_Desc blending_desc;
	Rasterization_Desc rasterization_desc;
	Depth_Stencil_Desc depth_stencil_desc;
	DXGI_FORMAT depth_stencil_format;
	Array<DXGI_FORMAT> render_targets_formats;
	Array<D3D12_INPUT_ELEMENT_DESC> input_elements;

	void add_render_target(DXGI_FORMAT format);
	void add_layout(const char *semantic_name, DXGI_FORMAT format);
	D3D12_INPUT_LAYOUT_DESC d3d12_input_layout();
};

struct Pipeline_State : D3D12_Object<ID3D12PipelineState> {
	Pipeline_State();
	~Pipeline_State();

	void create(Gpu_Device &device, Render_Pipeline_Desc &render_pipeline_desc);
};

#endif