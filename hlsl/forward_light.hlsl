#ifndef __FORWARD_LIGHT__
#define __FORWRAD_LIGHT__

#include "vertex.hlsl"
#include "cbuffer.hlsl"


#define MAX_NUMBER_LIGHT_IN_WORLD 255

#define SPOT_LIGHT_TYPE 0
#define POINT_LIGHT_TYPE 1
#define DIRECTIONAL_LIGHT_TYPE 2


struct Light {
	float4 position;
	float4 direction;
	float4 color;
	float radius;
	float range;
	uint light_type;
	uint shadow_map_idx;
};

struct Shadow_Map {
	uint light_view_matrix_idx;
};

struct Material {
	float4 ambient;
	float4 diffuse;
	float4 specular;
};

float4 calculate_spot_light(Light light, Material material, float3 normal, float3 position)
{
	float shininess = material.specular.w;

	float4 diffuse = { 0.0f, 0.0f, 0.0f, 0.0f };
	float4 specular = { 0.0f, 0.0f, 0.0f, 0.0f };

	float ambient_power = 0.1f;
	float4 ambient = ambient_power * material.ambient * float4(light.color.xyz, 1.0f);
	//float4 ambient = ambient_power * material.ambient * float4(light.color, 1.0f);
	//float4 ambient = calculate_ambient_light(normal, light.color);

	float3 to_light = normalize(light.position - position);

	float theta = dot(-to_light, light.direction);
	float x = cos(radians(light.radius));
	
	if (theta > x) {
		return float4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	normal = normalize(normal);

	float diffuse_factor = max(dot(-light.direction, normal), 0.0f);

	if (diffuse_factor > 0.0f) {
		float3 reflect_dir = normalize(reflect(light.direction, normal));
		float3 dir_to_camera = normalize(camera_position - position);
		
		float specular_factor = pow(max(dot(reflect_dir, dir_to_camera), 0.0f), shininess);
		
		diffuse = diffuse_factor * material.diffuse * light.color;
		specular = specular_factor * material.specular * light.color;
	}

	return specular + diffuse + ambient;
}

float4 calculate_point_light(Light light, Material material, float3 normal, float3 world_position)
{
	float shininess = material.specular.w;
	float distance = length(light.position - world_position);

	if (distance > light.range)
		return float4(0.0f, 0.0f, 0.0f, 0.0f);
	
	float3 to_light = normalize((float3)light.position - world_position);

	float4 diffuse = { 0.0f, 0.0f, 0.0f, 0.0f };
	float4 specular = { 0.0f, 0.0f, 0.0f, 0.0f };

	float ambient_power = 0.1f;
	float4 ambient = ambient_power * material.ambient * light.color;
	//float4 ambient = calculate_ambient_light(normal, light.color);

	normal = normalize(normal);

	float diffuse_factor = max(dot(to_light, normal), 0.0f);

	if (diffuse_factor > 0.0f) {

		float3 reflect_dir = normalize(reflect(-to_light, normal));

		float3 dir_to_camera = normalize(camera_position - world_position);
		float specular_factor = pow(max(dot(reflect_dir, dir_to_camera), 0.0f), shininess);
		
		diffuse = diffuse_factor * material.diffuse * light.color, 1.0f;
		specular = specular_factor * material.specular * light.color, 1.0f;

		float x = 1.0f - saturate(distance * ( 1.0f / light.range));
		float attenuation = x * x;

		ambient *= attenuation;
		diffuse *= attenuation;
		specular *= attenuation;
	}

	return specular + diffuse + ambient;
}

float4 calculate_directional_light(Light light, Material material, float3 normal, float3 world_position)
{
	float shininess = material.specular.w;

	float4 diffuse = { 0.0f, 0.0f, 0.0f, 0.0f };
	float4 specular = { 0.0f, 0.0f, 0.0f, 0.0f };

	float ambient_power = 0.1f;
	float4 ambient = ambient_power * material.ambient * light.color;
	//float4 ambient = calculate_ambient_light(normal, light.color);

	normal = normalize(normal);

	float diffuse_factor = max(dot(-light.direction, normal), 0.0f);

	if (diffuse_factor > 0.0f) {
		diffuse = diffuse_factor * material.diffuse * light.color;

		float3 reflect_dir = normalize(reflect(-light.direction, normal));

		float3 dir_to_camera = normalize(camera_position - world_position);
		float specular_factor = pow(max(dot(reflect_dir, dir_to_camera), 0.0f), shininess);
		specular = specular_factor * material.specular * light.color;
	}

	return specular + diffuse + ambient;
}

cbuffer Pass_Data : register(b2) {
	uint mesh_id;
	uint world_matrix_id;
	uint pad11;
	uint pad22;
}

struct Vertex_Out {
	float4 position : SV_POSITION;
	float3 world_position : POSITION;
	float3 normal : NORMAL;
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
	vertex_out.world_position = mul(float4(vertex.position, 1.0f), world_matrix);
	vertex_out.normal = normalize(mul(vertex.normal, (float3x3)world_matrix));
	vertex_out.uv = vertex.uv;
	return vertex_out;
}

StructuredBuffer<Light> lights : register(t7);
StructuredBuffer<Shadow_Map> shadow_maps : register(ps, t6);
StructuredBuffer<float4x4> light_view_matrices : register(ps, t8);

static const uint value = 4;
float2 poissonDisk[value] = {
	float2( -0.94201624, -0.39906216 ),
	float2( 0.94558609, -0.76890725 ),
  	float2( -0.094184101, -0.92938870 ),
  	float2( 0.34495938, 0.29387760 )
};

float calculate_shadow(Light light, float3 normal, float3 world_position, uint light_view_matrix_idx)
{
	float4x4 light_view_matrix = transpose(light_view_matrices[light_view_matrix_idx]);
	
	float4 position_from_light_perspective = mul(float4(world_position, 1.0f), mul(light_view_matrix, direction_light_matrix));

	float2 NDC_coordinates;
	
	NDC_coordinates.x = 0.5f + ((position_from_light_perspective.x / position_from_light_perspective.w) * 0.5f);
	NDC_coordinates.y = 0.5f - ((position_from_light_perspective.y / position_from_light_perspective.w) * 0.5f);

	float currect_depth = position_from_light_perspective.z /  position_from_light_perspective.w;
	
	float bias = max(0.05f * (1.0f - dot(-light.direction, normal)), 0.0005f);

//	for(uint i = 0; i < 4; i++) {
		//float2 temp = float2(NDC_coordinates.x + poissonDisk[i] / 700.0f,NDC_coordinates.y + poissonDisk[i] / 700.0f);
//		float shadow_map_depth = shadow_atlas.Sample(sampler_anisotropic, NDC_coordinates.xy + poissonDisk[i] / 700.0f);
//		if ((currect_depth - bias)  > shadow_map_depth) {
//			shadow_factor -= 0.25f;
//		}
//	}


	float2 texture_size;
	shadow_atlas.GetDimensions(texture_size.x, texture_size.y);

	const float2 texel_size = { 1.0f / texture_size.x, 1.0f / texture_size.y };

	float shadow_factor = 0.0f;
	for(int x = 0; x < 1; x++) {
		for(int y = 0; y < 1; y++) {
			float2 offset = float2(x, y) * texel_size;
			float shadow_map_depth = shadow_atlas.Sample(sampler_anisotropic, NDC_coordinates.xy + offset);
			if ((currect_depth - bias)  < shadow_map_depth) {
				shadow_factor += 1.0f;
			}
		}
	}
	return (shadow_factor / 1.0f);
}


float4 ps_main(Vertex_Out vertex_out) : SV_Target
{
	float4 test = texture_map.Sample(sampler_anisotropic, vertex_out.uv);

	if (light_count == 0) {
		return test;
	}

	Material material;
	material.ambient = float4(1.0f, 1.0f, 1.0f, 1.0f);
	material.diffuse = float4(1.0f, 1.0f, 1.0f, 1.0f);
	material.specular = float4(1.0f, 1.0f, 1.0f, 1.0f);

	float4 final_color = { 0.0f, 0.0f, 0.0f, 0.0f };
	for (uint i = 0; i < light_count; i++) {

		Light light = lights[i];
		Shadow_Map shadow_map = shadow_maps[light.shadow_map_idx];

		float shadow_factor = calculate_shadow(light, vertex_out.normal, vertex_out.world_position, shadow_map.light_view_matrix_idx);
		if (shadow_factor > 0.0f) {
			switch (light.light_type) {
				case SPOT_LIGHT_TYPE:
					final_color += calculate_spot_light(light, material, vertex_out.normal, vertex_out.world_position);
					break;
				case POINT_LIGHT_TYPE:
					final_color += calculate_point_light(light, material, vertex_out.normal, vertex_out.world_position);
					break;
				case DIRECTIONAL_LIGHT_TYPE:
					final_color += calculate_directional_light(light, material, vertex_out.normal, vertex_out.world_position);
					break;
			}
		}
		final_color *= shadow_factor;
	}

	float4 r = final_color * test;
	r.a = material.diffuse.a * test.a;
	return r;
}

#endif