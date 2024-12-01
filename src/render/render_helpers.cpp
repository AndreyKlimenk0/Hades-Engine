#include <assert.h>
#include <dxgi.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "render_helpers.h"
#include "../sys/sys.h"
#include "../sys/engine.h"

const u32 MAX_U24 = 16777215;

//bool is_valid_texture(Texture2D_Desc *texture_desc, String *error_message)
//{
//	assert(texture_desc);
//	assert(error_message);
//
//	bool result = true;
//	const char *message = NULL;
//
//	if (texture_desc->width == 0) {
//		message = "Texture2d is not valid. Width was not set.";
//		result = false;
//	} else if (texture_desc->height == 0) {
//		message = "Texture2d is not valid. Height was not set.";
//		result = false;
//	} else if (dxgi_format_size(texture_desc->format) == 0) {
//		message = "Texture2d is not valid. Format size was not set.";
//		result = false;
//	} else if (texture_desc->format == DXGI_FORMAT_UNKNOWN) {
//		message = "Texture2d is not valid. Format is unknown.";
//		result = false;
//	}
//
//	if (!result && error_message) {
//		*error_message = message;
//	}
//	return result;
//}
//
//static bool fill_texture_data(void *data, void *value, Texture2D_Desc *texture_desc)
//{
//	if (texture_desc->format == DXGI_FORMAT_R8G8B8A8_UNORM) {
//		u32 *texels = (u32 *)data;
//		u32 rgba = ((Color *)value)->get_packed_rgba();
//
//		for (u32 row = 0; row < texture_desc->height; row++) {
//			for (u32 column = 0; column < texture_desc->width; column++) {
//				texels[column] = rgba;
//			}
//			texels += texture_desc->width;
//		}
//		return true;
//	} else if ((texture_desc->format == DXGI_FORMAT_R24_UNORM_X8_TYPELESS) || (texture_desc->format == DXGI_FORMAT_R24G8_TYPELESS) || (texture_desc->format == DXGI_FORMAT_D24_UNORM_S8_UINT)) {
//		u32 *texels = (u32 *)data;
//		u32 r24u8 = ((R24U8 *)value)->get_packed_value();
//
//		for (u32 row = 0; row < texture_desc->height; row++) {
//			for (u32 column = 0; column < texture_desc->width; column++) {
//				texels[column] = r24u8;
//			}
//			texels += texture_desc->width;
//		}
//		return true;
//	} else {
//		print("fill_texture: A 2d texture can't be filled with value because the texture2d has unsupported format.");
//		return false;
//	}
//}
//
//static bool fill_dynamic_resource_texture(void *value, Texture2D_Desc *texture_desc, Texture2D *texture2d)
//{
//	void *data = Engine::get_render_system()->render_pipeline.map(*texture2d);
//	bool result = fill_texture_data(data, value, texture_desc);
//	Engine::get_render_system()->render_pipeline.unmap(*texture2d);
//	return result;
//}
//
//static bool fill_default_resource_texture(void *value, Texture2D_Desc *texture_desc, Texture2D *texture2d)
//{
//	void *data = new u8[get_texture_size(texture_desc)];
//	bool result = fill_texture_data(data, value, texture_desc);
//	Engine::get_render_system()->render_pipeline.update_subresource(texture2d, data, get_texture_pitch(texture_desc));
//	DELETE_ARRAY(data);
//	return result;
//}
//
//void fill_texture(void *value, Texture2D *texture2d)
//{
//	assert(texture2d);
//
//	if (!texture2d->resource) {
//		print("Texture2d is not valid. Gpu resource was not allocated.");
//		return;
//	}
//
//	Texture2D_Desc texture_desc;
//	texture2d->get_desc(&texture_desc);
//
//	String error_message;
//	if (!is_valid_texture(&texture_desc, &error_message)) {
//		print("fill_texture_with_value: {}", error_message);
//		return;
//	}
//
//	bool result = true;
//	if (texture_desc.usage == RESOURCE_USAGE_DEFAULT) {
//		result = fill_default_resource_texture(value, &texture_desc, texture2d);
//	} else if (texture_desc.usage == RESOURCE_USAGE_DYNAMIC) {
//		result = fill_dynamic_resource_texture(value, &texture_desc, texture2d);
//	} else {
//		print("fill_texture_with_value: A 2d texture can't be filled with value because the texture2d has unsupported resource usage.");
//	}
//
//	if (!result) {
//		print("fill_texture_with_value: Failed to fill a 2d texture.");
//	}
//}
//
R24U8::R24U8(u32 r24u8_value)
{
	numerator = (r24u8_value & 0x00ffffff);
	typeless_bits = (u8)(r24u8_value >> 24);
}

R24U8::R24U8(u32 numerator, u8 typeless_bits) : numerator(numerator), typeless_bits(typeless_bits)
{
	numerator &= 0x00ffffff;
}

u32 R24U8::get_packed_value()
{
	u32 value = 0;
	value = (value | typeless_bits) << 24;
	value = (value | numerator);
	return value;
}

float R24U8::get_unorm_value()
{
	assert(MAX_U24 >= numerator);
	return (float)numerator / (float)MAX_U24;
}

u32 dxgi_format_size(DXGI_FORMAT format)
{
	switch (static_cast<int>(format)) {
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