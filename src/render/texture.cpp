#include "directx.h"
#include "texture.h"
#include "../sys/sys_local.h"
#include "../libs/spng.h"
#include "../libs/os/file.h"
#include "../libs/os/path.h"


Texture_Manager texture_manager;


static bool load_png_file(const char *path_to_file, u8 **png_image_buffer, u32 *width, u32 *height)
{
	FILE *png_file = fopen(path_to_file, "rb");
	if (!png_file) {
		print("Can not open PNG file with name {}", extract_file_name(path_to_file));
		return false;
	}

	spng_ctx *ctx =  spng_ctx_new(0);
	spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);

	spng_set_png_file(ctx, png_file);

	struct spng_ihdr ihdr;
    int result = spng_get_ihdr(ctx, &ihdr);

	if (result) {
		print("spng_get_ihdr() error: {}", spng_strerror(result));
		spng_ctx_free(ctx);
		fclose(png_file);
		return false;
	}

	*width = ihdr.width;
	*height = ihdr.height;

	if (*png_image_buffer) {
		delete *png_image_buffer;
	}

	int size = ihdr.width * ihdr.width * 4;
	
	*png_image_buffer = new u8[size];


	//png_image_buffer = new u8[size];

	result = spng_decode_chunks(ctx);
	if (result) {
		print("spng_decode_chunks() error: {}", spng_strerror(result));
		delete *png_image_buffer;
		spng_ctx_free(ctx);
		fclose(png_file);
		return false;
	}

	result = spng_decode_image(ctx, (void *)*png_image_buffer, size, SPNG_FMT_RGBA8, 0);
	if (result) {
		print("spng_decode_image() error: {}", spng_strerror(result));
		delete *png_image_buffer;
		spng_ctx_free(ctx);
		fclose(png_file);
		return false;
	}

	spng_ctx_free(ctx);
	fclose(png_file);
	return true;
}

static Texture *create_texture_2d(Texture *texture, u32 width, u32 height, void *data = NULL, u32 mip_levels = 0,  D3D11_USAGE usage = D3D11_USAGE_DEFAULT, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM)
{
	texture->width = width;
	texture->height = height;

	D3D11_TEXTURE2D_DESC texture_2d_desc;
	ZeroMemory(&texture_2d_desc, sizeof(D3D11_TEXTURE2D_DESC));
	texture_2d_desc.Width = width;
	texture_2d_desc.Height = height;
	texture_2d_desc.MipLevels = mip_levels;
	texture_2d_desc.ArraySize = 1;
	texture_2d_desc.Format = format;
	texture_2d_desc.SampleDesc.Count = 1;
	texture_2d_desc.Usage = usage;
	texture_2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texture_2d_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;


	u32 is_support_mips;
	HR(directx11.device->CheckFormatSupport(format, &is_support_mips))
	if ((mip_levels == 0) && (is_support_mips & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN)) {
		texture_2d_desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
		texture_2d_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	}

	if (data && (mip_levels == 1)) {
		D3D11_SUBRESOURCE_DATA subresource_desc;
		ZeroMemory(&subresource_desc, sizeof(D3D11_SUBRESOURCE_DATA));
		subresource_desc.pSysMem = data;
		subresource_desc.SysMemPitch = width * sizeof(u32);
		
		HR(directx11.device->CreateTexture2D(&texture_2d_desc, &subresource_desc, (ID3D11Texture2D **)&texture->texture))
	} else {
		HR(directx11.device->CreateTexture2D(&texture_2d_desc, NULL, (ID3D11Texture2D **)&texture->texture));
	}

	if (data && (mip_levels == 1)) {
		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_desc;
		shader_resource_desc.Format = format;
		shader_resource_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shader_resource_desc.Texture2D.MostDetailedMip = 0;
		shader_resource_desc.Texture2D.MipLevels = mip_levels;
		
		HR(directx11.device->CreateShaderResourceView(texture->texture, &shader_resource_desc, &texture->shader_resource));
	} else {
		HR(directx11.device->CreateShaderResourceView(texture->texture, NULL, &texture->shader_resource))
	}

	if (data && (mip_levels == 0)) {
		directx11.device_context->UpdateSubresource(texture->texture, 0, NULL, data, width * sizeof(u32), 0);
		directx11.device_context->GenerateMips(texture->shader_resource);
	}

	return texture;
}

static Texture *create_texture_2d(u32 width, u32 height, void *data, u32 mip_levels = 0,  D3D11_USAGE usage = D3D11_USAGE_DEFAULT, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM)
{
	Texture *texture = new Texture();
	texture->type = TEXTURE2D;
	return create_texture_2d(texture, width, height, data, mip_levels, usage, format);
}

#include <d3dx11.h>

Texture *create_texture_from_file(const char *file_name)
{
	String texture_path;
	os_path.build_full_path_to_texture_file(file_name, texture_path);

	u8 *png_image_buffer = NULL;
	u32 png_image_width;
	u32 png_image_height;

	bool result = load_png_file(texture_path, &png_image_buffer, &png_image_width, &png_image_height);
	
	if (!result) {
		print("create_texture: Loading png file {} was failed.", file_name);
		DELETE_PTR(png_image_buffer);
		return NULL;
	}

	return create_texture_2d(png_image_width, png_image_height, (void *)png_image_buffer);
}

Texture::~Texture()
{
	RELEASE_COM(texture);
	RELEASE_COM(shader_resource);
}

void Texture::init(u32 width, u32 height)
{
	create_texture_2d(this, width, height, NULL, 1);
}

void Texture::set_color(const Color &color)
{
	u32 *data = new u32[width * height];
	
	u8* pixels = (u8*)data;
	for( UINT row = 0; row < height; row++ )
	{
		UINT rowStart = row * (width * sizeof(u32));
		for( UINT col = 0; col < width; col++ )
		{
			UINT colStart = col * 4;
			pixels[rowStart + colStart + 0] = u8(color.value.x * 255);
			pixels[rowStart + colStart + 1] = u8(color.value.y * 255);
			pixels[rowStart + colStart + 2] = u8(color.value.z * 255);
			pixels[rowStart + colStart + 3] = u8(color.value.w * 255);
		}
	}

	D3D11_MAPPED_SUBRESOURCE mapped_subresource;
	ZeroMemory(&mapped_subresource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	mapped_subresource.pData = data;
	mapped_subresource.RowPitch = width * sizeof(u32);

	directx11.device_context->UpdateSubresource(texture, 0, NULL, data, width * sizeof(u32), 0);
}

Texture::operator ID3D11ShaderResourceView*()
{
	assert(shader_resource);
	return shader_resource;
}

Texture_Manager::~Texture_Manager()
{
	for (int i = 0; i < textures.count; i++) {
		if (textures.nodes[i] != NULL) {
			DELETE_PTR(textures.nodes[i]);
		}
	}
}

void Texture_Manager::init()
{
	default_texture.init(128, 128);
	default_texture.set_color(Color(85, 85, 85));
}

Texture *Texture_Manager::get_texture(const char *texture_name)
{
	return &default_texture;
	if (texture_name == NULL) {
		return &default_texture;
	}

	Texture *texture;
	if (!textures.get(texture_name, &texture)) {
		texture = create_texture_from_file(texture_name);
		if (!texture) {
			return &default_texture;
		}
		textures.set(texture_name, texture);
	}
	return texture;
}
