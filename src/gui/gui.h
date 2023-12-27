#ifndef HADES_GUI_H
#define HADES_GUI_H

#include "../libs/str.h"
#include "../libs/color.h"
#include "../libs/ds/array.h"
#include "../libs/math/common.h"
#include "../win32/win_types.h"

struct Engine;
struct Texture2D;

typedef u32 Window_Style;
const Window_Style NO_WINDOW_STYLE = 0x0;
const Window_Style WINDOW_WITH_HEADER = 0x1;
const Window_Style WINDOW_WITH_OUTLINES = 0x2;
const Window_Style WINDOW_WITH_SCROLL_BAR = 0x4;
const Window_Style WINDOW_STYLE_DEFAULT = WINDOW_WITH_HEADER | WINDOW_WITH_OUTLINES | WINDOW_WITH_SCROLL_BAR;

struct Gui_Edit_Field_Theme {
	u32 float_precision = 2;
	s32 text_shift = 5;
	s32 rounded_border = 5;
	s32 caret_blink_time = 3000;
	Rect_s32 caret_rect{ 0, 0, 1, 14 };
	Rect_s32 edit_field_rect = { 0, 0, 125, 20 };
	Rect_s32 rect = { 0, 0, 220, 20 };
	Color color = Color(50, 50, 50);
	Color x_edit_field_color = Color(140, 0, 0);
	Color y_edit_field_color = Color(0, 140, 0);
	Color z_edit_field_color = Color(0, 0, 140);
};

struct Gui_Radio_Button_Theme {
	s32 rounded_border = 5;
	s32 text_shift = 5;
	Color default_background_color = Color(50, 50, 50);
	Color true_background_color = Color(0, 92, 206);
	Rect_s32 rect = { 0, 0, 220, 16 };
	Rect_s32 radio_rect = { 0, 0, 16, 16 };
	Rect_s32 check_texture_rect = { 0, 0, 12, 12 };
};

struct Gui_Tab_Theme {
	s32 tab_height = 26;
	s32 tab_bar_height = 30;
	s32 additional_space_in_tab = 40;
	Color tab_color = Color(40, 40, 40);
	Color tab_bar_color = Color(40, 40, 40);
	Color active_tab_color = Color(30, 30, 30);
};

struct Gui_List_Box_Theme {
	s32 rounded_border = 5;
	s32 shift_from_size = 10;
	s32 expand_down_texture_shift = 5;
	Color background_color = Color(50, 50, 50);
	Rect_s32 default_rect = { 0, 0, 170, 20 };
	Rect_s32 expand_down_texture_rect = { 0, 0, 15, 15 };
};

struct Gui_Text_Button_Theme {
	u32 aligment = 0;
	s32 shift_from_size = 10;
	s32 rounded_border = 5;
	Color stroke_color = Color::Black;
	Color color = Color(0, 75, 168);
	Color hover_color = Color(0, 60, 168);
	Rect_s32 rect{ 0, 0, 125, 20 };
};

struct Gui_Window_Theme {
	s32 header_height = 18;
	s32 mouse_wheel_spped = 30;
	s32 rounded_border = 6;
	s32 place_between_elements = 12;
	s32 shift_element_from_window_side = 20;
	s32 scroll_bar_width = 15;
	float outlines_width = 1.0f;
	Color header_color = Color(16, 16, 16);
	Color background_color = Color(30, 30, 30);
	Color outlines_color = Color(92, 100, 107);
};

namespace gui {
	void init_gui(Engine *engine, const char *font_name, u32 font_size);
	void handle_events();
	void shutdown();
	void set_font(const char *font_name, u32 font_size);
	void draw_test_gui();

	bool were_events_handled();

	void begin_frame();
	void end_frame();

	bool begin_window(const char *name, Window_Style window_style = WINDOW_STYLE_DEFAULT);
	void end_window();

	bool begin_child(const char *name, Window_Style window_style = WINDOW_STYLE_DEFAULT);
	void end_child();

	void set_theme(Gui_Window_Theme *gui_window_theme);
	void set_theme(Gui_Text_Button_Theme *gui_window_theme);
	void set_next_theme(Gui_Window_Theme *gui_window_theme);
	
	void reset_window_theme();
	void reset_button_theme();
	
	void set_next_window_size(s32 width, s32 height);
	void set_next_window_pos(s32 x, s32 y);
	void same_line();
	void next_line();

	bool button(const char *text, bool *state = NULL);
	bool radio_button(const char *name, bool *state);
	bool image_button(u32 width, u32 height, Texture2D *texture);

	bool add_tab(const char *tab_name);

	void text(const char *some_text);

	void list_box(Array<String> *array, u32 *item_index);
	
	void image(Texture2D *texture, s32 width = -1, s32 height = -1);

	void edit_field(const char *name, int *value);
	void edit_field(const char *name, float *value);
	void edit_field(const char *name, String *value);
	bool edit_field(const char *name, Vector3 *vector, const char *x = "X", const char *y = "Y", const char *z = "z");

	Size_s32 get_window_size();
}
#endif