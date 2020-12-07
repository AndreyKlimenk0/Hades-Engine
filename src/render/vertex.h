#ifndef VERTEX_H
#define VERTEX_H

#include <d3d11.h>
#include "../render/base.h"
#include "../libs/math/vector.h"


struct Vertex_Color {
	Vector3 position;
	Vector4 color;
	Vertex_Color() {}
	Vertex_Color(Vector3 position, Vector4 color) : position(position), color(color) {}
};

struct Vertex {
	Vector3 position;
	Vector3 normal;
	Vector2 uv;
};

struct Input_Layout {
	~Input_Layout();
	
	static ID3D11InputLayout *vertex_color;
	static const D3D11_INPUT_ELEMENT_DESC vertex_col_desc[2];
	
	static void init(const Direct3D * direct3d);
};
#endif