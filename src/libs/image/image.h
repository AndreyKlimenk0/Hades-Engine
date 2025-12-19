#ifndef IMAGE_H
#define IMAGE_H

#include <dxgi1_5.h>

#include "../str.h"
#include "../color.h"
#include "../number_types.h"

struct Image {
	Image();
	Image(const Image &other);
	~Image();

	u32 width = 0;
	u32 height = 0;
	u8 *data = NULL;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	String file_name;

	Image &operator=(const Image &other);

	void clear();
	void allocate_memory(u32 image_width, u32 image_height, DXGI_FORMAT image_format);
	void fill(const Color &color);
	bool valid();
};

bool load_image_from_file(const char *full_path_to_file, DXGI_FORMAT format, Image *image);

inline u32 find_max_mip_level(u32 width, u32 height)
{
	return math::log2(math::max(width, height));
}
#endif
