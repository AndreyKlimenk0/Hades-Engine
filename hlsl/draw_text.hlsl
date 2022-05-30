#ifndef __DRAW_TEXT__
#define __DRAW_TEXT__

#include "vertex.hlsl"
#include "cbuffer.hlsl"


cbuffer Pass_Data : register(b1) {
    float4x4 text_matrix;
}

Vertex_XUV_Out vs_main(Vertex_XUV_In vertex)
{
    Vertex_XUV_Out result;
    result.position = mul(float4(vertex.position.xy, 0.0f, 1.0f), text_matrix);
    result.position.z = 0.0f;
    result.position.w = 1.0f;
    result.uv = vertex.uv;
    return result;  
}

float4 ps_main(Vertex_XUV_Out pixel) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, texture_map.Sample(sampler_anisotropic, pixel.uv).r);
}

#endif