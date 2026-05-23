#ifndef GUI_WIDGETS_H
#define GUI_WIDGETS_H

#include "../libs/color.h"
#include "../libs/str.h"
#include "../libs/number_types.h"
#include "../libs/structures/array.h"
//#include "../libs/math/structures.h"

#include "guiv2.h"

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
	s32 field_panel_spacing;
	s32 list_item_spacing;
	u32 rounding;
	Color background_color;
	Color item_hover_color;
	imgui::Padding list_panel_padding;
	imgui::Padding list_item_padding;
};

struct Check_Box_Theme {
	Check_Box_Theme();
	~Check_Box_Theme();

	s32 width;
	s32 height;
};

void init_widgets();

void push_button_theme(Text_Button_Theme *button_theme);
void pop_button_theme();

void push_list_box_theme(List_Box_Theme *list_box_theme);
void pop_list_box_theme();

bool button(const char *text);
bool button(Texture_View *texture_view);

void list_box(const char *name, Array<String> &list, u32 *index);

bool check_box(const char *label, bool *state);

#endif
