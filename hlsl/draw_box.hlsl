#ifndef __DRAW_BOX__

cbuffer World_Matrix : register(b1, space0) {
    float4x4 world_matrix;
};

cbuffer View_Matrix : register(b2, space0) {
    float4x4 view_matrix;
};

cbuffer Perspective_Matrix : register(b3, space0) {
    float4x4 perspective_matrix;
};

Texture2D<float4> texture_map : register(t0, space0);
SamplerState linear_sampling : register(s0, space0);

struct Vertex_Input
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct Vertex_Output
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

Vertex_Output vs_main(Vertex_Input input)
{
    float4x4 wvp_matrix = mul(mul(world_matrix, view_matrix), perspective_matrix);

    Vertex_Output output;
    output.position = mul(float4(input.position, 1.0f), wvp_matrix);
    output.uv = input.uv;
    return output;
}

float4 ps_main(Vertex_Output output) : SV_TARGET
{
    float4 texel = texture_map.Sample(linear_sampling, output.uv);
    return texel;
}

#endif