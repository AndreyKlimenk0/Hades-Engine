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

float2 parallax_mapping(float2 uv, float3 camera_direction, float3x3 TBN_matrix)
{ 
    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = lerp(maxLayers, minLayers, abs(dot(float3(0.0, 0.0, 1.0), camera_direction)));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    float heightScale = 0.1f;
    float2 P = camera_direction.xy / camera_direction.z * heightScale; 
    float2 deltaTexCoords = P / numLayers;
  
    // get initial values
    float2  currentTexCoords = uv;
    float currentDepthMapValue = displacement_texture.SampleLevel(linear_sampling, currentTexCoords, 0).r;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = displacement_texture.SampleLevel(linear_sampling, currentTexCoords, 0).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    float2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = displacement_texture.SampleLevel(linear_sampling, prevTexCoords, 0).r - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    float2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}


float4 ps_main(Vertex_Out vertex_out) : SV_Target
{
    float3x3 TBN_matrix = get_TBN_matrix(vertex_out.tangent, vertex_out.normal);
    float3 tanget_space_camera_direction = normalize(mul(camera_direction, transpose(TBN_matrix)));
    
    float3x3 t = get_TBN_matrix(vertex_out.tangent, vertex_out.normal);
    //float3 dir = normalize(camera_direction - camera_position);
    float3 p = mul(camera_position, transpose(t));
    float3 p1 = mul(vertex_out.world_position, transpose(t));
    float3 dir = normalize(p - p1);
    //dir = mul(dir, transpose(t));
    //dir = normalize(dir);
    //vertex_out.uv = parallax_mapping(vertex_out.uv, dir, TBN_matrix);
    
    float3 normal_sample = normal_texture.Sample(linear_sampling, vertex_out.uv).rgb;
    
    Material material;
    material.normal = normal_mapping(normal_sample, vertex_out.normal, vertex_out.tangent);
    material.diffuse = diffuse_texture.Sample(linear_sampling, vertex_out.uv).rgb;
    material.specular = specular_texture.Sample(linear_sampling, vertex_out.uv).rgb;

    //@Note: hard code
	if (light_count == 0) {
		return float4(material.diffuse, 1.0f);
	}
    uint shadow_cascade_index;
    float4 shadow_factor = calculate_shadow_factor(vertex_out.world_position, vertex_out.position.xy, material.normal, shadow_cascade_index);    
    float3 light_factor = calculate_light(vertex_out.world_position, material, light_count, lights);

    return float4(light_factor, 1.0f) * shadow_factor;
}

#endif