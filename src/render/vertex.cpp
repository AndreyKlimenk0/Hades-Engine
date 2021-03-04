#include <d3dx11effect.h>
#include "base.h"
#include "vertex.h"
#include "effect.h"
#include "../libs/ds/hash_table.h"
#include "../sys/sys_local.h"

ID3D11InputLayout *Input_Layout::vertex_color = NULL;
ID3D11InputLayout *Input_Layout::vertex = NULL;

const D3D11_INPUT_ELEMENT_DESC Input_Layout::vertex_col_desc[2] = {
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

const D3D11_INPUT_ELEMENT_DESC Input_Layout::vertex_desc[3] = {
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
};

Input_Layout::~Input_Layout()
{
	//RELEASE_COM(vertex);
	RELEASE_COM(vertex_color);
}

void Input_Layout::init()
{
	ID3DX11Effect *fx = NULL;
	if (!get_fx_shaders(&direct3d)->get("base", fx)) {
		return;
	}

	D3DX11_PASS_DESC pass_desc;
	fx->GetTechniqueByIndex(0)->GetPassByIndex(0)->GetDesc(&pass_desc);

	//HR(direct3d.device->CreateInputLayout(vertex_col_desc, 2, pass_desc.pIAInputSignature, pass_desc.IAInputSignatureSize, &vertex_color));
	HR(direct3d.device->CreateInputLayout(vertex_desc, 3, pass_desc.pIAInputSignature, pass_desc.IAInputSignatureSize, &vertex));

}
