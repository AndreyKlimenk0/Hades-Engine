#include "pipeline_state.h"

D12_Rasterizer_Desc::D12_Rasterizer_Desc()
{
    ZeroMemory(&d3d12_rasterizer_desc, sizeof(D3D12_RASTERIZER_DESC));
    d3d12_rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
    d3d12_rasterizer_desc.CullMode = D3D12_CULL_MODE_BACK;
    d3d12_rasterizer_desc.FrontCounterClockwise = FALSE;
    d3d12_rasterizer_desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    d3d12_rasterizer_desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    d3d12_rasterizer_desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    d3d12_rasterizer_desc.DepthClipEnable = TRUE;
    d3d12_rasterizer_desc.MultisampleEnable = FALSE;
    d3d12_rasterizer_desc.AntialiasedLineEnable = FALSE;
    d3d12_rasterizer_desc.ForcedSampleCount = 0;
    d3d12_rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}

D12_Rasterizer_Desc::~D12_Rasterizer_Desc()
{
}

D12_Blend_Desc::D12_Blend_Desc()
{
    ZeroMemory(&d3d12_blend_desc, sizeof(D3D12_BLEND_DESC));
    d3d12_blend_desc.AlphaToCoverageEnable = false;
    d3d12_blend_desc.IndependentBlendEnable = false;
    d3d12_blend_desc.RenderTarget[0].BlendEnable = false;
    d3d12_blend_desc.RenderTarget[0].LogicOpEnable = false;
    d3d12_blend_desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    d3d12_blend_desc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
    d3d12_blend_desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    d3d12_blend_desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    d3d12_blend_desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    d3d12_blend_desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    d3d12_blend_desc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    d3d12_blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

}

D12_Blend_Desc::~D12_Blend_Desc()
{
}
