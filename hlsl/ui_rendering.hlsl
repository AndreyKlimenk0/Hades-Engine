#ifndef __UI_RENDERING__
#define __UI_RENDERING__

#include "globals.hlsl"
#include "utils.hlsl"

struct UI_Data
{
    float4x4 projection_matrix;
    uint ui_texture_index;
    uint pad3;
};

struct ImGui_Vertex
{
    float2 position;
    float2 uv;
    uint color; // ImGui packs 4-comp colors in U32
};

struct Pass_Data
{
    uint vertex_offset;
    uint index_offset;
    int texture_idx;
    int sample_idx;
};

ConstantBuffer<Pass_Data> pass_data : register(b0, space0);
ConstantBuffer<UI_Data> ui_data : register(b1, space0);

StructuredBuffer<ImGui_Vertex> unified_vertex_buffer : register(t0, space0);
StructuredBuffer<uint> unified_index_buffer : register(t1, space0);

struct PS_Input
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv : TEXCOORD0;
};

PS_Input vs_main(uint index_id : SV_VertexID)
{
    uint index = unified_index_buffer[index_id + pass_data.index_offset];
    ImGui_Vertex vertex = unified_vertex_buffer[index + pass_data.vertex_offset];

    PS_Input output;
    output.pos = mul(float4(vertex.position, frame_info.near_plane, 1.0f), ui_data.projection_matrix);
    output.col = unpack_rgba(vertex.color).abgr;
    output.uv = vertex.uv;
    return output;
};

float4 ps_main(PS_Input input) : SV_TARGET
{
    float4 color = input.col;

    // if (pass_data.texture_idx >= 0)
    // {
    //     Texture2D uiTexture = textures[pass_data.texture_idx];
    //     color = uiTexture.Sample(point_sampler(), input.uv);
    //     color.a = 1.0;
    // }
    // else
    // {
    //     Texture2D uiTexture = textures[ui_data.ui_texture_index];
    //     color = uiTexture.Sample(anisotropic_sampler(), input.uv).aaaa * input.col;
    // }
    Texture2D uiTexture = textures[pass_data.texture_idx];
    color = uiTexture.Sample(linear_sampler(), input.uv).aaaa * input.col;
    return color;
};
#endif