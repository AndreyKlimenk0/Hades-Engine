#ifndef __BACK_BUFFER_OUTPUT__
#define __BACK_BUFFER_OUTPUT__

#include "globals.hlsl"
#include "full_screen_triangle.hlsl"

struct Pass_Data {
    uint texture_idx;
    uint3 pad;
};

ConstantBuffer<Pass_Data> pass_data : register(b0, space0);

float4 ps_main(Vertex_Out vertex_out) : SV_TARGET
{
    Texture2D<float4> final_texture = ResourceDescriptorHeap[pass_data.texture_idx];
    float3 final_color = final_texture.Sample(point_sampler(), vertex_out.uv).rgb;
    return float4(final_color, 1.0f);
}

#endif