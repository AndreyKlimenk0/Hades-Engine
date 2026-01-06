#ifndef RENDER_HELPERS_H
#define RENDER_HELPERS_H

struct Image;
struct Texture;

Texture *create_texture_from_image(Image *image);
Texture *create_texture_from_file(const char *full_path_to_texture);
#endif
