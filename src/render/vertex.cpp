#include "vertex.h"


const ID3D11InputLayout *Input_Layout::vertex_col = NULL;

const D3D11_INPUT_ELEMENT_DESC Input_Layout::vertex_col_desc[2] = {
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

void Input_Layout::init()
{
	
}
