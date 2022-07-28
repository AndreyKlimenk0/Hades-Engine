#include "gui.h"

#include "../win32/win_local.h"

#include "../libs/os/input.h"
#include "../libs/os/event.h"
#include "../libs/math/common.h"
#include "../libs/ds/hash_table.h"
#include "../libs/ds/linked_list.h"

#include "../render/font.h"
#include "../render/render_system.h"

Render_2D *render_2d = get_render_2d();

Color header_color = Color(74, 82, 90);
Color Window_color = Color(36, 39, 43);

const s32 MIN_WINDOW_WIDTH = 20;

static Rect_s32 default_window_rect = { 50, 50, 300, 300 };


inline bool detect_collision(Rect_s32 *rect)
{
	if ((Mouse_Input::x > rect->x) && (Mouse_Input::x < (rect->x + rect->width)) && (Mouse_Input::y > rect->y) && (Mouse_Input::y < (rect->y + rect->height))) {
		return true;
	}
	return false;
}

struct Gui_Window_Theme {
	u32 header_height = 20;
	u32 rounded_border = 10;
	Color header_color = Color::White;
	Color background_color = Color(36, 39, 43);
};

Gui_Window_Theme window_theme;

typedef u32 Gui_ID;

enum Rect_Side {
	RECT_SIDE_LEFT,
	RECT_SIDE_RIGHT,
	RECT_SIDE_TOP,
	RECT_SIDE_BOTTOM,
};

struct Gui_Window {
	u32 gui_id;
	Rect_s32 rect;
};

struct Gui_Manager {
	s32 mouse_x;
	s32 mouse_y;
	s32 last_mouse_x;
	s32 last_mouse_y;
	s32 mouse_x_delta;
	s32 mouse_y_delta;

	Gui_ID hot_item;
	Gui_ID active_item;
	Gui_ID resizing_window;
	Gui_ID probably_resizing_window;

	Cursor_Type cursor_type;
	
	Array<Gui_Window> windows; 

	Gui_Window_Theme window_theme;
	
	void begine_frame();
	void end_frame();

	void begine_window(const char *name);
	void end_window();
	
	bool check_item(Gui_ID id, Rect_s32 *rect);
	bool detect_collision_window_borders(Rect_s32 *rect, Rect_Side *rect_side);
	
	Gui_Window *create_or_find_window(const char *name);
};

bool Gui_Manager::check_item(Gui_ID id, Rect_s32 *rect)
{
	bool result = false;
	if (detect_collision(rect)) {
		result = true;
		hot_item = id;
	} 
	return result;
}

Gui_Window *Gui_Manager::create_or_find_window(const char *name)
{
	u32 window_id = fast_hash(name);
	
	Gui_Window *window = NULL;
	For(windows, window) {
		if (window_id == window->gui_id) {
			return window;
		}
	}

	Gui_Window new_window;
	new_window.gui_id = window_id;
	new_window.rect = default_window_rect;
	windows.push(new_window);
	default_window_rect.x += new_window.rect.width + 40;
	return &windows.get_last();
}

inline u32 safe_sub_u32(u32 x, u32 y)
{
	u32 result = x - y;
	return ((result > x) && (result > y)) ? result : 0;
}

void Gui_Manager::begine_frame()
{
	mouse_x = Mouse_Input::x;
	mouse_y = Mouse_Input::y;
	mouse_x_delta = mouse_x - last_mouse_x;
	mouse_y_delta = mouse_y - last_mouse_y;
	hot_item = 0;
	//resizing_window = 0;
}

void Gui_Manager::end_frame()
{
	if (!is_left_mouse_button_down()) {
		active_item = 0;
		resizing_window = 0;
	} 
	last_mouse_x = Mouse_Input::x;
	last_mouse_y = Mouse_Input::y;
}

void Gui_Manager::begine_window(const char *name)
{
	Gui_Window *window = create_or_find_window(name);
	Rect_s32  *rect = &window->rect;

	bool result = check_item(window->gui_id, rect);

	if (result && was_left_mouse_button_just_pressed()) {
		active_item = window->gui_id;
	}

	if (active_item == window->gui_id && is_left_mouse_button_down()) {
		rect->x = math::clamp(rect->x + mouse_x_delta, 0, (s32)win32.window_width - rect->width);
		rect->y = math::clamp(rect->y + mouse_y_delta, 0, (s32)win32.window_height - rect->height);
	}

	Rect_Side rect_side;
	if (detect_collision_window_borders(rect, &rect_side) || (resizing_window == window->gui_id) && is_left_mouse_button_down()) {
		probably_resizing_window = window->gui_id;
		if (rect_side == RECT_SIDE_LEFT) {
			set_cursor(CURSOR_TYPE_RESIZE_LEFT_RIGHT);
		}
		if (was_left_mouse_button_just_pressed()) {
			resizing_window = window->gui_id;
		}
		if ((resizing_window == window->gui_id) && is_left_mouse_button_down()) {

			if ((mouse_x >= 0) && ((rect->width - mouse_x_delta) > MIN_WINDOW_WIDTH)) {
				//rect->x = math::clamp(rect->x + mouse_x_delta, 0, rect->right());
				rect->x = math::max(rect->x + mouse_x_delta, 0);
				//rect->width = math::clamp(rect->width - mouse_x_delta, MIN_WINDOW_WIDTH, win32.window_width - rect->width);
				rect->width = math::max(rect->width - mouse_x_delta, MIN_WINDOW_WIDTH);
			}
		}
	} else {
		if (probably_resizing_window == window->gui_id) {
			probably_resizing_window = 0;
			set_cursor(CURSOR_TYPE_ARROW);
		}
	}

	render_2d->draw_rect(&window->rect, window_theme.background_color, window_theme.rounded_border);
	render_2d->draw_rect(rect->x, rect->y, rect->width, window_theme.header_height, window_theme.header_color, window_theme.rounded_border, ROUND_TOP_RECT);
}

bool Gui_Manager::detect_collision_window_borders(Rect_s32 *rect, Rect_Side *rect_side)
{
	static u32 offset_from_border = 10;
	if ((mouse_x >= (rect->x - offset_from_border)) && (mouse_x <= rect->x) && ((mouse_y >= rect->y) && (mouse_y <= rect->bottom()))) {
		*rect_side = RECT_SIDE_LEFT;
		return true;
	}
	return false;
}

static Gui_Manager gui_manager;

inline void place_in_center(Rect_s32 *in_element_place, Rect_s32 *placed_element)
{
	placed_element->x = ((in_element_place->width / 2) - (placed_element->width / 2)) + in_element_place->x;
	placed_element->y = ((in_element_place->height / 2) - (placed_element->height / 2)) + in_element_place->y;
}

bool button(const char *name)
{
	//Size_u32 size = font.get_text_size(name);
	//Rect_s32 window_rect = { 0, 0, (u32)render_sys.view_info->window_width, (u32)render_sys.view_info->window_height };
	//Rect_s32 button_rect = { 0, 0, size.width * 2, size.height * 2};

	//place_in_center(&window_rect, &button_rect);

	//render_2d->draw_rect(&button_rect, Color::Blue);
	//render_2d->draw_text(&button_rect, name);

	return false;
}

void begine_frame()
{
	gui_manager.begine_frame();
}

void end_frame()
{
	gui_manager.end_frame();
}

void begine_window(const char *name)
{
	gui_manager.begine_window(name);
}
void end_window();

void draw_test_gui()
{
	begine_frame();
	
	begine_window("Test");	
	//begine_window("Test2");	

	end_frame();
}