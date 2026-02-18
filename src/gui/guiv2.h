#ifndef IMGUIV2_H
#define IMGUIV2_H

#include "../libs/number_types.h"
#include "../render/renderer.h"

enum Size_Type {
	SIZE_TYPE_FILLED,
	SIZE_TYPE_FIXED,
	SIZE_TYPE_PERCENT,
};

struct Size_Dimension {
	Size_Type type;
	union {
		u32 fixed;
		float percent;
	};
};

inline Size_Dimension filled_size()
{
	return { .type = SIZE_TYPE_FILLED };
}

inline Size_Dimension fixed_size(u32 value)
{
	return { .type = SIZE_TYPE_FIXED, .fixed = value };
}

inline Size_Dimension percent_size(u32 value)
{
	return { .type = SIZE_TYPE_FIXED, .fixed = value };
}

void init_guiv2(u32 window_width, u32 window_height, const char *font_name, u32 font_size, Render_2D *render_2d);
void shutdown_guiv2();

void begin_frame();
void end_frame();

void begin_ui_element(const char *name);
void end_ui_element();

void set_position(s32 x, s32 y);
void set_size(Size_Dimension horizontal, Size_Dimension vertical);
void set_background_color(const Color &color);

#endif
