#ifndef HADES_GUI_H
#define HADES_GUI_H

#include "../libs/str.h"
#include "../libs/color.h"
#include "../libs/ds/array.h"
#include "../win32/win_types.h"

struct Engine;
struct Texture2D;

typedef u32 Gui_ID;
typedef u32 Window_Style;
typedef u32 Element_Alignment;
typedef u32 Gui_List_Line_State;

const Element_Alignment RIGHT_ALIGNMENT = 0x01;
const Element_Alignment LEFT_ALIGNMENT = 0x02;

const Window_Style NO_WINDOW_STYLE = 0x0;
const Window_Style WINDOW_HEADER = 0x1;
const Window_Style WINDOW_OUTLINES = 0x2;
const Window_Style WINDOW_SCROLL_BAR = 0x4;
const Window_Style WINDOW_RESIZABLE = 0x8;
const Window_Style WINDOW_CLOSE_BUTTON = 0x10;
const Window_Style WINDOW_TAB_BAR = 0x20;
const Window_Style WINDOW_DEFAULT_STYLE = WINDOW_HEADER | WINDOW_OUTLINES | WINDOW_SCROLL_BAR | WINDOW_RESIZABLE | WINDOW_CLOSE_BUTTON;

enum Alignment {
	RECT_CENTER_ALIGNMENT,
	RECT_LEFT_ALIGNMENT,
	RECT_RIGHT_ALIGNMENT
};

enum Gui_List_Column_State : u32;

struct Gui_List_Column {
	const char *name = NULL;
	u32 size_in_percents = 0;
	Gui_List_Column_State state = (Gui_List_Column_State)0;
};

struct Gui_Edit_Field_Theme {
	bool draw_label = true;
	u32 float_precision = 2;
	s32 text_shift = 5;
	s32 rounded_border = 5;
	s32 caret_blink_time = 3000;
	Rect_s32 rect = { 0, 0, 125, 20 };
	Rect_s32 caret_rect{ 0, 0, 1, 14 };
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

struct Gui_List_Theme {
	bool column_filter = true;
	s32 line_height = 20;
	s32 line_text_offset = 10;
	s32 split_line_size = 18;
	s32 filter_button_size = 12;
	s32 filter_text_offset = 10;
	s32 filter_button_offset = 7;
	s32 filter_rect_height = 20;
	float split_line_thickness = 1.0f;
	Color filter_rect_color = Color(36, 36, 36);
	Color split_lines_color = Color(46, 46, 46);
	Color line_color = Color(40, 40, 40);
	Color background_color = Color(40, 40, 40);
	Color hover_line_color = Color(0, 89, 254, 170);
	Color picked_line_color = Color(0, 89, 254, 235);
	Size_s32 window_size = Size_s32(500, 300);
};

struct Gui_Window_Theme {
	s32 header_height = 18;
	s32 rounded_border = 6;
	s32 rounded_scrolling = 6;
	s32 scroll_size = 8;
	s32 scroll_bar_size = 8;
	s32 min_scroll_size = 10;
	s32 mouse_wheel_spped = 30;
	s32 place_between_rects = 12;
	s32 vertical_offset_from_sides = 10;
	s32 horizontal_offset_from_sides = 15;
	float outlines_width = 1.0f;
	Color header_color = Color(16, 16, 16);
	Color background_color = Color(30, 30, 30);
	Color outlines_color = Color(92, 100, 107);
	Color scroll_bar_color = Color(48, 50, 54);
	Color scroll_color = Color(87, 92, 97);
};

enum Gui_List_Sorting {
	NO_LIST_SORTING,
	SORTING_LIST_UP,
	SORTING_LIST_DOWN,
};

struct Gui_List_Column {
	const char *name = NULL;
	u32 size_in_percents = 0;
	Gui_List_Sorting list_sorting = NO_LIST_SORTING;
};

struct Gui_Line_State {
	bool selected = false;
	bool left_mouse_click = false;
	bool right_mouse_click = false;
};

namespace gui {
	void init_gui(Engine *engine, const char *font_name, u32 font_size);
	void handle_events();
	void shutdown();
	void set_font(const char *font_name, u32 font_size);

	bool were_events_handled();

	void begin_frame();
	void end_frame();

	bool begin_window(const char *name, Window_Style window_style = WINDOW_DEFAULT_STYLE);
	void end_window();

	bool begin_child(const char *name, Window_Style window_style = WINDOW_DEFAULT_STYLE);
	void end_child();

	void set_theme(Gui_Tab_Theme *gui_tab_theme);
	void set_theme(Gui_List_Theme *gui_list_theme);
	void set_theme(Gui_Window_Theme *gui_window_theme);
	void set_theme(Gui_Text_Button_Theme *gui_window_theme);
	void set_theme(Gui_Edit_Field_Theme *gui_edit_field_theme);
	void set_next_theme(Gui_Window_Theme *gui_window_theme);
	
	void reset_tab_theme();
	void reset_list_theme();
	void reset_window_theme();
	void reset_button_theme();
	void reset_edit_field_theme();
	
	void set_next_window_size(s32 width, s32 height);
	void set_next_window_pos(s32 x, s32 y);
	void same_line();
	void next_line();
	
	void make_tab_active(Gui_ID tab_gui_id);
	void make_next_list_active();

	bool button(const char *text, bool *state = NULL);
	bool radio_button(const char *name, bool *state);
	bool image_button(u32 width, u32 height, Texture2D *texture);

	bool add_tab(const char *tab_name);

	void text(const char *some_text);

	void list_box(Array<String> *array, u32 *item_index);
	
	void image(Texture2D *texture, s32 width = -1, s32 height = -1);

	void edit_field(const char *name, int *value);
	void edit_field(const char *name, float *value);
	void edit_field(const char *name, String *string);
	bool edit_field(const char *name, Vector3 *vector, const char *x = "X", const char *y = "Y", const char *z = "z");

	bool begin_list(const char *name, Gui_List_Column filters[], u32 filter_count);
	void end_list();
	
	bool begin_line(Gui_List_Line_State *list_line);
	void end_line();

	bool selected(Gui_List_Line_State list_line_state);
	bool left_mouse_click(Gui_List_Line_State list_line_state);
	bool right_mouse_click(Gui_List_Line_State list_line_state);
	
	bool begin_column(const char *column_name);
	void end_column();

	void add_text(const char *text, Alignment alignment);
	void add_image(Texture2D *texture, Alignment alignment);
	void add_image_button(Texture2D *texture, Alignment alignment);

	Size_s32 get_window_size();
	Gui_ID get_last_tab_gui_id();
}
#endif