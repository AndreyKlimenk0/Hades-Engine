#ifndef TO_D3D12_TYPES_H
#define TO_D3D12_TYPES_H

#include <assert.h>
#include <d3d12.h>

#include "../base_types.h"
#include "../base_structs.h"

inline D3D12_HEAP_TYPE to_d3d12_heap_type(Resource_Usage type)
{
	switch (type) {
		case RESOURCE_USAGE_DEFAULT:
			return D3D12_HEAP_TYPE_DEFAULT;
		case RESOURCE_USAGE_UPLOAD:
			return D3D12_HEAP_TYPE_UPLOAD;
		case RESOURCE_USAGE_READBACK:
			return D3D12_HEAP_TYPE_READBACK;
	}
	assert(false);
	return (D3D12_HEAP_TYPE)0;
}

inline D3D12_RESOURCE_DIMENSION to_d3d12_resource_dimension(Texture_Dimension texture_dimension)
{
	switch (texture_dimension) {
		case TEXTURE_DIMENSION_UNKNOWN:
			return D3D12_RESOURCE_DIMENSION_UNKNOWN;
		case TEXTURE_DIMENSION_1D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		case TEXTURE_DIMENSION_2D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		case TEXTURE_DIMENSION_3D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	}
	assert(false);
	return static_cast<D3D12_RESOURCE_DIMENSION>(0);
}

inline D3D12_RESOURCE_STATES to_d3d12_resource_state(const Resource_State &resource_state)
{
	switch (resource_state) {
		case RESOURCE_STATE_COMMON:
			return D3D12_RESOURCE_STATE_COMMON;
		case RESOURCE_STATE_GENERIC_READ:
			return D3D12_RESOURCE_STATE_GENERIC_READ;
		case RESOURCE_STATE_COPY_DEST:
			return D3D12_RESOURCE_STATE_COPY_DEST;
		case RESOURCE_STATE_COPY_SOURCE:
			return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case RESOURCE_STATE_RENDER_TARGET:
			return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case RESOURCE_STATE_PRESENT:
			return D3D12_RESOURCE_STATE_PRESENT;
		case RESOURCE_STATE_DEPTH_WRITE:
			return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
			return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		case RESOURCE_STATE_ALL_SHADER_RESOURCE:
			return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}
	assert(false);
	return (D3D12_RESOURCE_STATES)0;
}

inline D3D_PRIMITIVE_TOPOLOGY to_d3d12_primitive_topology(Primitive_Type primitive_type)
{
	switch (primitive_type) {
		case PRIMITIVE_TYPE_UNKNOWN:
			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		case PRIMITIVE_TYPE_POINT:
			return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case PRIMITIVE_TYPE_LINE:
			return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case PRIMITIVE_TYPE_TRIANGLE:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case PRIMITIVE_TYPE_PATCH:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	}
	assert(false);
	return (D3D_PRIMITIVE_TOPOLOGY)0;
}

inline DXGI_FORMAT to_shader_resource_view_format(DXGI_FORMAT format)
{
	switch (format) {
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_TYPELESS:
			return DXGI_FORMAT_R32_FLOAT;
	}
	return format;
}

inline DXGI_FORMAT to_depth_stencil_view_format(DXGI_FORMAT format)
{
	switch (format) {
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_TYPELESS:
			return DXGI_FORMAT_D32_FLOAT;
	}
	return format;
}

inline D3D12_COMMAND_LIST_TYPE to_d3d12_command_list_type(Command_List_Type command_list_type)
{
	switch (command_list_type) {
		case COMMAND_LIST_TYPE_DIRECT:
			return D3D12_COMMAND_LIST_TYPE_DIRECT;
		case COMMAND_LIST_TYPE_BUNDLE:
			return D3D12_COMMAND_LIST_TYPE_BUNDLE;
		case COMMAND_LIST_TYPE_COMPUTE:
			return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		case COMMAND_LIST_TYPE_COPY:
			return D3D12_COMMAND_LIST_TYPE_COPY;
		default:
			assert(false);
	}
	return (D3D12_COMMAND_LIST_TYPE)0;
}

inline D3D12_PRIMITIVE_TOPOLOGY_TYPE to_d3d12_primitive_topology_type(Primitive_Type &primitive_type)
{
    switch (primitive_type) {
        case PRIMITIVE_TYPE_UNKNOWN:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
        case PRIMITIVE_TYPE_POINT:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case PRIMITIVE_TYPE_LINE:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case PRIMITIVE_TYPE_TRIANGLE:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        case PRIMITIVE_TYPE_PATCH:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    }
    assert(false);
    return (D3D12_PRIMITIVE_TOPOLOGY_TYPE)0;
}

inline D3D12_BLEND to_d3d12_blend_option(Blend_Option blend_option)
{
    switch (blend_option) {
        case BLEND_ZERO:
            return D3D12_BLEND_ZERO;
        case BLEND_ONE:
            return D3D12_BLEND_ONE;
        case BLEND_SRC_COLOR:
            return D3D12_BLEND_SRC_COLOR;
        case BLEND_INV_SRC_COLOR:
            return D3D12_BLEND_INV_SRC_COLOR;
        case BLEND_SRC_ALPHA:
            return D3D12_BLEND_SRC_ALPHA;
        case BLEND_INV_SRC_ALPHA:
            return D3D12_BLEND_INV_SRC_ALPHA;
        case BLEND_DEST_ALPHA:
            return D3D12_BLEND_DEST_ALPHA;
        case BLEND_INV_DEST_ALPHA:
            return D3D12_BLEND_INV_DEST_ALPHA;
        case BLEND_DEST_COLOR:
            return D3D12_BLEND_DEST_COLOR;
        case BLEND_INV_DEST_COLOR:
            return D3D12_BLEND_INV_DEST_COLOR;
        case BLEND_SRC_ALPHA_SAT:
            return D3D12_BLEND_SRC_ALPHA_SAT;
        case BLEND_BLEND_FACTOR:
            return D3D12_BLEND_BLEND_FACTOR;
        case BLEND_INV_BLEND_FACTOR:
            return D3D12_BLEND_INV_BLEND_FACTOR;
        case BLEND_SRC1_COLOR:
            return D3D12_BLEND_SRC1_COLOR;
        case BLEND_INV_SRC1_COLOR:
            return D3D12_BLEND_INV_SRC1_COLOR;
        case BLEND_SRC1_ALPHA:
            return D3D12_BLEND_SRC1_ALPHA;
        case BLEND_INV_SRC1_ALPHA:
            return D3D12_BLEND_INV_SRC1_ALPHA;
    }
    assert(false);
    return (D3D12_BLEND)0;
}

inline D3D12_BLEND_OP to_d3d12_blend_operator(Blend_Operation blend_operation)
{
    switch (blend_operation) {
        case BLEND_OP_ADD:
            return D3D12_BLEND_OP_ADD;
        case BLEND_OP_SUBTRACT:
            return D3D12_BLEND_OP_SUBTRACT;
        case BLEND_OP_REV_SUBTRACT:
            return D3D12_BLEND_OP_REV_SUBTRACT;
        case BLEND_OP_MIN:
            return D3D12_BLEND_OP_MIN;
        case BLEND_OP_MAX:
            return D3D12_BLEND_OP_MAX;
    }
    assert(false);
    return (D3D12_BLEND_OP)0;
}

inline D3D12_FILL_MODE to_d3d12_fill_mode(Fill_Type fill_type)
{
    switch (fill_type) {
        case FILL_TYPE_WIREFRAME:
            return D3D12_FILL_MODE_WIREFRAME;
        case FILL_TYPE_SOLID:
            return D3D12_FILL_MODE_SOLID;
    }
    assert(false);
    return (D3D12_FILL_MODE)0;
}

inline D3D12_CULL_MODE to_d3d12_cull_mode(Cull_Type cull_type)
{
    switch (cull_type) {
        case CULL_TYPE_UNKNOWN:
            return D3D12_CULL_MODE_NONE;
        case CULL_TYPE_FRONT:
            return D3D12_CULL_MODE_FRONT;
        case CULL_TYPE_BACK:
            return D3D12_CULL_MODE_BACK;
    }
    assert(false);
    return (D3D12_CULL_MODE)0;
}

inline D3D12_STENCIL_OP to_d3d12_stencil_op(Stencil_Operation stencil_operation)
{
    switch (stencil_operation) {
        case STENCIL_OP_KEEP:
            return D3D12_STENCIL_OP_KEEP;
        case STENCIL_OP_ZERO:
            return D3D12_STENCIL_OP_ZERO;
        case STENCIL_OP_REPLACE:
            return D3D12_STENCIL_OP_REPLACE;
        case STENCIL_OP_INCR_SAT:
            return D3D12_STENCIL_OP_INCR_SAT;
        case STENCIL_OP_DECR_SAT:
            return D3D12_STENCIL_OP_DECR_SAT;
        case STENCIL_OP_INVERT:
            return D3D12_STENCIL_OP_INVERT;
        case STENCIL_OP_INCR:
            return D3D12_STENCIL_OP_INCR;
        case STENCIL_OP_DECR:
            return D3D12_STENCIL_OP_DECR;
    }
    assert(false);
    return (D3D12_STENCIL_OP)0;
}

inline D3D12_COMPARISON_FUNC to_d3d12_comparison_func(Comparison_Func func)
{
    switch (func) {
        case COMPARISON_NEVER:
            return D3D12_COMPARISON_FUNC_NEVER;
        case COMPARISON_LESS:
            return D3D12_COMPARISON_FUNC_LESS;
        case COMPARISON_EQUAL:
            return D3D12_COMPARISON_FUNC_EQUAL;
        case COMPARISON_LESS_EQUAL:
            return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case COMPARISON_GREATER:
            return D3D12_COMPARISON_FUNC_GREATER;
        case COMPARISON_NOT_EQUAL:
            return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case COMPARISON_GREATER_EQUAL:
            return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case COMPARISON_ALWAYS:
            return D3D12_COMPARISON_FUNC_ALWAYS;
    }
    assert(false);
    return (D3D12_COMPARISON_FUNC)0;
}

inline D3D12_DEPTH_WRITE_MASK to_d3d12_depth_write_mask(Depth_Write depth_write)
{
    switch (depth_write) {
        case DEPTH_WRITE_ZERO:
            return D3D12_DEPTH_WRITE_MASK_ZERO;
        case DEPTH_WRITE_ALL:
            return D3D12_DEPTH_WRITE_MASK_ALL;
    }
    assert(false);
    return (D3D12_DEPTH_WRITE_MASK)0;
}

inline D3D12_BLEND_DESC to_d3d12_blend_desc(u32 render_target_count, Blending_Desc blending_desc)
{
    assert(render_target_count <= 8);

    D3D12_BLEND_DESC d3d12_blend_desc;
    ZeroMemory(&d3d12_blend_desc, sizeof(D3D12_BLEND_DESC));
    for (u32 i = 0; i < render_target_count; i++) {
        d3d12_blend_desc.RenderTarget[i].BlendEnable = blending_desc.enable;
        d3d12_blend_desc.RenderTarget[i].LogicOpEnable = blending_desc.enable;
        d3d12_blend_desc.RenderTarget[i].SrcBlend = to_d3d12_blend_option(blending_desc.src);
        d3d12_blend_desc.RenderTarget[i].DestBlend = to_d3d12_blend_option(blending_desc.dest);
        d3d12_blend_desc.RenderTarget[i].BlendOp = to_d3d12_blend_operator(blending_desc.blend_op);
        d3d12_blend_desc.RenderTarget[i].SrcBlendAlpha = to_d3d12_blend_option(blending_desc.src_alpha);
        d3d12_blend_desc.RenderTarget[i].DestBlendAlpha = to_d3d12_blend_option(blending_desc.dest_alpha);
        d3d12_blend_desc.RenderTarget[i].BlendOpAlpha = to_d3d12_blend_operator(blending_desc.blend_op_alpha);
        d3d12_blend_desc.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
        d3d12_blend_desc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }
    return d3d12_blend_desc;
}

inline D3D12_RASTERIZER_DESC to_d3d12_rasterizer_desc(Rasterization_Desc rasterization_desc)
{
    D3D12_RASTERIZER_DESC d3d12_rasterizer_desc;
    ZeroMemory(&d3d12_rasterizer_desc, sizeof(D3D12_RASTERIZER_DESC));
    d3d12_rasterizer_desc.FillMode = to_d3d12_fill_mode(rasterization_desc.fill_type);
    d3d12_rasterizer_desc.CullMode = to_d3d12_cull_mode(rasterization_desc.cull_type);
    d3d12_rasterizer_desc.FrontCounterClockwise = FALSE;
    d3d12_rasterizer_desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    d3d12_rasterizer_desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    d3d12_rasterizer_desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    d3d12_rasterizer_desc.DepthClipEnable = TRUE;
    d3d12_rasterizer_desc.MultisampleEnable = FALSE;
    d3d12_rasterizer_desc.AntialiasedLineEnable = FALSE;
    d3d12_rasterizer_desc.ForcedSampleCount = 0;
    d3d12_rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    return d3d12_rasterizer_desc;
}

inline D3D12_DEPTH_STENCIL_DESC to_d3d12_depth_stencil_desc(Depth_Stencil_Desc depth_stencil_desc)
{
    D3D12_DEPTH_STENCIL_DESC d3d12_depth_stencil_desc;
    ZeroMemory(&d3d12_depth_stencil_desc, sizeof(D3D12_DEPTH_STENCIL_DESC));
    d3d12_depth_stencil_desc.DepthEnable = depth_stencil_desc.enable_depth_test;
    d3d12_depth_stencil_desc.DepthWriteMask = to_d3d12_depth_write_mask(depth_stencil_desc.depth_write);
    d3d12_depth_stencil_desc.DepthFunc = to_d3d12_comparison_func(depth_stencil_desc.depth_compare_func);

    d3d12_depth_stencil_desc.StencilEnable = depth_stencil_desc.enable_stencil_test;
    d3d12_depth_stencil_desc.StencilReadMask = depth_stencil_desc.stencil_read_mask;
    d3d12_depth_stencil_desc.StencilWriteMask = depth_stencil_desc.stencil_write_mask;

    d3d12_depth_stencil_desc.FrontFace.StencilFailOp = to_d3d12_stencil_op(depth_stencil_desc.stencil_failed);
    d3d12_depth_stencil_desc.FrontFace.StencilDepthFailOp = to_d3d12_stencil_op(depth_stencil_desc.stencil_passed_depth_failed);
    d3d12_depth_stencil_desc.FrontFace.StencilPassOp = to_d3d12_stencil_op(depth_stencil_desc.stencil_and_depth_passed);
    d3d12_depth_stencil_desc.FrontFace.StencilFunc = to_d3d12_comparison_func(depth_stencil_desc.stencil_compare_func);

    d3d12_depth_stencil_desc.BackFace.StencilFailOp = to_d3d12_stencil_op(depth_stencil_desc.stencil_failed);
    d3d12_depth_stencil_desc.BackFace.StencilDepthFailOp = to_d3d12_stencil_op(depth_stencil_desc.stencil_passed_depth_failed);
    d3d12_depth_stencil_desc.BackFace.StencilPassOp = to_d3d12_stencil_op(depth_stencil_desc.stencil_and_depth_passed);
    d3d12_depth_stencil_desc.BackFace.StencilFunc = to_d3d12_comparison_func(depth_stencil_desc.stencil_compare_func);

    return d3d12_depth_stencil_desc;
}

inline D3D12_SHADER_BYTECODE tO_d3d12_shader_bytecode(Bytecode_Ref bytecode)
{
    return { bytecode.data, bytecode.size };
}

inline D3D12_FILTER to_d3d12_filter(Sampler_Filter filter)
{
    switch (filter) {
        case SAMPLER_FILTER_POINT:
            return D3D12_FILTER_MIN_MAG_MIP_POINT;
        case SAMPLER_FILTER_LINEAR:
            return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        case SAMPLER_FILTER_ANISOTROPIC:
            return D3D12_FILTER_ANISOTROPIC;
    }
    assert(false);
    return (D3D12_FILTER)0;
}

inline D3D12_TEXTURE_ADDRESS_MODE to_d3d12_texture_address_mode(Address_Mode address_mode)
{
    switch (address_mode) {
        case ADDRESS_MODE_WRAP:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case ADDRESS_MODE_MIRROR:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case ADDRESS_MODE_CLAMP:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case ADDRESS_MODE_BORDER:
            return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    }
    assert(false);
    return (D3D12_TEXTURE_ADDRESS_MODE)0;
}

inline D3D12_CLEAR_VALUE to_d3d12_clear_value(Clear_Value &clear_value, DXGI_FORMAT format)
{
    D3D12_CLEAR_VALUE d3d12_clear_value;
    ZeroMemory(&d3d12_clear_value, sizeof(D3D12_CLEAR_VALUE));
    d3d12_clear_value.Format = format;
    if (clear_value.type == CLEAR_VALUE_COLOR) {
        clear_value.color.store(d3d12_clear_value.Color);
    } else if (clear_value.type == CLEAR_VALUE_DEPTH_STENCIL) {
        d3d12_clear_value.DepthStencil.Depth = clear_value.depth;
        d3d12_clear_value.DepthStencil.Stencil = clear_value.stencil;
    }
    return d3d12_clear_value;
}
#endif
