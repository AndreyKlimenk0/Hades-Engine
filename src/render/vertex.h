#ifndef VERTEX_H
#define VERTEX_H

#include <d3d11.h>

#include "../elib/math/vector.h"


struct Vertex_Col {
	Vector3 position;
	Vector4 color;
};

struct Input_Layout {
	~Input_Layout();
	
	static const ID3D11InputLayout *vertex_col;
	static const D3D11_INPUT_ELEMENT_DESC vertex_col_desc[2];
	
	void init();
};
#endif