#ifndef IMGUIV2_H
#define IMGUIV2_H

#include "../libs/number_types.h"
#include "../render/renderer.h"

void init_guiv2(u32 window_width, u32 window_height, Render_2D *render_2d);

void begin_ui_element(const char *name);
void end_ui_element();

void set_position(s32 x, s32 y);
void set_size(s32 width, s32 height);
void set_background_color(const Color &color);

#endif
