#ifndef PND_IMAGE_H
#define PND_IMAGE_H

#include "../win32/win_types.h"

bool load_png_file(const char *path_to_file, u8 **png_image_buffer, u32 *width, u32 *height);

#endif