#ifndef __FORWARD_LIGHT__
#define __FORWRAD_LIGHT__

#include "light.hlsl"
#include "vertex.hlsl"
#include "globals.hlsl"
#include "shadows.hlsl"

cbuffer Pass_Data : register(b0) {
	uint mesh_id;
	uint world_matrix_id;
	uint pad11;
	uint pad22;
}

struct Vertex_Out {
	float4 position : SV_POSITION;
	float3 world_position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : TEXCOORD;
};

StructuredBuffer<Mesh_Instance> mesh_instances : register(t2);
StructuredBuffer<float4x4> world_matrices : register(t3);
StructuredBuffer<uint> unified_index_buffer : register(t4);
StructuredBuffer<Vertex_XNUV> unified_vertex_buffer : register(t5);

Vertex_Out vs_main(uint vertex_id : SV_VertexID)
{
	Mesh_Instance mesh_instance = mesh_instances[mesh_id];
	
	uint index = unified_index_buffer[mesh_instance.index_offset + vertex_id];
	Vertex_XNUV vertex = unified_vertex_buffer[mesh_instance.vertex_offset + index];

	float4x4 world_matrix = transpose(world_matrices[world_matrix_id]);
	
	Vertex_Out vertex_out;
	vertex_out.position = mul(float4(vertex.position, 1.0f), mul(world_matrix, mul(view_matrix, perspective_matrix))); 
	vertex_out.world_position = mul(float4(vertex.position, 1.0f), world_matrix).xyz;
	vertex_out.normal = mul(vertex.normal, (float3x3)world_matrix);
	vertex_out.tangent = mul(vertex.tangent, (float3x3)world_matrix);
	vertex_out.uv = vertex.uv;
	return vertex_out;
}

StructuredBuffer<Light> lights : register(t7);

float2 quadtree_displacement_mapping(Texture2D displacement_texture, float2 uv, float3 camera_direction)
{
    float2 offset = camera_direction.xy / camera_direction.z * 0.1f;

    uint2 texture_size;
    displacement_texture.GetDimensions(texture_size.x, texture_size.y);
    
    uint max_texture_size = max(texture_size.x texture_size.y);
    uint max_mip_map_level = (uint)floor(log2(max_texture_size));
    uint mip_map_number = max_mip_map_level;
    
    uint mip_map_level = max_mip_map_level;
    while (mip_map_level >= 0) {
        
        float depth = displacement_texture.Sample(point_sampling, uv + offset, mip_map_level);
        
        mip_map_level -= 1;
    }
}

float4 ps_main(Vertex_Out vertex_out) : SV_Target
{
    float3x3 TBN_matrix = get_TBN_matrix(vertex_out.tangent, vertex_out.normal);
    float3 tanget_space_camera_direction = normalize(camera_direction * transpose(TBN_matrix));
    
    Material material;
    material.ambient = ambient_texture.Sample(linear_sampling, vertex_out.uv).rgb;
    material.diffuse = diffuse_texture.Sample(linear_sampling, vertex_out.uv).rgb;
    material.specular = specular_texture.Sample(linear_sampling, vertex_out.uv).rgb;
    
    float3 normal_sample = normal_texture.Sample(linear_sampling, vertex_out.uv).rgb;
    float3 normal = normal_mapping(normal_sample, vertex_out.normal, vertex_out.tangent);

    //@Note: hard code
	if (light_count == 0) {
		return float4(material.diffuse, 1.0f);
	}
    uint shadow_cascade_index;
    float4 shadow_factor = calculate_shadow_factor(vertex_out.world_position, vertex_out.position.xy, normal, shadow_cascade_index);    
    float3 light_factor = calculate_light(vertex_out.world_position, normal, material, light_count, lights);

    return float4(light_factor, 1.0f) * shadow_factor;
}

#endif