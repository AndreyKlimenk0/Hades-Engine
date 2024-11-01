#ifndef IMAGE_H
#define IMAGE_H

#include "../str.h"
#include "../number_types.h"
#include "../../render/render_api.h"

struct Image {
	Image();
	Image(const Image &other);
	~Image();

	u32 width = 0;
	u32 height = 0;
	String file_name;
	Texture2D texture;

	bool init_from_file(const char *file_name, const char *data_directory_name);
	
	Image &operator=(const Image &other);
};
#endif
