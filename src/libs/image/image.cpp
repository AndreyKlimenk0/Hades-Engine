#include <assert.h>

#include "png.h"
#include "image.h"
#include "../os/path.h"
#include "../os/file.h"

Image::Image()
{
}

Image::~Image()
{
	width = 0;
	height = 0;
	texture.release();
}

bool Image::init_from_file(const char *file_name, const char *data_directory_name)
{
	assert(file_name);
	assert(data_directory_name);

	String file_extension;
	if (!extract_file_extension(file_name, file_extension) || (file_extension != "png")) {
		return false;
	}
	String path_to_data_directory;
	build_full_path_to_data_directory(data_directory_name, path_to_data_directory);
	String full_path_to_image_file = join_paths(path_to_data_directory, file_name);
	if (!file_exists(full_path_to_image_file)) {
		return false;
	}
	u32 image_width = 0;
	u32 image_height = 0;
	u8 *image_data = NULL;
	if (load_png_file(full_path_to_image_file, &image_data, &image_width, &image_height)) {
		Gpu_Device *gpu_device = get_current_gpu_device();
		Texture2D_Desc texture_desc;
		texture_desc.width = image_width;
		texture_desc.height = image_height;
		texture_desc.mip_levels = 1;
		texture_desc.data = (void *)image_data;

		texture.release();
		gpu_device->create_texture_2d(&texture_desc, &texture);
		gpu_device->create_shader_resource_view(&texture_desc, &texture);
		
		width = image_width;
		height = image_height;
		DELETE_PTR(image_data);
		return true;
	}
	return false;
}
