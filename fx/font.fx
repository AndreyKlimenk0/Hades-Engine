#include "global.fx"

cbuffer font_matrix {
	float4x4 orthogonal_matrix;
	float4x4 projection_matrix;
};


struct Vertex_In {
	float3 position : POSITION;
	float2 uv : TEXCOORD;
};

struct Vertex_Out {
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

Vertex_Out vs_main(Vertex_In vertex)
{
	Vertex_Out result;
	result.position = mul(float4(vertex.position.xy, 0.0f, 1.0f), orthogonal_matrix);
	//result.position = mul(float4(vertex.position.xy, 0.0f, 1.0f), projection_matrix);
	result.uv = vertex.uv;
	return result;
}

float4 ps_main(Vertex_Out pixel) : SV_Target
{
	//return float4(1.0, 1.0f, 1.0f, texture_map.Sample(sampler_anisotropic, pixel.uv).r);
	return float4(0.2f, 1.0f, 1.0f, 1.0f);
}

technique11 FOnt {
	pass P0 {
		SetVertexShader(CompileShader(vs_5_0, vs_main()));
		SetPixelShader(CompileShader(ps_5_0, ps_main()));
		//SetDepthStencilState(LessEqualDSS, 0);
	}
}