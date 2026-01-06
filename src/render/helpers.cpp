#include "helpers.h"
#include "render_api/render.h"
#include "../libs/image/image.h"
#include "../sys/engine.h"
#include "../libs/os/file.h"
#include "../libs/os/path.h"

Texture *create_texture_from_image(Image *image)
{
	Render_Device *render_device = Engine::get_render_system()->render_device;
	if (!image->valid()) {
		return NULL;
	}

	Texture_Desc texture_desc;
	texture_desc.name = "Temp texture";
	texture_desc.dimension = TEXTURE_DIMENSION_2D;
	texture_desc.width = image->width;
	texture_desc.height = image->height;
	texture_desc.format = image->format;
	//texture_desc.flags = ALLOW_UNORDERED_ACCESS;
	texture_desc.miplevels = find_max_mip_level(image->width, image->height);
	texture_desc.data = (void *)image->data;

	return render_device->create_texture(&texture_desc);
}

Texture *create_texture_from_file(const char *full_path_to_texture)
{
	Render_Device *render_device = Engine::get_render_system()->render_device;

	Image image;
	if (load_image_from_file(full_path_to_texture, DXGI_FORMAT_R8G8B8A8_UNORM, &image)) {
		Texture_Desc texture_desc;
		extract_file_name(full_path_to_texture, texture_desc.name);
		texture_desc.dimension = TEXTURE_DIMENSION_2D;
		texture_desc.width = image.width;
		texture_desc.height = image.height;
		texture_desc.format = image.format;
		//texture_desc.flags = ALLOW_UNORDERED_ACCESS;
		texture_desc.miplevels = find_max_mip_level(image.width, image.height);
		//texture_desc.resource_state = RESOURCE_STATE_COPY_DEST;
		texture_desc.resource_state = RESOURCE_STATE_COMMON;
		texture_desc.data = (void *)image.data;

		return render_device->create_texture(&texture_desc);
	}
	return NULL;
}

Texture *create_texture_from_file(const char *file_name, const char *directory)
{
	String path_to_data_directory;
	build_full_path_to_data_directory(directory, path_to_data_directory);
	String full_path_to_file = join_paths(path_to_data_directory, file_name);
	return create_texture_from_file(full_path_to_file);
}
