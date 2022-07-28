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
		String file_name;
		extract_file_name(path_to_file, file_name);
		print("Can not open PNG file with name {}", file_name);
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

static u32 size_of_dxgi_format(DXGI_FORMAT format)
{
	switch (static_cast<int>(format))
	{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return 16;

		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 12;

		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		case DXGI_FORMAT_Y416:
		case DXGI_FORMAT_Y210:
		case DXGI_FORMAT_Y216:
			return 8;

		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		case DXGI_FORMAT_AYUV:
		case DXGI_FORMAT_Y410:
		case DXGI_FORMAT_YUY2:
			return 4;

		case DXGI_FORMAT_P010:
		case DXGI_FORMAT_P016:
			return 3;

		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_B5G6R5_UNORM:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
		case DXGI_FORMAT_A8P8:
		case DXGI_FORMAT_B4G4R4A4_UNORM:
			return 2;

		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return 1;

		default:
			return 0;
		}
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
		subresource_desc.SysMemPitch = width * size_of_dxgi_format(format);
		
		HR(directx11.device->CreateTexture2D(&texture_2d_desc, &subresource_desc, (ID3D11Texture2D **)&texture->texture))
	} else {
		HR(directx11.device->CreateTexture2D(&texture_2d_desc, NULL, (ID3D11Texture2D **)&texture->texture));
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_desc;
	shader_resource_desc.Format = format;
	shader_resource_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shader_resource_desc.Texture2D.MostDetailedMip = 0;
	shader_resource_desc.Texture2D.MipLevels = mip_levels == 1 ? 1 : (mip_levels == 0 ? -1 : mip_levels);

	HR(directx11.device->CreateShaderResourceView(texture->texture, &shader_resource_desc, &texture->shader_resource));

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

Texture *create_texture_from_file(const char *file_name)
{
	String file_extension;
	extract_file_extension(file_name, file_extension);

	if (file_extension != "png") {
		print("create_texture_from_file: the fucntion supports only png file");
		return NULL;
	}

	String texture_path;
	os_path.build_full_path_to_texture_file(file_name, texture_path);
	if (!file_exists(texture_path)) {
		
		os_path.build_full_path_to_editor_file(file_name, texture_path);
		if (!file_exists(texture_path)) {
			print("create_texture_from_file: file with name {} was not found", file_name);
			return NULL;
		}
	}

	u8 *png_image_buffer = NULL;
	u32 png_image_width;
	u32 png_image_height;

	bool result = load_png_file(texture_path, &png_image_buffer, &png_image_width, &png_image_height);
	
	if (!result) {
		print("create_texture_from_file: Loading png file {} was failed.", file_name);
		DELETE_PTR(png_image_buffer);
		return NULL;
	}

	Texture *texture = create_texture_2d(png_image_width, png_image_height, (void *)png_image_buffer);
	DELETE_PTR(png_image_buffer);
	
	return texture;
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
	
	u8* pixels = (u8 *)data;
	for(u32 row = 0; row < height; row++ )
	{
		u32 row_start = row * (width * sizeof(u32));
		for(u32 col = 0; col < width; col++ )
		{
			u32 col_start = col * 4;
			pixels[row_start + col_start + 0] = u8(color.value.x * 255);
			pixels[row_start + col_start + 1] = u8(color.value.y * 255);
			pixels[row_start + col_start + 2] = u8(color.value.z * 255);
			pixels[row_start + col_start + 3] = u8(color.value.w * 255);
		}
	}

	D3D11_MAPPED_SUBRESOURCE mapped_subresource;
	ZeroMemory(&mapped_subresource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	mapped_subresource.pData = data;
	mapped_subresource.RowPitch = width * sizeof(u32);

	directx11.device_context->UpdateSubresource(texture, 0, NULL, data, width * sizeof(u32), 0);

	DELETE_PTR(data);
}

void Texture::update(Rect_u32 *rect, void *data, u32 row_pitch)
{
	D3D11_BOX box;
	box.left = rect->x;
	box.right = rect->x + rect->width;
	box.top = rect->y;
	box.bottom = rect->y + rect->height;
	box.front = 0;
	box.back = 1;
	directx11.device_context->UpdateSubresource(texture, 0, &box, (const void *)data, row_pitch, 0);
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

	Texture *texture = NULL;
	For(rest_textures, texture) {
		DELETE_PTR(texture);
	}
}

void Texture_Manager::init()
{
	default_texture.init(128, 128);
	default_texture.set_color(Color(150, 150, 150));
}

Texture *Texture_Manager::get_texture(const char *texture_name)
{
	if (texture_name == NULL) {
		return &default_texture;
	}

	Texture *texture = NULL;
	if (!textures.get(texture_name, &texture)) {
		texture = create_texture_from_file(texture_name);
		if (!texture) {
			return &default_texture;
		}
		texture->name = texture_name;
		textures.set(texture_name, texture);
	}
	return texture;
}

Texture *Texture_Manager::create_texture(u32 width, u32 height, DXGI_FORMAT format, u32 mips_level)
{
	Texture *texture = create_texture_2d(width, height, NULL, mips_level, D3D11_USAGE_DEFAULT, format);
	rest_textures.push(texture);
	return texture;
}

u32 *r8_to_rgba32(u8 *data, u32 width, u32 height)
{
	u32 *new_data = new u32[width * height];

	u8* pixels = (u8*)new_data;
	for(u32 row = 0; row < height; row++) {
		u32 row_start = row * (width * sizeof(u32));
		u32 row_2 = row * (width * sizeof(u8));
		
		for(u32 col = 0; col < width; col++) {
			u32 col_start = col * 4;
			pixels[row_start + col_start + 0] = 255;
			pixels[row_start + col_start + 1] = 255;
			pixels[row_start + col_start + 2] = 255;
			pixels[row_start + col_start + 3] = data[row_2 + col];
		}
	}
	return new_data;
}

//void rgba32_to_abgr32(u32 *data, u32 width, u32 height)
//{
//	u8* pixels = (u8 *)data;
//	for(u32 row = 0; row < height; row++)
//	{
//		u32 row_start = row * (width * sizeof(u32));
//		u32 row_2 = row * (width * sizeof(u8));
//		
//		for(u32 col = 0; col < width; col++)
//		{
//			u32 col_start = col * 4;
//			pixels[row_start + col_start + 0] = data[row_2 + col];
//			pixels[row_start + col_start + 1] = 0;
//			pixels[row_start + col_start + 2] = 0;
//			pixels[row_start + col_start + 3] = 255;
//		}
//	}
//	return new_data;
//}
