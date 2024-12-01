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


struct Vertex_Input
{
    float3 position : POSITION;
    float3 color    : COLOR;
};

struct Vertex_Output
{
    float4 position : SV_POSITION;
    float3 color    : COLOR;
};

Vertex_Output vs_main(Vertex_Input input)
{
    float4x4 wvp_matrix = mul(mul(world_matrix, view_matrix), perspective_matrix);

    Vertex_Output output;
    output.position = mul(float4(input.position, 1.0f), wvp_matrix);
    output.color = input.color;
    return output;
}

float4 ps_main(Vertex_Output output) : SV_TARGET
{
    return float4(output.color, 1.0f);
}

#endif