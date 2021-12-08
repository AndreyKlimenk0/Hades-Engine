#include "global.fx"


struct Vertex_In {
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct Vertex_Out {
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

Vertex_Out vs_main(Vertex_In vertex)
{
	Vertex_Out result;
	result.position = mul(float4(vertex.position, 1.0f), world_view_projection);
	result.normal = vertex.normal;
	result.uv = vertex.uv;
	return result;
}

float4 ps_main(Vertex_Out pixel) : SV_Target
{
	return texture_map.Sample(sampler_anisotropic, pixel.uv);
}

Vertex_Out vs_draw_outlining(Vertex_In vertex)
{
	Vertex_Out result;
	result.position = mul(float4(vertex.position + vertex.normal * 4.0, 1.0f), world_view_projection);
	result.normal = vertex.normal;
	result.uv = vertex.uv;
	return result;
}

float4 ps_draw_outlining(Vertex_Out pixel) : SV_Target
{
	return normalize_rgb(201, 131, 50);
}

Vertex_Out vs_draw_texture(Vertex_In vertex)
{
	Vertex_Out result;
	result.position = mul(float4(vertex.position, 1.0f), world_view_projection);
	result.normal = vertex.normal;
	result.uv = vertex.uv;
	return result;
}

float4 ps_draw_texture(Vertex_Out pixel) : SV_Target
{
	return texture_map.Sample(sampler_anisotropic, pixel.uv);
}


technique11 ColorTech {
	pass P0 {
		SetVertexShader(CompileShader(vs_5_0, vs_main()));
		SetPixelShader(CompileShader(ps_5_0, ps_main()));
	}
}

technique11 draw_outlining {
	pass P0 {
		SetVertexShader(CompileShader(vs_5_0, vs_draw_outlining()));
		SetPixelShader(CompileShader(ps_5_0, ps_draw_outlining()));
	}
}

technique11 draw_texture {
	pass P0 {
		SetVertexShader(CompileShader(vs_5_0, vs_draw_texture()));
		SetPixelShader(CompileShader(ps_5_0, ps_draw_texture()));
	}
}