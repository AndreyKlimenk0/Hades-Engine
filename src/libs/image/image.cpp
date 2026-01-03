#include <assert.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "image.h"
#include "../os/path.h"
#include "../os/file.h"
#include "../../sys/sys.h"
#include "../../sys/utils.h"
#include "../../render/render_apiv2/d3d12_render_api/d3d12_functions.h"


Image::Image()
{
}

Image::~Image()
{
	clear();
}

Image::Image(const Image &other)
{
	*this = other;
}

Image &Image::operator=(const Image &other)
{
	if ((this != &other) && other.data && (other.width != 0) && (other.height != 0) && (other.format != DXGI_FORMAT_UNKNOWN)) {
		width = other.width;
		height = other.height;
		format = other.format;
		file_name = other.file_name;

		if (data) { DELETE_PTR(data); }
		u32 image_size = width * height * dxgi_format_size(format);
		data = new u8[image_size];
		memcpy(data, other.data, image_size);
	}
	return *this;
}

void Image::clear()
{
	width = 0;
	height = 0;
	DELETE_PTR(data);
	format = DXGI_FORMAT_UNKNOWN;
	file_name.free();
}

void Image::allocate_memory(u32 image_width, u32 image_height, DXGI_FORMAT image_format)
{
	assert(image_width > 0);
	assert(image_height > 0);
	assert(image_format != DXGI_FORMAT_UNKNOWN);

	clear();
	width = image_width;
	height = image_height;
	format = image_format;

	data = new u8[width * height * dxgi_format_size(image_format)];
}

void Image::fill(const Color &color)
{
	if (format == DXGI_FORMAT_R8G8B8A8_UNORM) {
		u32 *texels = (u32 *)data;
		u32 rgba = const_cast<Color &>(color).get_packed_rgba();
		for (u32 row = 0; row < height; row++) {
			for (u32 column = 0; column < width; column++) {
				texels[column] = rgba;
			}
			texels += width;
		}
	} else {
		assert(false);
	}
}

bool Image::valid()
{
	return (width != 0) && (height != 0) && (format != DXGI_FORMAT_UNKNOWN) && data;
}

bool load_image_from_file(const char *full_path_to_file, DXGI_FORMAT format, Image *image)
{
	assert(full_path_to_file);

	if (!file_exists(full_path_to_file)) {
		print("load_image_from_file: Failed to load a image file. The path {} doesn't exist.", full_path_to_file);
		return false;
	}

	s32 width = 0;
	s32 height = 0;
	s32 channels = 0;
	u8 *data = stbi_load(full_path_to_file, (s32 *)&width, (s32 *)&height, &channels, dxgi_format_size(format));
	if (!data) {
		print("load_image_from_file: stbi_load failed.");
		return false;
	}

	if (dxgi_format_size(format) != channels) {
		//print("load_image_from_file: Failed to load a image file from {}. A desired chanenels is not equal a image file channels.", full_path_to_file);
		//DELETE_PTR(data);
		//return false;
	}

	image->clear();
	image->width = static_cast<u32>(width);
	image->height = static_cast<u32>(height);
	image->data = data;
	image->format = format;
	extract_file_name(full_path_to_file, image->file_name);

	return true;
}
