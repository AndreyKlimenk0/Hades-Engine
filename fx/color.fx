#include "global.fx"

struct Vertex_In {
	float3 position : POSITION;
	float4 color : COLOR;
};

struct Vertex_Out {
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

Vertex_Out vs_main(Vertex_In vertex)
{
	Vertex_Out result;
	result.position = mul(float4(vertex.position, 1.0f), world_view_projection);
	result.color = vertex.color;
	return result;
}

float4 ps_main(Vertex_Out pixel) : SV_TARGET
{
	return pixel.color;
}

technique11 Color {
	pass P0 {
		SetVertexShader(CompileShader(vs_5_0, vs_main()));
		SetPixelShader(CompileShader(ps_5_0, ps_main()));
	}
}
