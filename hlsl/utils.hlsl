#ifndef __UTILS__
#define __UTILS__

struct Material {
	float4 ambient;
	float4 diffuse;
	float4 specular;
};

//@Note: Temp function
Material get_material()
{
    static Material default_material;
    default_material.ambient = float4(1.0f, 1.0f, 1.0f, 1.0f);
    default_material.diffuse = float4(1.0f, 1.0f, 1.0f, 1.0f);
    default_material.specular = float4(1.0f, 1.0f, 1.0f, 1.0f);
    return default_material;
}

float4 normalize_rgb(int r, int g, int b)
{
	return float4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
}

float2 calculate_ndc_coordinates(float4 transformed_vertex_position)
{
    float2 ndc_coordinates;
    ndc_coordinates.x = 0.5f + ((transformed_vertex_position.x / transformed_vertex_position.w) * 0.5f);
	ndc_coordinates.y = 0.5f - ((transformed_vertex_position.y / transformed_vertex_position.w) * 0.5f);
	return ndc_coordinates;
}

#endif