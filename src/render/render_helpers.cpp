#include <assert.h>
#include <dxgi.h>

#include "render_helpers.h"
#include "../sys/engine.h"
#include "../sys/sys_local.h"
#include "../win32/win_types.h"


const u32 MAX_U24 = 16777215;


bool is_valid_texture(Texture2D *texture2d, String *error_message)
{
	assert(texture2d);

	bool result = true;
	const char *message = NULL;

	if (texture2d->width == 0) {
		message = "Texture2d is not valid. Width was not set.";
		result = false;
	} else if (texture2d->height == 0) {
		message = "Texture2d is not valid. Height was not set.";
		result = false;
	} else if (texture2d->format_size == 0) {
		message = "Texture2d is not valid. Format size was not set.";
		result = false;
	} else if (texture2d->format == DXGI_FORMAT_UNKNOWN) {
		message = "Texture2d is not valid. Format is unknown.";
		result = false;
	} else if (!texture2d->resource) {
		message = "Texture2d is not valid. Gpu resource was not allocated.";
		result = false;
	}

	if (!result && error_message) {
		*error_message = message;
	}
	return result;
}

static bool fill_texture_data(void *data, void *value, Texture2D *texture2d)
{
	if (texture2d->format == DXGI_FORMAT_R8G8B8A8_UNORM) {
		u32 *texels = (u32 *)data;
		u32 rgba = ((Color *)value)->get_packed_rgba();

		for (u32 row = 0; row < texture2d->height; row++) {
			for (u32 column = 0; column < texture2d->width; column++) {
				texels[column] = rgba;
			}
			texels += texture2d->width;
		}
		return true;
	} else if ((texture2d->format == DXGI_FORMAT_R24_UNORM_X8_TYPELESS) || (texture2d->format == DXGI_FORMAT_R24G8_TYPELESS) || (texture2d->format == DXGI_FORMAT_D24_UNORM_S8_UINT)) {
		u32 *texels = (u32 *)data;
		u32 r24u8 = ((R24U8 *)value)->get_packed_value();

		for (u32 row = 0; row < texture2d->height; row++) {
			for (u32 column = 0; column < texture2d->width; column++) {
				texels[column] = r24u8;
			}
			texels += texture2d->width;
		}
		return true;
	} else {
		print("fill_texture: A 2d texture can't be filled with value because the texture2d has unsupported format.");
		return false;
	}
}

static bool fill_dynamic_resource_texture(void *value, Texture2D *texture2d)
{
	void *data = Engine::get_render_system()->render_pipeline.map(*texture2d);
	bool result = fill_texture_data(data, value, texture2d);
	Engine::get_render_system()->render_pipeline.unmap(*texture2d);
	return result;
}

static bool fill_default_resource_texture(void *value, Texture2D *texture2d)
{
	void *data = new u8[texture2d->get_size()];
	bool result = fill_texture_data(data, value, texture2d);
	Engine::get_render_system()->render_pipeline.update_subresource(texture2d, data, texture2d->get_pitch());
	DELETE_ARRAY(data);
	return result;
}

void fill_texture_with_value(void *value, Texture2D *texture2d)
{
	assert(texture2d);

	String error_message;
	if (!is_valid_texture(texture2d, &error_message)) {
		print("fill_texture_with_color: {}", error_message);
		return;
	}

	bool result = true;
	if (texture2d->usage == RESOURCE_USAGE_DEFAULT) {
		result = fill_default_resource_texture(value, texture2d);
	} else if (texture2d->usage == RESOURCE_USAGE_DYNAMIC) {
		result = fill_dynamic_resource_texture(value, texture2d);
	} else {
		print("fill_texture_with_color: A 2d texture can't be filled with value because the texture2d has unsupported resource usage.");
	}

	if (!result) {
		print("fill_texture_with_color: Failed to fill a 2d texture.");
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
