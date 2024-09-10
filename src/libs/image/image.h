#ifndef IMAGE_H
#define IMAGE_H

#include "../number_types.h"
#include "../../render/render_api.h"

struct Image {
	u32 width = 0;
	u32 height = 0;
	Texture2D texture;

	bool init_from_file(const char *file_name, const char *data_directory_name);
};
#endif
