#ifndef GUI_WIDGETS_H
#define GUI_WIDGETS_H

#include "../libs/color.h"
#include "../libs/number_types.h"
//#include "../libs/math/structures.h"

typedef void *Texture_View;

struct Button_Theme {
	s32 width;
	s32 height;
	Color color;
	Color hover_color;
};

Button_Theme default_button_theme();

void init_widgets();

void push_button_theme(Button_Theme *button_theme);
void pop_button_theme();

bool button(const char *text);
bool button(Texture_View *texture_view);

#endif
