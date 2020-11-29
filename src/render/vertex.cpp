#include "vertex.h"
#include "effect.h"
#include "base.h"
#include "../libs/ds/hash_table.h"

#include <d3dx11effect.h>

ID3D11InputLayout *Input_Layout::vertex_color = NULL;

const D3D11_INPUT_ELEMENT_DESC Input_Layout::vertex_col_desc[2] = {
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

void Input_Layout::init(const Direct3D *direct3d)
{
	ID3DX11Effect *fx = NULL;
	if (!get_fx_shaders(direct3d)->get("base", fx)) {
		return;
	}

	D3DX11_PASS_DESC pass_desc;
	fx->GetTechniqueByIndex(0)->GetPassByIndex(0)->GetDesc(&pass_desc);
	HR(direct3d->device->CreateInputLayout(vertex_col_desc, 2, pass_desc.pIAInputSignature, pass_desc.IAInputSignatureSize, &vertex_color));
}
