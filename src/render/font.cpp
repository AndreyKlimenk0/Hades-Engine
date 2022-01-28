#include <d3dx11.h>
#include <windows.h>

#include "mesh.h"
#include "font.h"
#include "vertex.h"
#include "directx.h"
#include "render_system.h"
#include "../win32/win_types.h"


Font font;

void Font::init(int font_size)
{
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		print("Font::init: Could not init FreeType Library");
	}

	FT_Face face;
	if (FT_New_Face(ft, "C:/Windows/Fonts/consola.ttf", 0, &face)) {
		print("Font::init: Failed to load font");
	}

	FT_Set_Pixel_Sizes(face, 0, font_size);


	for (unsigned char c = 0; c < 128; c++) {
		
		if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
			print("Font::init: Failed to load Char [{}] index [{}]", (char)c, (int)c);
			continue;
		}

		if (!face->glyph->bitmap.buffer) {
			print("Font::init: Failed to load the face bitmap of char {} index {}", (char)c, (int)c);
			continue;
		}
		
		Character *character = new Character();
		character->advance = face->glyph->advance.x;
		character->size.x = face->glyph->bitmap.width;
		character->size.y = face->glyph->bitmap.rows;
		character->bearing.x = face->glyph->bitmap_left;
		character->bearing.y = face->glyph->bitmap_top;

		ID3D11Texture2D *texture = NULL;
		D3D11_TEXTURE2D_DESC glyph_texture_desc;
		ZeroMemory(&glyph_texture_desc, sizeof(D3D11_TEXTURE2D_DESC));
		glyph_texture_desc.Width = face->glyph->bitmap.width;
		glyph_texture_desc.Height = face->glyph->bitmap.rows;
		glyph_texture_desc.MipLevels = 1;
		glyph_texture_desc.ArraySize = 1;
		glyph_texture_desc.SampleDesc.Count = 1;
		glyph_texture_desc.Usage = D3D11_USAGE_DEFAULT;
		glyph_texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		glyph_texture_desc.Format = DXGI_FORMAT_R8_UINT;

		D3D11_SUBRESOURCE_DATA resource_data;
		ZeroMemory(&resource_data, sizeof(D3D11_SUBRESOURCE_DATA));
		resource_data.pSysMem = (void *)face->glyph->bitmap.buffer;
		resource_data.SysMemPitch = face->glyph->bitmap.width;

		HR(directx11.device->CreateTexture2D(&glyph_texture_desc, &resource_data, &texture));

		D3D11_SHADER_RESOURCE_VIEW_DESC resource_view_desc;
		resource_view_desc.Format = DXGI_FORMAT_R8_UINT;
		resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		resource_view_desc.Texture2D.MipLevels = 1;
		resource_view_desc.Texture2D.MostDetailedMip = 0;

		HR(directx11.device->CreateShaderResourceView(texture, &resource_view_desc, &character->texture));
		
		characters.set(c, character);
	}
}


void draw_text(int x, int y, const char *text)
{
	//ID3D11DepthStencilState *state = NULL;
	//D3D11_DEPTH_STENCIL_DESC depth_test_desc;
	//ZeroMemory(&depth_test_desc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	//depth_test_desc.DepthEnable = false;

	//HR(directx11.device->CreateDepthStencilState(&depth_test_desc, &state));
	//directx11.device_context->OMSetDepthStencilState(state, 0);

	//ID3D11BlendState* transparent = NULL;

	//D3D11_BLEND_DESC transparent_desc = { 0 };
	//transparent_desc.AlphaToCoverageEnable = false;
	//transparent_desc.IndependentBlendEnable = false;

	//transparent_desc.RenderTarget[0].BlendEnable = true;
	//transparent_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	//transparent_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	//transparent_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	//transparent_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	//transparent_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	//transparent_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	//transparent_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	//HR(directx11.device->CreateBlendState(&transparent_desc, &transparent));

	//Fx_Shader *font_shader = fx_shader_manager.get_shader("font");

	//directx11.device_context->IASetInputLayout(Input_Layout::vertex_xuv);
	//directx11.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//float b[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	////directx11.device_context->OMSetBlendState(transparent, b, 0xffffffff);

	//const char *symbol = text;
	//while (*symbol) {

	//	Character *character = font.characters[*symbol];

	//	float xpos = x + character->bearing.x;
	//	float ypos = y - (character->size.y - character->bearing.y);

	//	float width = character->size.x;
	//	float height = character->size.y;

	//	Vertex_XUV vertices[6] = {
	//		Vertex_XUV(Vector3(xpos, ypos + height, 1.0f), Vector2(0.0f, 0.0f)),
	//		Vertex_XUV(Vector3(xpos, ypos, 1.0f),          Vector2(0.0f, 1.0f)),
	//		Vertex_XUV(Vector3(xpos + width, ypos, 1.0f),  Vector2(1.0f, 1.0f)),

	//		Vertex_XUV(Vector3(xpos, ypos + height, 1.0f), Vector2(0.0f, 0.0f)),
	//		Vertex_XUV(Vector3(xpos + width, ypos, 1.0f),  Vector2(1.0f, 1.0f)),
	//		Vertex_XUV(Vector3(xpos + width, ypos + height, 1.0f), Vector2(1.0f, 0.0f))
	//	};

	//	ID3D11Buffer *vertex_buffer = NULL;
	//	create_static_vertex_buffer(6, sizeof(Vertex_XUV), (void *)&vertices[0], &vertex_buffer);

	//	font_shader->bind("projection_matrix", &render_sys.view_info->perspective_matrix);
	//	font_shader->bind("orthogonal_matrix", &render_sys.view_info->orthogonal_matrix);
	//	font_shader->bind("texture_map", character->texture);
	//	font_shader->attach();

	//	u32 stride = sizeof(Vertex_XUV);
	//	u32 offset = 0;
	//	directx11.device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);

	//	directx11.device_context->Draw(6, 0);

	//	x += (character->advance >> 6);
	//	symbol++;
	//	RELEASE_COM(vertex_buffer);
	//}
	//directx11.device_context->OMSetBlendState(0, b, 0xffffffff);
	//directx11.device_context->OMSetDepthStencilState(0, 0);
}