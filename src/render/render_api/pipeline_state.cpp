#include <assert.h>

#include "pipeline_state.h"
#include "../shader_manager.h"
#include "../render_helpers.h"
#include "../../sys/utils.h"

static D3D12_PRIMITIVE_TOPOLOGY_TYPE to_d3d12_primitive_topology_type(Primitive_Type &primitive_type)
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

static D3D12_BLEND to_d3d12_blend_option(Blend_Option blend_option)
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

static D3D12_BLEND_OP to_d3d12_blend_operator(Blend_Operation blend_operation)
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

static D3D12_FILL_MODE to_d3d12_fill_mode(Fill_Type fill_type)
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

static D3D12_CULL_MODE to_d3d12_cull_mode(Cull_Type cull_type)
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

static D3D12_STENCIL_OP to_d3d12_stencil_op(Stencil_Operation stencil_operation)
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

static D3D12_COMPARISON_FUNC to_d3d12_comparison_func(Comparison_Func func)
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

static D3D12_DEPTH_WRITE_MASK to_d3d12_depth_write_mask(Depth_Write depth_write)
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

static D3D12_BLEND_DESC to_d3d12_blend_desc(u32 render_target_count, Blending_Desc blending_desc)
{
    assert(render_target_count > 0);
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

static D3D12_RASTERIZER_DESC to_d3d12_rasterizer_desc(Rasterization_Desc rasterization_desc)
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

static D3D12_DEPTH_STENCIL_DESC to_d3d12_depth_stencil_desc(Depth_Stencil_Desc depth_stencil_desc)
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

Blending_Desc::Blending_Desc()
{
    enable = false;
    src = BLEND_ONE;
    dest = BLEND_ZERO;
    blend_op = BLEND_OP_ADD;
    src_alpha = BLEND_ONE;
    dest_alpha = BLEND_ZERO;
    blend_op_alpha = BLEND_OP_ADD;
}

Blending_Desc::~Blending_Desc()
{
}

Rasterization_Desc::Rasterization_Desc()
{
    fill_type = FILL_TYPE_SOLID;
    cull_type = CULL_TYPE_BACK;
}

Rasterization_Desc::~Rasterization_Desc()
{
}

Depth_Stencil_Desc::Depth_Stencil_Desc()
{
    enable_depth_test = true;
    enable_stencil_test = false;
    stencil_read_mask = 0xff;
    stencil_write_mask = 0xff;
    depth_write = DEPTH_WRITE_ALL;
    depth_compare_func = COMPARISON_LESS;
    stencil_compare_func = COMPARISON_ALWAYS;
    stencil_failed = STENCIL_OP_KEEP;
    stencil_passed_depth_failed = STENCIL_OP_KEEP;
    stencil_and_depth_passed = STENCIL_OP_KEEP;
}

Depth_Stencil_Desc::~Depth_Stencil_Desc()
{
}

void Render_Pipeline_Desc::add_render_target(DXGI_FORMAT format)
{
    render_targets_formats.push(format);
}

void Render_Pipeline_Desc::add_layout(const char *semantic_name, DXGI_FORMAT format)
{
    input_elements.push({ semantic_name, 0, format, 0, layout_offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    layout_offset += dxgi_format_size(format);
}

D3D12_INPUT_LAYOUT_DESC Render_Pipeline_Desc::d3d12_input_layout()
{
    return { input_elements.items, input_elements.count };
}

Pipeline_State::Pipeline_State()
{
}

Pipeline_State::~Pipeline_State()
{
}

void Pipeline_State::create(Gpu_Device &device, Render_Pipeline_Desc &render_pipeline_desc)
{
    assert(render_pipeline_desc.render_targets_formats.count > 0);

    primitive_type = render_pipeline_desc.primitive_type;
    root_signature = render_pipeline_desc.root_signature;
    viewport = render_pipeline_desc.viewport;
    if ((render_pipeline_desc.clip_rect.width == 0) && (render_pipeline_desc.clip_rect.height == 0)) {
        clip_rect.x = static_cast<u32>(render_pipeline_desc.viewport.x);
        clip_rect.y = static_cast<u32>(render_pipeline_desc.viewport.y);
        clip_rect.width = static_cast<u32>(render_pipeline_desc.viewport.width);
        clip_rect.height = static_cast<u32>(render_pipeline_desc.viewport.height);
    } else {
        clip_rect = render_pipeline_desc.clip_rect;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphics_pipeline_state;
    ZeroMemory(&graphics_pipeline_state, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    graphics_pipeline_state.pRootSignature= render_pipeline_desc.root_signature->get();
    graphics_pipeline_state.VS = render_pipeline_desc.vertex_shader->vs_bytecode.d3d12_shader_bytecode();
    graphics_pipeline_state.PS = render_pipeline_desc.vertex_shader->ps_bytecode.d3d12_shader_bytecode();
    graphics_pipeline_state.InputLayout = render_pipeline_desc.d3d12_input_layout();

    graphics_pipeline_state.BlendState = to_d3d12_blend_desc(render_pipeline_desc.render_targets_formats.count, render_pipeline_desc.blending_desc);
    graphics_pipeline_state.RasterizerState = to_d3d12_rasterizer_desc(render_pipeline_desc.rasterization_desc);
    graphics_pipeline_state.DepthStencilState = to_d3d12_depth_stencil_desc(render_pipeline_desc.depth_stencil_desc);
    graphics_pipeline_state.SampleMask = UINT32_MAX;
    graphics_pipeline_state.PrimitiveTopologyType = to_d3d12_primitive_topology_type(render_pipeline_desc.primitive_type);
    graphics_pipeline_state.NumRenderTargets = render_pipeline_desc.render_targets_formats.count;
    for (u32 i = 0; i < render_pipeline_desc.render_targets_formats.count; i++) {
        graphics_pipeline_state.RTVFormats[i] = render_pipeline_desc.render_targets_formats[i];
    }
    graphics_pipeline_state.DSVFormat = render_pipeline_desc.depth_stencil_format;
    graphics_pipeline_state.SampleDesc.Count = 1;

    HR(device->CreateGraphicsPipelineState(&graphics_pipeline_state, IID_PPV_ARGS(release_and_get_address())));
}
