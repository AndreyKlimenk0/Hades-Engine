#ifndef HADES_GUI_H
#define HADES_GUI_H

#include "../libs/ds/array.h"
#include "../render/render_system.h"

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
	Color color = Color(66, 70, 75);
};

struct Gui_Radio_Button_Theme {
	s32 rounded_border = 10;
	s32 text_shift = 5;
	Color default_color = Color(74, 82, 90);
	Color color_for_true = Color(0, 75, 168);
	Rect_s32 true_rect = { 0, 0, 15, 15 };
	Rect_s32 radio_rect = { 0, 0, 20, 20 };
	Rect_s32 rect = { 0, 0, 220, 20 };
};

struct Gui_Tab_Theme {
	s32 tab_height = 26;
	s32 tab_bar_height = 30;
	s32 additional_space_in_tab = 40;
	Color tab_color = Color(40, 40, 40);
	Color tab_bar_color = Color(40, 40, 40);
	Color active_tab_color = Color(30, 30, 30);
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
	void init_gui(Render_2D *render_2d, Win32_Info *win32_info, Font *font, Gpu_Device *gpu_device);
	void shutdown();
	void draw_test_gui();

	bool were_events_handled();

	void begin_frame();
	void end_frame();

	bool begin_window(const char *name, Window_Style window_style = WINDOW_STYLE_DEFAULT);
	void end_window();

	bool begin_child(const char *name, Window_Style window_style = WINDOW_STYLE_DEFAULT);
	void end_child();

	void set_next_theme(Gui_Window_Theme *gui_window_theme);
	void set_next_window_size(s32 width, s32 height);
	void set_next_window_pos(s32 x, s32 y);
	void same_line();
	void next_line();

	bool button(const char *text);
	void radio_button(const char *name, bool *state);

	bool add_tab(const char *tab_name);

	void text(const char *some_text);

	void list_box(Array<String> *array, u32 *item_index);
	
	void image(Texture *texture, s32 width = -1, s32 height = -1);

	void edit_field(const char *name, int *value);
	void edit_field(const char *name, float *value);
	void edit_field(const char *name, String *value);
	void edit_field(const char *name, Vector3 *vector, const char *x = "X", const char *y = "Y", const char *z = "z");
}
#endif