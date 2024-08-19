#ifndef __WORLD_VOXELIZATION__
#define __WORLD_VOXELIZATION__

#include "utils.hlsl"
#include "globals.hlsl"
#include "vertex.hlsl"

cbuffer Pass_Data : register(b0) {
	uint mesh_id;
	uint world_matrix_id;
	uint pad11;
	uint pad22;
}

cbuffer Voxelization_Info : register(b1) {
    uint voxel_grid_width;  // Holds a number of voxels in a voxel grid by x axis.
    uint voxel_grid_height; // Holds a number of voxels in a voxel grid by y axis.
    uint voxel_grid_depth;  // Holds a number of voxels in a voxel grid by z axis.
    
    uint voxel_grid_ceil_width;
    uint voxel_grid_ceil_height;
    uint voxel_grid_ceil_depth;
    float2 texel_size;
    
    float3 voxel_grid_center;
    float1 pad33;
    
    float4x4 voxel_orthographic_matrix;
    float4x4 voxel_view_matrices[3];
};

struct Voxel {
    uint packed_color;
	uint packed_normal;
	uint occlusion;
};

struct VS_Output {
	float3 world_position : POSITION0;
	float3 voxel_position : POSITION1;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct GS_Output {
	float4 screen_position : SV_POSITION;
	float3 voxel_position : POSITION1;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

StructuredBuffer<Mesh_Instance> mesh_instances : register(t2);
StructuredBuffer<float4x4> world_matrices : register(t3);
StructuredBuffer<uint> unified_index_buffer : register(t4);
StructuredBuffer<Vertex_XNUV> unified_vertex_buffer : register(t5);
RWStructuredBuffer<Voxel> voxels : register(u1);

static uint pack_RGBA8(float4 value)
{
    return (uint(value.r) << 24) | (uint(value.g) << 16) | (uint(value.b) << 8) | uint(value.a);
}

static uint pack_RGB8(float3 value)
{
    return (uint(value.r) << 24) | (uint(value.g) << 16) | (uint(value.b) << 8);
}

static float4 unpack_RGBA8(uint value)
{
    return float4(float(value & 0xff000000), float(value & 0x00ff0000), float(value & 0x0000ff00), float(value & 0x000000ff));
}

static uint get_view_matrix_index(float3 normal)
{
    float3x3 direction_matrix;
    direction_matrix[0] = float3(1.0f, 0.0f, 0.0f);
    direction_matrix[1] = float3(0.0f, 1.0f, 0.0f);
    direction_matrix[2] = float3(0.0f, 0.0f, 1.0f);
    float3 values = abs(mul(normal, direction_matrix));
    float maximum = max(values.x, max(values.y, values.z));
    float index;
    if (maximum == values.x) {
        index = 0;
    } else if (maximum == values.y) {
        index = 1;
    } else {
        index = 2;
    }
    return index;
}

VS_Output vs_main(uint vertex_id : SV_VertexID)
{
    Mesh_Instance mesh_instance = mesh_instances[mesh_id];
	uint index = unified_index_buffer[mesh_instance.index_offset + vertex_id];
	Vertex_XNUV vertex = unified_vertex_buffer[mesh_instance.vertex_offset + index];
	
	float4x4 world_matrix = transpose(world_matrices[world_matrix_id]);

	VS_Output output;
	output.world_position = (float3)mul(float4(vertex.position, 1.0f), world_matrix);
	output.voxel_position = output.world_position;
	output.voxel_position -= voxel_grid_center; // Translate to a voxel grid space.
	output.normal = normalize(mul(vertex.normal, (float3x3)world_matrix));
	output.uv = vertex.uv;
    return output;
}

[maxvertexcount(3)]
void gs_main( triangle VS_Output input[3], inout TriangleStream<GS_Output> output_stream )
{
    uint index = get_view_matrix_index(input[0].normal + input[1].normal + input[2].normal);
    GS_Output output[3];
    [unroll]
    for (uint i = 0; i < 3; i++) {
        output[i].screen_position =  mul(float4(input[i].world_position, 1.0f), mul(voxel_view_matrices[index], voxel_orthographic_matrix));
    	output[i].voxel_position = input[i].voxel_position;
    	output[i].normal = input[i].normal;
    	output[i].uv = input[i].uv;
    }    
    float2 side0 = normalize(output[1].screen_position.xy - output[0].screen_position.xy);
    float2 side1 = normalize(output[2].screen_position.xy - output[1].screen_position.xy);
    float2 side2 = normalize(output[0].screen_position.xy - output[2].screen_position.xy);
    output[0].screen_position.xy += normalize(-side0 + side2) * texel_size;
    output[1].screen_position.xy += normalize(side0 + side1) * texel_size;
    output[2].screen_position.xy += normalize(side1 + side2) * texel_size;
    [unroll]
    for (uint j = 0; j < 3; j++) {
        output_stream.Append(output[j]);
    }
    output_stream.RestartStrip();
}

static float3 normalize_RGB(float3 value)
{
    return value / 255.0f;
}

static float3 denormalize_RGB(float3 value)
{
    return value * 255.0f;
}

void write_voxel_data(uint voxel_index, float3 color)
{
    uint old_packed_color = voxels[voxel_index].packed_color;
    uint new_packed_color = 0;
    uint current_packed_color;

    [allow_uav_condition] 
    while (old_packed_color != current_packed_color) {
        old_packed_color = voxels[voxel_index].packed_color;
        
        float3 old_color = unpack_RGBA8(old_packed_color).xyz;
        float color_count = unpack_RGBA8(old_packed_color).w + 1.0f;
        float3 new_color = (normalize_RGB(old_color) + color) / color_count;
        new_packed_color = pack_RGBA8(float4(denormalize_RGB(new_color), color_count));

        InterlockedCompareExchange(voxels[voxel_index].packed_color, old_packed_color, new_packed_color, current_packed_color);
    }
}

void ps_main(GS_Output input)
{
    float3 color = diffuse_texture.Sample(linear_sampling, input.uv).rgb;
    float3 temp = input.voxel_position / float3(voxel_grid_ceil_width, voxel_grid_ceil_height, voxel_grid_ceil_depth);
    temp = floor(temp);
    
    int3 voxel_grid_size = int3(voxel_grid_width, voxel_grid_height, voxel_grid_depth);
    int3 voxel_grid_index = (int3)((uint3)voxel_grid_size / 2) + (int3)temp;
    
    if (in_range(0, voxel_grid_width - 1, voxel_grid_index.x) && in_range(0, voxel_grid_height - 1, voxel_grid_index.y) && in_range(0, voxel_grid_depth - 1, voxel_grid_index.z)) {
        int voxel_index = voxel_grid_index.x * (voxel_grid_width * voxel_grid_depth) + voxel_grid_index.y * voxel_grid_depth + voxel_grid_index.z;

        //write_voxel_data(voxel_index, color);
        InterlockedMax(voxels[voxel_index].packed_color, pack_RGB8(color * 255.0f));
        InterlockedMax(voxels[voxel_index].packed_normal, pack_RGB8(input.normal));
        InterlockedMax(voxels[voxel_index].occlusion, uint(1));
    }
}

#endif