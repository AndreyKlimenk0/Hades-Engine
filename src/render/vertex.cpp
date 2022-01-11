#include <d3dx11effect.h>
#include "directx.h"
#include "vertex.h"
#include "effect.h"
#include "../libs/ds/hash_table.h"
#include "../sys/sys_local.h"


ID3D11InputLayout *Input_Layout::vertex_color = NULL;
ID3D11InputLayout *Input_Layout::vertex = NULL;
ID3D11InputLayout *Input_Layout::vertex_xuv = NULL;
Hash_Table<String, ID3D11InputLayout *> Input_Layout::table;


const D3D11_INPUT_ELEMENT_DESC Input_Layout::vertex_xuv_desc[2] = {
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{"TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

const D3D11_INPUT_ELEMENT_DESC Input_Layout::vertex_col_desc[2] = {
	{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

const D3D11_INPUT_ELEMENT_DESC Input_Layout::vertex_desc[3] = {
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
};

Input_Layout::~Input_Layout()
{
	RELEASE_COM(vertex);
	RELEASE_COM(vertex_xuv);
	RELEASE_COM(vertex_color);
}

void Input_Layout::init()
{
	ID3DX11Effect *fx = fx_shader_manager.get_shader("base")->shader;
	ID3DX11Effect *color = fx_shader_manager.get_shader("color")->shader;
	ID3DX11Effect *font = fx_shader_manager.get_shader("font")->shader;

	D3DX11_PASS_DESC pass_desc;
	fx->GetTechniqueByIndex(0)->GetPassByIndex(0)->GetDesc(&pass_desc);

	D3DX11_PASS_DESC color_pass_desc;
	color->GetTechniqueByIndex(0)->GetPassByIndex(0)->GetDesc(&color_pass_desc);

	D3DX11_PASS_DESC font_pass_desc;
	font->GetTechniqueByIndex(0)->GetPassByIndex(0)->GetDesc(&font_pass_desc);


	HR(directx11.device->CreateInputLayout(vertex_col_desc, 2, color_pass_desc.pIAInputSignature, color_pass_desc.IAInputSignatureSize, &vertex_color));
	HR(directx11.device->CreateInputLayout(vertex_desc, 3, pass_desc.pIAInputSignature, pass_desc.IAInputSignatureSize, &vertex));
	HR(directx11.device->CreateInputLayout(vertex_xuv_desc, 2, font_pass_desc.pIAInputSignature, font_pass_desc.IAInputSignatureSize, &vertex_xuv));

	table.set("vertex_color", vertex_color);
	table.set("vertex", vertex);
	table.set("vertex_xuv", vertex_xuv);

}
