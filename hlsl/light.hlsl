#ifndef __LIGHT__
#define __LIGHT__

#include "utils.hlsl"
#include "globals.hlsl"

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

float4 calculate_light(float3 world_position, float3 normal, Material material, StructuredBuffer<Light> lights, uint light_count)
{
    float4 light_factor = { 0.0f, 0.0f, 0.0f, 0.0f };
    for (uint i = 0; i < light_count; i++) {
		Light light = lights[i];
		switch (light.light_type) {
			case SPOT_LIGHT_TYPE:
				light_factor += calculate_spot_light(light, material, normal, world_position);
				break;
			case POINT_LIGHT_TYPE:
				light_factor += calculate_point_light(light, material, normal, world_position);
				break;
			case DIRECTIONAL_LIGHT_TYPE:
				light_factor += calculate_directional_light(light, material, normal, world_position);
				break;
		}
	}
	return light_factor;
}

#endif