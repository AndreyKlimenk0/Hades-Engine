struct Vertex_In {
	float2 position : POSITION;
	float4 color : COLOR;
};

struct Vertex_Out {
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

Vertex_Out vs_main(Vertex_In vertex)
{
	Vertex_Out result;
	result.position = float4(vertex.position, 0.0, 1.0f);
	result.color = vertex.color;
	return result;
}

float4 ps_main(Vertex_Out pixel) : SV_TARGET
{
	return pixel.color;
}