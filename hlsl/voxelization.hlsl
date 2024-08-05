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
    
    uint2 pad234;
    float3 voxel_grid_center;
    float1 pad33;
    float4x4 voxel_orthographic_matrix;
    float4x4 voxel_view_matrix;
};

StructuredBuffer<Mesh_Instance> mesh_instances : register(t2);
StructuredBuffer<float4x4> world_matrices : register(t3);
StructuredBuffer<uint> unified_index_buffer : register(t4);
StructuredBuffer<Vertex_XNUV> unified_vertex_buffer : register(t5);

struct Vertex_Out {
	float4 screen_position : SV_POSITION;
	float3 world_position : POSITION0;
	float3 voxel_position : POSITION1;
	float2 uv : TEXCOORD;
};

Vertex_Out vs_main(uint vertex_id : SV_VertexID)
{
    Mesh_Instance mesh_instance = mesh_instances[mesh_id];
	uint index = unified_index_buffer[mesh_instance.index_offset + vertex_id];
	Vertex_XNUV vertex = unified_vertex_buffer[mesh_instance.vertex_offset + index];
	
	float4x4 world_matrix = transpose(world_matrices[world_matrix_id]);

	Vertex_Out vertex_out;
	vertex_out.screen_position = mul(float4(vertex.position, 1.0f), mul(world_matrix, mul(voxel_view_matrix, voxel_orthographic_matrix)));
	vertex_out.world_position = (float3)mul(float4(vertex.position, 1.0f), world_matrix);
	vertex_out.voxel_position = (float3)mul(float4(vertex.position, 1.0f), world_matrix);
	vertex_out.voxel_position -= voxel_grid_center; // Translate to a voxel grid space.
	vertex_out.uv = vertex.uv;
    return vertex_out;
}

struct Voxel {
	uint encoded_color;
	uint occlusion;
};

static uint pack_RGB(float3 color)
{
    uint r = (uint)(255.0f * color.r);
    uint g = (uint)(255.0f * color.g);
    uint b = (uint)(255.0f * color.b);
    return (r << 24) | (g << 16) | (b << 8) | 0xff;
}

RWStructuredBuffer<Voxel> voxels : register(u1);


void ps_main(Vertex_Out vertex_out)
{
    float3 color = diffuse_texture.Sample(linear_sampling, vertex_out.uv).rgb;
    float3 temp = vertex_out.voxel_position / float3(voxel_grid_ceil_width, voxel_grid_ceil_height, voxel_grid_ceil_depth);
    temp = floor(temp);
    
    int3 voxel_grid_size = int3(voxel_grid_width, voxel_grid_height, voxel_grid_depth);
    int3 voxel_grid_index = (int3)((uint3)voxel_grid_size / 2) + (int3)temp;
    
    if (in_range(0, voxel_grid_width - 1, voxel_grid_index.x) && in_range(0, voxel_grid_height - 1, voxel_grid_index.y) && in_range(0, voxel_grid_depth - 1, voxel_grid_index.z)) {
        //int voxel_index = voxel_grid_index.x + voxel_grid_width * (voxel_grid_index.y + voxel_grid_depth * voxel_grid_index.z);
        int voxel_index = voxel_grid_index.x * (voxel_grid_width * voxel_grid_depth) + voxel_grid_index.y * voxel_grid_depth + voxel_grid_index.z;
        InterlockedMax(voxels[voxel_index].encoded_color, pack_RGB(color));
        InterlockedMax(voxels[voxel_index].occlusion, uint(1));
    }
}

#endif