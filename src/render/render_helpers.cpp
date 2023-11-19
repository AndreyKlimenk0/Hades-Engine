#include <assert.h>
#include <dxgi.h>

#include "render_helpers.h"
#include "../sys/engine.h"
#include "../sys/sys_local.h"
#include "../win32/win_types.h"


const u32 MAX_U24 = 16777215;


bool is_valid_texture(Texture2D_Desc *texture_desc, String *error_message)
{
	assert(texture_desc);
	assert(error_message);

	bool result = true;
	const char *message = NULL;

	if (texture_desc->width == 0) {
		message = "Texture2d is not valid. Width was not set.";
		result = false;
	} else if (texture_desc->height == 0) {
		message = "Texture2d is not valid. Height was not set.";
		result = false;
	} else if (get_dxgi_format_size(texture_desc->format) == 0) {
		message = "Texture2d is not valid. Format size was not set.";
		result = false;
	} else if (texture_desc->format == DXGI_FORMAT_UNKNOWN) {
		message = "Texture2d is not valid. Format is unknown.";
		result = false;
	}

	if (!result && error_message) {
		*error_message = message;
	}
	return result;
}

static bool fill_texture_data(void *data, void *value, Texture2D_Desc *texture_desc)
{
	if (texture_desc->format == DXGI_FORMAT_R8G8B8A8_UNORM) {
		u32 *texels = (u32 *)data;
		u32 rgba = ((Color *)value)->get_packed_rgba();

		for (u32 row = 0; row < texture_desc->height; row++) {
			for (u32 column = 0; column < texture_desc->width; column++) {
				texels[column] = rgba;
			}
			texels += texture_desc->width;
		}
		return true;
	} else if ((texture_desc->format == DXGI_FORMAT_R24_UNORM_X8_TYPELESS) || (texture_desc->format == DXGI_FORMAT_R24G8_TYPELESS) || (texture_desc->format == DXGI_FORMAT_D24_UNORM_S8_UINT)) {
		u32 *texels = (u32 *)data;
		u32 r24u8 = ((R24U8 *)value)->get_packed_value();

		for (u32 row = 0; row < texture_desc->height; row++) {
			for (u32 column = 0; column < texture_desc->width; column++) {
				texels[column] = r24u8;
			}
			texels += texture_desc->width;
		}
		return true;
	} else {
		print("fill_texture: A 2d texture can't be filled with value because the texture2d has unsupported format.");
		return false;
	}
}

static bool fill_dynamic_resource_texture(void *value, Texture2D_Desc *texture_desc, Texture2D *texture2d)
{
	void *data = Engine::get_render_system()->render_pipeline.map(*texture2d);
	bool result = fill_texture_data(data, value, texture_desc);
	Engine::get_render_system()->render_pipeline.unmap(*texture2d);
	return result;
}

static bool fill_default_resource_texture(void *value, Texture2D_Desc *texture_desc, Texture2D *texture2d)
{
	void *data = new u8[get_texture_size(texture_desc)];
	bool result = fill_texture_data(data, value, texture_desc);
	Engine::get_render_system()->render_pipeline.update_subresource(texture2d, data, get_texture_pitch(texture_desc));
	DELETE_ARRAY(data);
	return result;
}

void fill_texture(void *value, Texture2D *texture2d)
{
	assert(texture2d);

	if (!texture2d->resource) {
		print("Texture2d is not valid. Gpu resource was not allocated.");
		return;
	}

	Texture2D_Desc texture_desc;
	texture2d->get_desc(&texture_desc);

	String error_message;
	if (!is_valid_texture(&texture_desc, &error_message)) {
		print("fill_texture_with_value: {}", error_message);
		return;
	}

	bool result = true;
	if (texture_desc.usage == RESOURCE_USAGE_DEFAULT) {
		result = fill_default_resource_texture(value, &texture_desc, texture2d);
	} else if (texture_desc.usage == RESOURCE_USAGE_DYNAMIC) {
		result = fill_dynamic_resource_texture(value, &texture_desc, texture2d);
	} else {
		print("fill_texture_with_value: A 2d texture can't be filled with value because the texture2d has unsupported resource usage.");
	}

	if (!result) {
		print("fill_texture_with_value: Failed to fill a 2d texture.");
	}
}

R24U8::R24U8(u32 r24u8_value)
{
	numerator = (r24u8_value & 0x0ffffff);
	typeless_bits = (u8)(r24u8_value >> 24);
}

R24U8::R24U8(u32 numerator, u8 typeless_bits) : numerator(numerator), typeless_bits(typeless_bits)
{
	numerator &= 0x0ffffff;
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

u32 *r8_to_rgba32(u8 *data, u32 width, u32 height)
{
	u32 *new_data = new u32[width * height];

	u8* pixels = (u8*)new_data;
	for (u32 row = 0; row < height; row++) {
		u32 row_start = row * (width * sizeof(u32));
		u32 row_2 = row * (width * sizeof(u8));

		for (u32 col = 0; col < width; col++) {
			u32 col_start = col * 4;
			if (data[row_2 + col] > 0) {
				pixels[row_start + col_start + 0] = 255;
				pixels[row_start + col_start + 1] = 255;
				pixels[row_start + col_start + 2] = 255;
				pixels[row_start + col_start + 3] = data[row_2 + col];
			} else {
				pixels[row_start + col_start + 0] = 255;
				pixels[row_start + col_start + 1] = 0;
				pixels[row_start + col_start + 2] = 0;
				pixels[row_start + col_start + 3] = 0;
			}
		}
	}
	return new_data;
}
//
//template<typename T>
//void Gpu_Struct_Buffer::allocate(u32 elements_count)
//{
//	Gpu_Buffer_Desc desc;
//	desc.usage = RESOURCE_USAGE_DYNAMIC;
//	desc.data = NULL;
//	desc.data_size = sizeof(T);
//	desc.struct_size = sizeof(T);
//	desc.data_count = elements_count;
//	desc.bind_flags = BIND_SHADER_RESOURCE;
//	desc.cpu_access = CPU_ACCESS_WRITE;
//	desc.misc_flags = RESOURCE_MISC_BUFFER_STRUCTURED;
//
//	Gpu_Device *gpu_device = &Engine::get_render_system()->gpu_device;
//	gpu_device->create_gpu_buffer(&desc, &gpu_buffer);
//	gpu_device->create_shader_resource_view(&gpu_buffer);
//}
//
//template<typename T>
//void Gpu_Struct_Buffer::update(Array<T> *array)
//{
//	if (array->count == 0) {
//		return;
//	}
//
//	Render_Pipeline *render_pipeline = &Engine::get_render_system()->render_pipeline;
//
//	if (array->count > gpu_buffer.data_count) {
//		free();
//		allocate<T>(array->count);
//	}
//
//	T *buffer = (T *)render_pipeline->map(gpu_buffer);
//	memcpy((void *)buffer, (void *)array->items, sizeof(T) * array->count);
//	render_pipeline->unmap(gpu_buffer);
//}
//
//void Gpu_Struct_Buffer::free()
//{
//	if (!gpu_buffer.is_empty()) {
//		gpu_buffer.free();
//	}
//}