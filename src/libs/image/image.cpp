#include <assert.h>

#include "image.h"
#include "../os/path.h"
#include "../os/file.h"
#include "../../render/render_helpers.h"

Image::Image()
{
}

Image::~Image()
{
	width = 0;
	height = 0;
	file_name.free();
	texture.release();
}

Image::Image(const Image &other)
{
	*this = other;
}

Image &Image::operator=(const Image &other)
{
	if (this != &other) {
		width = other.width;
		height = other.height;
		file_name = other.file_name;
		texture = other.texture;
	}
	return *this;
}

bool Image::init_from_file(const char *_file_name, const char *data_directory_name)
{
	assert(_file_name);
	assert(data_directory_name);

	String path_to_data_directory;
	build_full_path_to_data_directory(data_directory_name, path_to_data_directory);
	String full_path_to_image_file = join_paths(path_to_data_directory, _file_name);
	if (!file_exists(full_path_to_image_file)) {
		return false;
	}
	
	if (create_texture2d_from_file(full_path_to_image_file, texture)) {
		Texture_Info texture_info = get_last_create_texture_info();
		width = texture_info.width;
		height = texture_info.height;
		return true;
	}
	return false;
}
