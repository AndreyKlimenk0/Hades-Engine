#include <d3dx11effect.h>
#include "directx.h"
#include "vertex.h"
#include "shader.h"
#include "../libs/ds/hash_table.h"
#include "../sys/sys_local.h"
#include "render_system.h"


ID3D11InputLayout *Input_Layout::vertex_color = NULL;
ID3D11InputLayout *Input_Layout::vertex = NULL;
ID3D11InputLayout *Input_Layout::vertex_xuv = NULL;
Hash_Table<String, ID3D11InputLayout *> Input_Layout::table;


const D3D11_INPUT_ELEMENT_DESC Input_Layout::vertex_xuv_desc[2] = {
	{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{"TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
	Shader_Manager *shader_manager = render_sys.get_shader_manager();
	Shader *render_2d = shader_manager->get_shader("forward_light");
	Shader *text = shader_manager->get_shader("draw_text");

	//HR(directx11.device->CreateInputLayout(vertex_col_desc, 2, (void *)render_2d->byte_code, render_2d->byte_code_size, &vertex_color));
	HR(directx11.device->CreateInputLayout(vertex_desc, 3, (void *)render_2d->byte_code, render_2d->byte_code_size, &vertex));
	HR(directx11.device->CreateInputLayout(vertex_xuv_desc, 2, (void *)text->byte_code, text->byte_code_size, &vertex_xuv));

	table.set("vertex_color", vertex_color);
	table.set("vertex", vertex);
	table.set("vertex_xuv", vertex_xuv);
}
