#ifndef GUI_WIDGETS_H
#define GUI_WIDGETS_H

#include "../libs/color.h"
#include "../libs/number_types.h"
//#include "../libs/math/structures.h"

typedef void *Texture_View;

struct Text_Button_Theme {
	Text_Button_Theme();
	~Text_Button_Theme();

	s32 width;
	s32 height;
	u32 rounding;
	u32 rounding_flags;
	Color color;
	Color hover_color;
};

struct List_Box_Theme {
	List_Box_Theme();
	~List_Box_Theme();

	s32 width;
	s32 height;
	u32 rounding;
	Color color;
};

void init_widgets();

void push_button_theme(Text_Button_Theme *button_theme);
void pop_button_theme();

bool button(const char *text);
bool button(Texture_View *texture_view);

void begin_list_box(const char *name);

#endif
