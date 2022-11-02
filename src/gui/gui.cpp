#include <stdlib.h>
#include "gui.h"

#include "../win32/win_local.h"
#include "../win32/win_time.h"
#include "../sys/sys_local.h"

#include "../libs/str.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/os/input.h"
#include "../libs/os/event.h"
#include "../libs/math/common.h"
#include "../libs/ds/stack.h"
#include "../libs/ds/hash_table.h"
#include "../libs/ds/linked_list.h"

#include "../render/font.h"

//@Note included for testing
#include "../game/world.h"


typedef u32 Gui_ID;

typedef u32 Element_Alignment;
const Element_Alignment ALIGNMENT_HORIZONTALLY = 0x01;
const Element_Alignment ALIGNMENT_VERTICALLY = 0x02;
const Element_Alignment ALIGNMENT_RIGHT = 0x04;
const Element_Alignment ALIGNMENT_LEFT = 0x08;

const s32 MIN_WINDOW_WIDTH = 20;
const s32 MIN_WINDOW_HEIGHT = 40;

static const Rect_s32 default_window_rect = { 50, 50, 300, 300 };

#define GET_RENDER_LIST() (&window->render_list)

struct Gui_Edit_Field_Theme {
	s32 rounded_border = 5;
	s32 text_shift = 5;
	Rect_s32 edit_field_rect = { 0, 0, 125, 20 };
	Rect_s32 default_rect = { 0, 0, 220, 20 };
	Color color = Color(74, 82, 90);
};

struct Gui_Radio_Button_Theme {
	s32 rounded_border = 10;
	s32 text_shift = 5;
	Color default_color = Color(74, 82, 90);
	Color color_for_true = Color(0, 75, 168);
	Rect_s32 true_rect = { 0, 0, 15, 15 };
	Rect_s32 radio_rect = { 0, 0, 20, 20 };
	Rect_s32 default_rect = { 0, 0, 220, 20 };
};

struct Gui_Text_Button_Theme {
	u32 aligment = 0;
	s32 shift_from_size = 10;
	s32 rounded_border = 5;
	Color stroke_color = Color::Black;
	Color color = Color(0, 75, 168);
	Color hover_color = Color(0, 60, 168);
	Rect_s32 default_rect{ 0, 0, 125, 20 };
};

struct Gui_Window_Theme {
	s32 header_height = 20;
	s32 rounded_border = 6;
	s32 place_between_elements = 10;
	s32 shift_element_from_window_side = 20;
	s32 scroll_bar_width = 15;
	float outlines_width = 2.0f;
	Color header_color = Color::White;
	Color background_color = Color(36, 39, 43);
	Color outlines_color = Color(92, 100, 107);
};

Gui_Window_Theme default_window_theme;
Gui_Text_Button_Theme default_button_theme;

static void draw_debug_rect(Rect_s32 *rect)
{
	Color red = Color::Red;
	red.value.w = 0.4f;
	//render_2d->draw_rect(rect, red);
}

inline bool detect_collision(Rect_s32 *rect)
{
	if ((Mouse_Input::x > rect->x) && (Mouse_Input::x < (rect->x + rect->width)) && (Mouse_Input::y > rect->y) && (Mouse_Input::y < (rect->y + rect->height))) {
		return true;
	}
	return false;
}

template <typename T>
inline bool detect_collision(Triangle<T> *triangle, Point_V2<T> *point)
{
	T triangle_area = triangle->get_area();

	T area1 = Triangle<T>(triangle->a, triangle->b, *point).get_area();
	T area2 = Triangle<T>(*point, triangle->b, triangle->c).get_area();
	T area3 = Triangle<T>(triangle->a, *point, triangle->c).get_area();

	if (area1 + area2 + area3 == triangle_area) {
		return true;
	}
	return false;
}

enum Axis {
	X_AXIS = 0,
	Y_AXIS = 1,
	BOTH_AXIS = 2
};

inline void place_in_center(Rect_s32 *in_element_place, Rect_s32 *placed_element, Axis axis)
{
	if ((axis == X_AXIS) || (axis == BOTH_AXIS)) {
		placed_element->x = ((in_element_place->width / 2) - (placed_element->width / 2)) + in_element_place->x;
	}

	if ((axis == Y_AXIS) || (axis == BOTH_AXIS)) {
		placed_element->y = ((in_element_place->height / 2) - (placed_element->height / 2)) + in_element_place->y;
	}
}
inline void place_in_middle_and_by_left(Rect_s32 *placing_element, Rect_s32 *placed_element, int offset_from_left = 0)
{
	placed_element->x = placing_element->x + offset_from_left;
	place_in_center(placing_element, placed_element, Y_AXIS);
}

inline void place_in_middle_and_by_right(Rect_s32 *placing_element, Rect_s32 *placed_element, int offset_from_right = 0)
{
	placed_element->x = placing_element->x + placing_element->width - placed_element->width - offset_from_right;
	place_in_center(placing_element, placed_element, Y_AXIS);
}

enum Window_Type {
	WINDOW_TYPE_PARENT,
	WINDOW_TYPE_CHILD,
};

struct Gui_Window {
	Gui_ID gui_id;

	Window_Type type;
	Element_Alignment alignment;

	Rect_s32 rect;
	Rect_s32 view_rect;
	Rect_s32 content_rect;
	Point_s32 next_place;
	Point_s32 scroll;

	String name;
	
	Array<Gui_Window *> child_windows;
	Render_Primitive_List render_list;

	void set_position(s32 x, s32 y);
	Rect_s32 get_scrollbar_rect(Axis axis);
};

void Gui_Window::set_position(s32 x, s32 y)
{
	s32 old_win_y = rect.y;
	s32 old_win_x = rect.x;

	rect.set(x, y);
	view_rect.set(x, y);

	content_rect.x += rect.x - old_win_x;
	content_rect.y += rect.y - old_win_y;

	next_place.x = content_rect.x;
	next_place.y = content_rect.y;

	scroll.x += rect.x - old_win_x;
	scroll.y += rect.y - old_win_y;
}

Rect_s32 Gui_Window::get_scrollbar_rect(Axis axis)
{
	const static s32 scroll_bar_width = 8;
	if (axis == Y_AXIS) {
		return Rect_s32(rect.right() - scroll_bar_width, rect.y, scroll_bar_width, view_rect.height);
	} 
	return Rect_s32(rect.x, rect.bottom() - scroll_bar_width, view_rect.width, scroll_bar_width);
}

const u32 SET_WINDOW_POSITION = 0x1;
const u32 SET_WINDOW_SIZE = 0x2;
const u32 SET_WINDOW_THEME = 0x4;

struct Gui_Manager {

	s32 mouse_x;
	s32 mouse_y;
	s32 last_mouse_x;
	s32 last_mouse_y;
	s32 mouse_x_delta;
	s32 mouse_y_delta;

	u32 reset_window_params;
	u32 list_box_count;
	u32 edit_field_count;
	u32 window_parent_count;

	Gui_ID hot_item;
	Gui_ID active_item;
	Gui_ID active_list_box;
	Gui_ID focused_edit_field;
	Gui_ID became_just_focused; //window
	Gui_ID resizing_window;
	Gui_ID probably_resizing_window;

	Font *font = NULL;
	Win32_Info *win32_info = NULL;

	//2D render api
	Render_2D *render_2d = NULL;

	Cursor_Type cursor_type;
	Rect_s32 window_rect;
	
	Stack<Gui_Window *> window_stack;
	Array<Gui_Window *> windows_order;
	Array<Gui_Window> windows;

	Gui_Edit_Field_Theme edit_field_theme;
	Gui_Window_Theme window_theme;
	Gui_Text_Button_Theme button_theme;
	Gui_Radio_Button_Theme radio_button_theme;

	struct Edit_Field_State {
		Edit_Field_State() {};
		//@Cleanup: font member must be deleted
		Font *font = NULL;
		bool (*is_symbol_valid)(char symbol);
		
		s32 caret_x_posiiton;
		int max_symbol_number;
		int caret_index_in_text; // this caret index specifies at character is placed befor the caret.
		int caret_index_for_inserting; // this caret index specifies at character is placed after the caret.
		String data;

		void handle_event(Event *event);
		void set_params(const char *_str_value, s32 _caret_x_posiiton, int _max_text_width, bool(*is_symbol_valid)(char symbol));

	} edit_field_state;

	void init(Render_2D *_render_2d, Win32_Info *_win32_info, Font *_font);
	void shutdown();
	
	void new_frame();
	void end_frame();

	void begin_window(const char *name, Window_Type window_type);
	void end_window();
	void same_line();
	void next_line();
	void set_next_window_pos(s32 x, s32 y);
	void set_next_window_size(s32 width, s32 height);
	void set_next_window_theme(Gui_Window_Theme *theme);
	void place_rect_in_window(Gui_Window *window, Rect_s32 *rect);

	void list_box(const char *strings[], u32 item_count, u32 *item_index);
	void scroll_bar(Gui_Window *window, Axis axis, Rect_s32 *scroll_bar);
	void radio_button(const char *name, bool *state);
	
	void edit_field(const char *name, int *value);
	void edit_field(const char *name, float *value);
	void edit_field(const char *name, String *value);
	bool edit_field(const char *name, const char *value, u32 max_symbols_number, bool(*is_symbol_valid)(char symbol));
	
	bool button(const char *name);

	bool check_item(Gui_ID id, Rect_s32 *rect);
	bool detect_collision_window_borders(Rect_s32 *rect, Rect_Side *rect_side);
	bool check_rect_state(const char *name, Rect_s32 *rect, bool (*click_callback)() = NULL);

	Rect_s32 get_win32_rect();
	Rect_s32 get_text_rect(const char *text);

	Gui_Window *get_window();
	Gui_Window *find_window(const char *name);
	Gui_Window *find_window_in_order(const char *name, int *window_index);

	Gui_Window *create_window(const char *name, Window_Type window_type);
	Gui_Window *create_window(const char *name, Rect_s32 *rect);
};

Rect_s32 Gui_Manager::get_text_rect(const char *text)
{
	Rect_s32 text_rect;
	Size_u32 size = font->get_text_size(text);
	text_rect.width = (s32)size.width;
	text_rect.height = (s32)size.height;
	return text_rect;
}

void Gui_Manager::set_next_window_pos(s32 x, s32 y)
{
	window_rect.set(x, y);
	reset_window_params |= SET_WINDOW_POSITION;
}

void Gui_Manager::set_next_window_size(s32 width, s32 height)
{
	window_rect.set_size(width, height);
	reset_window_params |= SET_WINDOW_SIZE;
}

void Gui_Manager::set_next_window_theme(Gui_Window_Theme *theme)
{
	window_theme = *theme;
	reset_window_params |= SET_WINDOW_THEME;;
}

void Gui_Manager::place_rect_in_window(Gui_Window *window, Rect_s32 *rect)
{
	assert(!((window->alignment & ALIGNMENT_VERTICALLY) && (window->alignment & ALIGNMENT_HORIZONTALLY)));

	static bool reset_x_position = false;
	static s32 prev_rect_height = 0;
	u32 offset = window_theme.place_between_elements;

	if (window->alignment & ALIGNMENT_VERTICALLY) {
		if (reset_x_position) {
			window->next_place.x = window->content_rect.x;
			reset_x_position = false;
			window->next_place.y += prev_rect_height + offset;
			window->content_rect.height += prev_rect_height + offset;
		}
		rect->y = window->next_place.y + offset;
		rect->x = window->next_place.x + offset;
		window->next_place.y += rect->height + offset;
		window->content_rect.height += rect->height + offset;
	}

	if (window->alignment & ALIGNMENT_HORIZONTALLY) {
		reset_x_position = true;
		prev_rect_height = rect->height;
		rect->x = window->next_place.x + offset;
		rect->y = window->next_place.y + offset;
		window->next_place.x += rect->width + offset;
		window->content_rect.width += rect->width + offset;
	}
}

Rect_s32 Gui_Manager::get_win32_rect()
{
	return Rect_s32(0, 0, win32_info->window_width, win32_info->window_height);
}

Gui_Window *Gui_Manager::get_window()
{
	if (window_stack.is_empty()) {
		error("Hades gui error: Window stask is empty");
	}
	return window_stack.top();
}

Gui_Window *Gui_Manager::find_window(const char *name)
{
	Gui_ID window_id = fast_hash(name);

	for (int i = 0; i < windows.count; i++) {
		if (name == windows[i].name) {
			return &windows[i];
		}
	}
	return NULL;
}

Gui_Window *Gui_Manager::find_window_in_order(const char *name, int *window_index)
{
	Gui_ID window_id = fast_hash(name);
	
	for (int i = 0; i < windows_order.count; i++) {
		if (name == windows_order[i]->name) {
			*window_index = i;
			return windows_order[i];
		}
	}
	return NULL;
}

Gui_Window *Gui_Manager::create_window(const char *name, Window_Type window_type)
{
	Gui_ID window_id = fast_hash(name);
	
	Gui_Window new_window;
	new_window.gui_id = window_id;
	new_window.type = window_type;
	new_window.rect = window_rect;
	new_window.view_rect = new_window.rect;
	new_window.alignment = 0;
	new_window.alignment |= ALIGNMENT_VERTICALLY;
	new_window.content_rect.set(new_window.rect.x, new_window.rect.y);

	new_window.scroll.x = new_window.rect.x;
	new_window.scroll.y = new_window.rect.y;

	new_window.name = name;
	new_window.render_list = Render_Primitive_List(render_2d);

	windows.push(new_window);
	window_rect.x += new_window.rect.width + 40;

	if (window_type == WINDOW_TYPE_PARENT) {
		window_parent_count++;
	}

	return &windows.last_item();
}

Gui_Window *Gui_Manager::create_window(const char *name, Rect_s32 *rect)
{
	Gui_ID window_id = fast_hash(name);

	Gui_Window new_window;
	new_window.gui_id = window_id;
	new_window.type = WINDOW_TYPE_PARENT;
	new_window.rect = *rect;
	new_window.view_rect = new_window.rect;
	new_window.alignment = 0;
	new_window.alignment |= ALIGNMENT_VERTICALLY;
	new_window.content_rect.set(new_window.rect.x, new_window.rect.y);

	new_window.scroll.x = new_window.rect.x;
	new_window.scroll.y = new_window.rect.y;

	new_window.name = name;
	new_window.render_list= Render_Primitive_List(render_2d);

	windows.push(new_window);
	windows_order.push(&windows.last_item());

	window_parent_count += 1;

	return &windows.last_item();
}

bool Gui_Manager::check_rect_state(const char *name, Rect_s32 *rect, bool (*click_callback)())
{
	Gui_ID gui_id = 0;
	if (name) {
		gui_id = fast_hash(name);
	}
	if (!click_callback) {
		click_callback = was_click_by_left_mouse_button;
	}

	bool button_click = false;
	bool result = detect_collision(rect);
	if (result) {
		hot_item = gui_id;
		if (click_callback()) {
			active_item = gui_id;
			button_click = true;
		}
	}
	return button_click;
}

inline bool must_item_be_drawn(Rect_s32 *win_rect, Rect_s32 *item_rect)
{
	if ((item_rect->x > win_rect->right()) || (item_rect->y > win_rect->bottom())) {
		return false;
	}
	return true;
}

Rect_s32 calcualte_clip_rect(Rect_s32 *win_rect, Rect_s32 *item_rect)
{
	Rect_s32 clip_rect;

	if (win_rect->x > item_rect->x) {
		clip_rect.x = win_rect->x;
	} else {
		clip_rect.x = item_rect->x;
	}

	if (item_rect->right() > win_rect->right()) {
		clip_rect.width = item_rect->width - math::abs(win_rect->right() - item_rect->right());
	} else {
		clip_rect.width = item_rect->width;
	}

	if (win_rect->y > item_rect->y) {
		clip_rect.y = win_rect->y;
	} else {
		clip_rect.y = item_rect->y;
	}

	if (item_rect->bottom() > win_rect->bottom()) {
		clip_rect.height = item_rect->height - math::abs(win_rect->bottom() - item_rect->bottom());
	} else {
		clip_rect.height = item_rect->height;
	}
	return clip_rect;
}

void Gui_Manager::radio_button(const char *name, bool *state)
{
	Rect_s32 true_rect = radio_button_theme.true_rect;
	Rect_s32 radio_rect = radio_button_theme.radio_rect;
	Rect_s32 rect = radio_button_theme.default_rect;
	Rect_s32 text_rect = get_text_rect(name);
	rect.width = text_rect.width + radio_rect.width + radio_button_theme.text_shift;
	
	Gui_Window *window = get_window();
	place_rect_in_window(window, &rect);

	if (must_item_be_drawn(&window->rect, &rect)) {
		Rect_s32 clip_rect = calcualte_clip_rect(&window->rect, &rect);
		
		place_in_middle_and_by_left(&rect, &text_rect, radio_rect.width + radio_button_theme.text_shift);
		place_in_middle_and_by_left(&rect, &radio_rect, 0);
		place_in_center(&radio_rect, &true_rect, BOTH_AXIS);

		if (check_rect_state(NULL, &radio_rect)) {
			if (*state) {
				*state = false;
			} else {
				*state = true;
			}
		}

		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&clip_rect);
		render_list->add_rect(&radio_rect, radio_button_theme.default_color, radio_button_theme.rounded_border);
		if (*state) {
			render_list->add_rect(&true_rect, radio_button_theme.color_for_true, radio_button_theme.rounded_border);
		}
		render_list->add_text(&text_rect, name);
		render_list->pop_clip_rect();
	}
}

//Rect_s32 set_caret_position_on_mouse_click(const char *text, int mouse_x, int mouse_y, Rect_s32 *field_rect, Rect_s32 *rect)
//{
//	Rect_s32 caret_rect = { 0, 0, 1, 20 };
//	
//	Size_u32 size = font.get_text_size(text);
//	int text_width = size.width;
//	
//	int shift_caret_from_left = 5;
//	//int mouse_x_relative_text = mouse_x - rect.x - theme.shift_caret_from_left;
//	int mouse_x_relative_text = mouse_x - rect->x - shift_caret_from_left;
//
//	if (mouse_x_relative_text > text_width) {
//		caret_rect.x = field_rect->x + shift_caret_from_left + text_width;
//		return;
//	}
//
//	float caret_temp_x = caret_rect.x;
//	float characters_width = 0.0f;
//
//	int len = strlen(text);
//	for (int i = 0; i < len; i++) {
//		char c = text[i];
//		u32 char_width = font.get_char_width(c);
//		float mouse_x_relative_character = mouse_x_relative_text - characters_width;
//
//		float pad = 0.5f;
//		if ((mouse_x_relative_character >= 0.0f) && ((mouse_x_relative_character + pad) <= char_width)) {
//
//			float characters_width = 0.0f;
//			for (int j = 0; j < i; j++) {
//				char _char = text[j];
//				u32 char_width = font.get_char_width(c);
//				characters_width += char_width;
//			}
//
//			caret_rect.x = rect->x + shift_caret_from_left + characters_width;
//			caret_index_for_inserting = i;
//			caret_index_in_text = i - 1;
//			break;
//		}
//
//		characters_width += char_width;
//		caret.rect.x = caret_temp_x;
//	}
//}

bool is_draw_caret(s32 blink_time)
{
	s64 show_time = blink_time;
	s64 hidding_time = blink_time;
	
	static bool show = true;
	static s64 show_time_accumulator = 0;
	static s64 hidding_time_accumulator = 0;
	static s64 last_time = 0;

	s64 current_time = milliseconds_counter();

	s64 elapsed_time = current_time - last_time;

	if (show) {
		show_time_accumulator += elapsed_time;
		if (show_time_accumulator <= show_time) {
			return true;
		} else {
			show = false;
			show_time_accumulator = 0;
		}
	} else {
		hidding_time_accumulator += elapsed_time;
		if (hidding_time_accumulator >= hidding_time) {
			show = true;
			hidding_time_accumulator = 0;
		}
	}

	last_time = milliseconds_counter();
	return false;
}

static bool is_symbol_int_valid(char symbol)
{
	return (isdigit(symbol) || (symbol == '-'));
}

static bool is_symbol_float_valid(char symbol)
{
	return (isdigit(symbol) || (symbol == '-') || (symbol == '.'));
}

void Gui_Manager::edit_field(const char *name, int *value)
{
	char *str_value = to_string(*value);
	bool is_focused = edit_field(name, str_value, 10, &is_symbol_int_valid);
	free_string(str_value);
	if (is_focused) {
		*value = atoi(edit_field_state.data.c_str());
	}
}

void Gui_Manager::edit_field(const char *name, float *value)
{
	char *str_value = to_string(*value);
	int len = strlen(str_value);
	String str(str_value, 0, len - 4);
	bool is_focused = edit_field(name, str.c_str(), 15, &is_symbol_float_valid);
	free_string(str_value);
	if (is_focused) {
		*value = (float)atof(edit_field_state.data.c_str());
	}
}

void Gui_Manager::edit_field(const char *name, String *value)
{
}

bool Gui_Manager::edit_field(const char *name, const char *value, u32 max_symbol_number, bool(*is_symbol_valid)(char symbol))
{
	bool is_focused = false;
	Rect_s32 caret_rect{ 0, 0, 1, 14 };

	Rect_s32 rect = edit_field_theme.default_rect;
	Rect_s32 edit_field_rect = edit_field_theme.edit_field_rect;
	Rect_s32 text_rect = get_text_rect(name);
	Rect_s32 value_rect = get_text_rect(value);
	rect.width = text_rect.width + edit_field_rect.width + edit_field_theme.text_shift;

	Gui_Window *window = get_window();
	place_rect_in_window(window, &rect);

	if (must_item_be_drawn(&window->rect, &rect)) {
		Rect_s32 clip_rect = calcualte_clip_rect(&window->rect, &rect);
		Rect_s32 label_rect = get_text_rect(name);

		place_in_middle_and_by_left(&rect, &edit_field_rect, 0);
		place_in_middle_and_by_left(&rect, &label_rect, edit_field_rect.width + edit_field_theme.text_shift);
		place_in_middle_and_by_left(&edit_field_rect, &value_rect, edit_field_theme.text_shift);
		place_in_middle_and_by_left(&value_rect, &caret_rect, value_rect.width);

		bool is_new_session = false;
		String edit_field_name = window->name + String("_edit_field_") + String((int)edit_field_count);
		Gui_ID edit_field_id = fast_hash(edit_field_name);
		if (check_rect_state(edit_field_name, &edit_field_rect)) {
			if (active_item == edit_field_id) {
				focused_edit_field = edit_field_id;
				is_new_session = true;
			}
		} else {
			if (was_click_by_left_mouse_button() && !detect_collision(&edit_field_rect) && (focused_edit_field == edit_field_id)) {
				focused_edit_field = 0;
			}
		}

		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&clip_rect);
		render_list->add_rect(&edit_field_rect, edit_field_theme.color, edit_field_theme.rounded_border);
		render_list->add_text(&label_rect, name);

		if (focused_edit_field == edit_field_id) {
			is_focused = true;
			if (is_new_session) {
				edit_field_state.set_params(value, caret_rect.x, max_symbol_number, is_symbol_valid);
			}
			if (value != edit_field_state.data) {
				edit_field_state.data = value;
			}
			handle_events(&edit_field_state, &Gui_Manager::Edit_Field_State::handle_event);

			caret_rect.x = edit_field_state.caret_x_posiiton;
			if (is_draw_caret(900)) {
				render_list->add_rect(&caret_rect, Color::White);
			}
		}
		render_list->add_text(&value_rect, value);
		render_list->pop_clip_rect();
	}
	edit_field_count++;
	return is_focused;
}

void Gui_Manager::Edit_Field_State::handle_event(Event * event)
{
	if (event->type == EVENT_TYPE_KEY) {
		if (event->is_key_down(VK_BACK)) {
			if (caret_index_in_text > -1) {

				char c = data[caret_index_in_text];
				data.remove(caret_index_in_text);

				u32 char_width = font->get_char_width(c);
				caret_x_posiiton -= (s32)char_width;

				caret_index_in_text -= 1;
				caret_index_for_inserting -= 1;
			}

		} else if (event->is_key_down(VK_LEFT)) {
			if (caret_index_in_text > -1) {

				char c = data[caret_index_in_text];
				u32 char_width = font->get_char_width(c);
				if (c == '.') {
					char_width = font->get_char_advance(c);
				}
				caret_x_posiiton -= (s32)char_width;

				caret_index_in_text -= 1;
				caret_index_for_inserting -= 1;
			}
		} else if (event->is_key_down(VK_RIGHT)) {
			if (caret_index_in_text < data.len) {

				caret_index_in_text += 1;
				caret_index_for_inserting += 1;

				char c = data[caret_index_in_text];
				u32 char_width = font->get_char_width(c);
				if (c == '.') {
					char_width = font->get_char_advance(c);
				}
				caret_x_posiiton += (s32)char_width;
			}
		} else if (event->is_key_down(VK_HOME)) {
				
		}
	} else if (event->type == EVENT_TYPE_CHAR) {
			if ((max_symbol_number > data.len) && is_symbol_valid(event->char_key)) {

				if (caret_index_in_text == (data.len - 1)) {
					data.append(event->char_key);
				} else {
					data.insert(caret_index_for_inserting, event->char_key);
				}
				caret_index_in_text += 1;
				caret_index_for_inserting += 1;

				u32 char_width = font->get_char_width(event->char_key);
				caret_x_posiiton += (s32)char_width;
			}
	}
}

void Gui_Manager::Edit_Field_State::set_params(const char *_str_value, s32 _caret_x_posiiton, int _max_text_width, bool(*_is_symbol_valid)(char symbol))
{
	data = _str_value;
	caret_x_posiiton = _caret_x_posiiton;
	max_symbol_number = _max_text_width;
	caret_index_for_inserting = data.len;
	caret_index_in_text = data.len - 1;
	is_symbol_valid = _is_symbol_valid;
}

void Gui_Manager::list_box(const char * strings[], u32 item_count, u32 *item_index)
{
	assert(item_count > 0);

	if (*item_index >= item_count) {
		*item_index = 0;
	}

	Rect_s32 list_box_rect = { 0, 0, 220, 20 };
	
	Gui_Window *window = get_window();
	place_rect_in_window(window, &list_box_rect);
	
	String list_box_name = window->name + String("_list_box_") + String((int)list_box_count);
	if (check_rect_state(list_box_name, &list_box_rect)) {
		if (active_item == active_list_box) {
			active_list_box = 0;
		} else {
			active_list_box = active_item;
		}
	}

	u32 alignment = ALIGNMENT_LEFT;
	Gui_ID gui_id = fast_hash(list_box_name);
	if (gui_id == active_list_box) {
		Gui_Window_Theme win_theme;
		win_theme.shift_element_from_window_side = 0;
		win_theme.outlines_width = 1.0f;
		win_theme.place_between_elements = 0;
		set_next_window_theme(&win_theme);
		
		set_next_window_pos(list_box_rect.x, list_box_rect.bottom() + 5);
		set_next_window_size(list_box_rect.width, item_count * button_theme.default_rect.height);
		
		begin_window(list_box_name, WINDOW_TYPE_CHILD);

		Gui_Text_Button_Theme theme;
		theme.default_rect.width = list_box_rect.width;
		theme.color = window_theme.background_color;
		theme.aligment |= alignment;

		button_theme = theme;
		for (u32 i = 0; i < item_count; i++) {
			if (button(strings[i])) {
				*item_index = i;
			}
		}
		button_theme = default_button_theme;
		end_window();
	}

	if (must_item_be_drawn(&window->rect, &list_box_rect)) {
		Rect_s32 clip_rect = calcualte_clip_rect(&window->rect, &list_box_rect);
		Rect_s32 text_rect = get_text_rect(strings[*item_index]);
		
		if (alignment & ALIGNMENT_RIGHT) {
			place_in_middle_and_by_right(&list_box_rect, &text_rect, button_theme.shift_from_size);
		} else if (alignment & ALIGNMENT_LEFT) {
			place_in_middle_and_by_left(&list_box_rect, &text_rect, button_theme.shift_from_size);
		} else {
			place_in_center(&list_box_rect, &text_rect, BOTH_AXIS);
		}
		
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&clip_rect);
		render_list->add_rect(&list_box_rect, Color(74, 82, 90), button_theme.rounded_border);
		render_list->add_text(&text_rect, strings[*item_index]);
		render_list->pop_clip_rect();
	}

	list_box_count++;
}

bool Gui_Manager::button(const char *name)
{
	bool button_click = false;
	Rect_s32 button_rect = button_theme.default_rect;
	
	Gui_Window *window = get_window();
	place_rect_in_window(window, &button_rect);

	bool button_down = check_rect_state(NULL, &button_rect);

	Color button_color = detect_collision(&button_rect) ? button_theme.hover_color : button_theme.color;

	if (must_item_be_drawn(&window->rect, &button_rect)) {
		Rect_s32 clip_rect = calcualte_clip_rect(&window->rect, &button_rect);
		Rect_s32 text_rect = get_text_rect(name);

		if (button_theme.aligment & ALIGNMENT_RIGHT) {
			place_in_middle_and_by_right(&button_rect, &text_rect, button_theme.shift_from_size);
		} else if (button_theme.aligment & ALIGNMENT_LEFT) {
			place_in_middle_and_by_left(&button_rect, &text_rect, button_theme.shift_from_size);
		} else {
			place_in_center(&button_rect, &text_rect, BOTH_AXIS);
		}

		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&clip_rect);
		render_list->add_rect(&button_rect, button_color, button_theme.rounded_border);
		render_list->add_text(&text_rect, name);
		render_list->pop_clip_rect();
	}
	return button_down;
}

bool Gui_Manager::check_item(Gui_ID id, Rect_s32 *rect)
{
	bool result = false;
	if (detect_collision(rect)) {
		result = true;
		hot_item = id;
	} 
	return result;
}


inline u32 safe_sub_u32(u32 x, u32 y)
{
	u32 result = x - y;
	return ((result > x) && (result > y)) ? result : 0;
}

void Gui_Manager::init(Render_2D *_render_2d, Win32_Info *_win32_info, Font *_font)
{
	render_2d = _render_2d;
	win32_info = _win32_info;
	font = _font;
	edit_field_state.font = font;

	window_parent_count = 0;
	reset_window_params = 0;
	edit_field_count = 0;
	window_theme = default_window_theme;

	String path_to_save_file;
	build_full_path_to_gui_file("new_gui_data.gui", path_to_save_file);

	File save_file;
	if (!save_file.open(path_to_save_file, FILE_MODE_READ, FILE_OPEN_EXISTING)) {
		print("Gui_Manager::init: Hades gui file was not found.");
		return;
	}

	int window_count = 0;
	save_file.read((void *)&window_count, sizeof(int));

	for (int i = 0; i < window_count; i++) {
		int string_len = 0;
		save_file.read((void *)&string_len, sizeof(int));
		char *string = new char[string_len + 1];
		save_file.read((void *)string, string_len);
		string[string_len] = '\0';

		Rect_s32 rect;
		save_file.read((void *)&rect, sizeof(Rect_s32));

		print("[{}] Gui Window", i);
		print("name:", string);
		print("rect:", &rect);

		create_window(string, &rect);
		free_string(string);
	}
}

void Gui_Manager::shutdown()
{
	String path_to_save_file;
	build_full_path_to_gui_file("new_gui_data.gui", path_to_save_file);

	File save_file;
	if (!save_file.open(path_to_save_file, FILE_MODE_WRITE, FILE_CREATE_ALWAYS)) {
		print("Gui_Manager::shutdown: Hades gui data can not be save in file from path {}.", path_to_save_file);
		return;
	}

	save_file.write((void *)&window_parent_count, sizeof(int));

	Gui_Window *window = NULL;
	int i = 0;
	For(windows, window) {
		if (window->type == WINDOW_TYPE_PARENT) {

			save_file.write((void *)&window->name.len, sizeof(int));
			save_file.write((void *)&window->name.data[0], window->name.len);
			save_file.write((void *)&window->rect, sizeof(Rect_s32));
		}
	}
}

void Gui_Manager::new_frame()
{
	window_rect = default_window_rect;
	
	mouse_x = Mouse_Input::x;
	mouse_y = Mouse_Input::y;
	mouse_x_delta = mouse_x - last_mouse_x;
	mouse_y_delta = mouse_y - last_mouse_y;
	hot_item = 0;
	list_box_count = 0;
	edit_field_count = 0;
	became_just_focused = 0;
}

void Gui_Manager::end_frame()
{
	if (!is_left_mouse_button_down()) {
		active_item = 0;
		resizing_window = 0;
	}
	last_mouse_x = Mouse_Input::x;
	last_mouse_y = Mouse_Input::y;

	Gui_Window *window = NULL;
	For(windows_order, window) {
		render_2d->add_render_primitive_list(&window->render_list);
	}
}

void Gui_Manager::begin_window(const char *name, Window_Type window_type)
{	
	Gui_Window *window = NULL;
	window = find_window(name);
	
	if (!window) {
		window = create_window(name, window_type);
		became_just_focused = window->gui_id;
		
		if (window_type == WINDOW_TYPE_CHILD) {
			Gui_Window *parent_window = get_window();
			parent_window->child_windows.push(window);
		}
	}
	window_stack.push(window);
	
	window->next_place.x = window->content_rect.x;
	window->next_place.y = window->content_rect.y;
	window->alignment = 0;
	window->alignment |= ALIGNMENT_VERTICALLY;

	Rect_s32  *rect = &window->rect;
	if (check_item(window->gui_id, rect) && was_left_mouse_button_just_pressed()) {
		active_item = window->gui_id;
		became_just_focused = window->gui_id;
	}

	if (active_item == window->gui_id && is_left_mouse_button_down()) {
		s32 x = math::clamp(rect->x + mouse_x_delta, 0, (s32)win32_info->window_width - rect->width);
		s32 y = math::clamp(rect->y + mouse_y_delta, 0, (s32)win32_info->window_height - rect->height);
		window->set_position(x, y);
	}

	static Rect_Side rect_side;
	if ((active_item == 0) && (((resizing_window == window->gui_id) && is_left_mouse_button_down()) || detect_collision_window_borders(rect, &rect_side))) {
		active_item = 0;
		probably_resizing_window = window->gui_id;
		s32 old_width = rect->width;
		s32 old_height = rect->height;

		if ((rect_side == RECT_SIDE_LEFT) || (rect_side == RECT_SIDE_RIGHT)) {
			set_cursor(CURSOR_TYPE_RESIZE_LEFT_RIGHT);
		} else if ((rect_side == RECT_SIDE_BOTTOM) || (rect_side == RECT_SIDE_TOP)) {
			set_cursor(CURSOR_TYPE_RESIZE_TOP_BUTTOM);
		} else if (rect_side == RECT_SIDE_RIGHT_BOTTOM) {
			set_cursor(CURSOR_TYPE_RESIZE_TOP_LEFT);
		} else if (rect_side == RECT_SIDE_LEFT_BOTTOM) {
			set_cursor(CURSOR_TYPE_RESIZE_TOP_RIGHT);
		}

		if (was_left_mouse_button_just_pressed()) {
			resizing_window = window->gui_id;
		}

		if ((resizing_window == window->gui_id) && is_left_mouse_button_down()) {

			if ((rect_side == RECT_SIDE_LEFT) || (rect_side == RECT_SIDE_LEFT_BOTTOM)) {
				if ((mouse_x >= 0) && ((rect->width - mouse_x_delta) > MIN_WINDOW_WIDTH)) {
					rect->x = math::max(rect->x + mouse_x_delta, 0);
					rect->width = math::max(rect->width - mouse_x_delta, MIN_WINDOW_WIDTH);
				}
			} 
			if ((rect_side == RECT_SIDE_RIGHT) || (rect_side == RECT_SIDE_RIGHT_BOTTOM)) {
				if ((rect->right() + mouse_x_delta) < win32_info->window_width) {
					rect->width = math::max(rect->width + mouse_x_delta, MIN_WINDOW_WIDTH);
				}
			}
			if ((rect_side == RECT_SIDE_BOTTOM) || (rect_side == RECT_SIDE_RIGHT_BOTTOM) || (rect_side == RECT_SIDE_LEFT_BOTTOM)) {
				if ((rect->bottom() + mouse_y_delta) < win32_info->window_height) {
					rect->height = math::max(rect->height + mouse_y_delta, MIN_WINDOW_HEIGHT);
				}
			} 
			if (rect_side == RECT_SIDE_TOP) {
				if ((mouse_y >= 0) && ((rect->height - mouse_y_delta) > MIN_WINDOW_HEIGHT)) {
					rect->y = math::max(rect->y + mouse_y_delta, 0);
					rect->height = math::max(rect->height - mouse_y_delta, MIN_WINDOW_HEIGHT);
				}
			}
		}
	} else {
		if (probably_resizing_window == window->gui_id) {
			active_item = window->gui_id;
			probably_resizing_window = 0;
			set_cursor(CURSOR_TYPE_ARROW);
		}
	}

	if (reset_window_params & SET_WINDOW_POSITION) {
		window->set_position(window_rect.x, window_rect.y);
		reset_window_params &= ~SET_WINDOW_POSITION;
	}

	if (reset_window_params & SET_WINDOW_SIZE) {
		rect->set_size(window_rect.width, window_rect.height);
		reset_window_params &= ~SET_WINDOW_SIZE;
	}
	
	window->view_rect = *rect;

	Render_Primitive_List *render_list = GET_RENDER_LIST();
	render_list->add_outlines(rect->x, rect->y, rect->width, rect->height, window_theme.outlines_color, window_theme.outlines_width, window_theme.rounded_border);
	render_list->add_rect(&window->rect, window_theme.background_color, window_theme.rounded_border);
}

void Gui_Manager::end_window()
{
	Gui_Window *window = get_window();

	if (window->type == WINDOW_TYPE_PARENT) {
		int window_index;
		if (find_window_in_order(window->name, &window_index) == NULL) {
			windows_order.push(window);
			Gui_Window *child = NULL;
			For(window->child_windows, child) {
				windows_order.push(child);
			}
		} else {
			if (became_just_focused == window->gui_id) {
				windows_order.remove(window_index);
				windows_order.push(window);
			
				Gui_Window *child = NULL;
				For(window->child_windows, child) {
					windows_order.remove(window_index++);
					windows_order.push(child);
				}
			}
		}
	}

	
	window->content_rect.height += window_theme.place_between_elements;
	window->content_rect.width += window_theme.place_between_elements;
	
	bool draw_right_scroll_bar = false;
	bool draw_bottom_scroll_bar = false;
	Rect_s32 right_scroll_bar;
	Rect_s32 bottom_scroll_bar;
	
	if ((window->content_rect.height > window->view_rect.height) || (window->scroll[Y_AXIS] > window->view_rect.y)) {
		right_scroll_bar = window->get_scrollbar_rect(Y_AXIS);
		window->view_rect.width -= right_scroll_bar.width;
		draw_right_scroll_bar = true;
	}

	if ((window->content_rect.width > window->view_rect.width) || (window->scroll[X_AXIS] > window->view_rect.x)) {
		bottom_scroll_bar = window->get_scrollbar_rect(X_AXIS);
		window->view_rect.height -= bottom_scroll_bar.height;
		draw_bottom_scroll_bar = true;
	}

	if (draw_right_scroll_bar) {
		scroll_bar(window, Y_AXIS, &right_scroll_bar);
	}
	if (draw_bottom_scroll_bar) {
		scroll_bar(window, X_AXIS, &bottom_scroll_bar);
	}
	
	window->content_rect.set_size(0, 0);
	window_rect = default_window_rect;
	if (reset_window_params & SET_WINDOW_THEME) {
		reset_window_params &= ~SET_WINDOW_THEME;
		window_theme = default_window_theme;
	}

	window_stack.pop();
}

void Gui_Manager::same_line()
{
	Gui_Window *window = get_window();
	if (window->alignment & ALIGNMENT_VERTICALLY) {
		window->alignment &= ~ALIGNMENT_VERTICALLY;
	}
	window->alignment |= ALIGNMENT_HORIZONTALLY;
}

void Gui_Manager::next_line()
{
	Gui_Window *window = get_window();
	if (window->alignment & ALIGNMENT_HORIZONTALLY) {
		window->alignment &= ~ALIGNMENT_HORIZONTALLY;
	}
	window->alignment |= ALIGNMENT_VERTICALLY;
}

void Gui_Manager::scroll_bar(Gui_Window *window, Axis axis, Rect_s32 *scroll_bar)
{
	s32 window_size = window->view_rect.get_size()[axis];
	s32 content_size = window->content_rect.get_size()[axis];

	float ratio = (float)(window_size) / (float)content_size;
	s32 scroll_size = (ratio < 1.0f) ? (float)window_size  * ratio : window_size;

	s32 scroll_bar_width = 8;
	s32 scroll_offset = math::abs(window->scroll[axis] - window->view_rect[axis]);
	scroll_size = math::clamp(scroll_size, 10, math::abs(window->view_rect.get_size()[axis] - scroll_offset));

	Rect_s32 scroll_rect;
	if (axis == Y_AXIS) {
		scroll_rect = Rect_s32(scroll_bar->x, window->scroll[axis], scroll_bar_width, scroll_size);
	} else {
		scroll_rect = Rect_s32(window->scroll[axis], scroll_bar->y, scroll_size, scroll_bar_width);
	}

	String scroller = window->name;
	scroller.append("_scroller" + String((int)axis));

	Gui_ID scroller_id = fast_hash(scroller);

	if ((active_item == scroller_id) || check_rect_state(scroller, &scroll_rect, is_left_mouse_button_down)) {

		s32 window_side = (axis == Y_AXIS) ? window->view_rect.bottom() : window->view_rect.right();
		s32 mouse_delta = (axis == Y_AXIS) ? mouse_y_delta : mouse_x_delta;

		scroll_rect[axis] = math::clamp(scroll_rect[axis] + mouse_delta, window->view_rect[axis], window_side - scroll_rect.get_size()[axis]);
		
		s32 scroll_pos = scroll_rect[axis] - window->view_rect[axis];
		float scroll_ration = (float)scroll_pos / window->view_rect.get_size()[axis];
		s32 result = window->content_rect.get_size()[axis] * scroll_ration;
		window->content_rect[axis] = window->view_rect[axis] - result;
	}
	u32 flag = (axis == Y_AXIS) ? ROUND_RIGHT_RECT : ROUND_BOTTOM_RECT;

	Render_Primitive_List *render_list = GET_RENDER_LIST();
	render_list->add_rect(scroll_bar, Color(48, 50, 54), window_theme.rounded_border, flag);
	render_list->add_rect(&scroll_rect, Color(107, 114, 120), scroll_rect.get_size()[math::abs((int)axis - 1)] / 2);

	window->scroll[axis] = scroll_rect[axis];
}

bool Gui_Manager::detect_collision_window_borders(Rect_s32 *rect, Rect_Side *rect_side)
{
	static s32 offset_from_border = 5;
	static s32 tri_size = 10;
	
	Point_s32 mouse = { mouse_x, mouse_y };
	Triangle<s32> left_triangle = Triangle<s32>(Point_s32(rect->x, rect->bottom()), Point_s32(rect->x, rect->bottom() - tri_size), Point_s32(rect->x + tri_size, rect->bottom()));
	Triangle<s32> right_triangle = Triangle<s32>(Point_s32(rect->right() - tri_size, rect->bottom()), Point_s32(rect->right(), rect->bottom() - tri_size), Point_s32(rect->right(), rect->bottom()));

	if (detect_collision(&left_triangle, &mouse)) {
		*rect_side = RECT_SIDE_LEFT_BOTTOM;
		return true;
	} else if (detect_collision(&right_triangle, &mouse)) {
		*rect_side = RECT_SIDE_RIGHT_BOTTOM;
		return true;
	} else if ((mouse_x >= (rect->x - offset_from_border)) && (mouse_x <= rect->x) && ((mouse_y >= rect->y) && (mouse_y <= rect->bottom()))) {
		*rect_side = RECT_SIDE_LEFT;
		return true;
	} else if ((mouse_x >= rect->right()) && (mouse_x <= (rect->right() + offset_from_border)) && ((mouse_y >= rect->y) && (mouse_y <= rect->bottom()))) {
		*rect_side = RECT_SIDE_RIGHT;
		return true;
	} else if ((mouse_x >= rect->x) && (mouse_x <= rect->right() && (mouse_y >= rect->bottom()) && (mouse_y <= (rect->bottom() + offset_from_border)))) {
		*rect_side = RECT_SIDE_BOTTOM;
		return true;
	} else if ((mouse_x >= rect->x) && (mouse_x <= rect->right()) && (mouse_y <= rect->y) && (mouse_y >= (rect->y - offset_from_border))) {
		*rect_side = RECT_SIDE_TOP;
		return true;
	}
	return false;
}

static Gui_Manager gui_manager;


void same_line()
{
	gui_manager.same_line();
}

void next_line()
{
	gui_manager.next_line();
}

bool gui::button(const char *text)
{
	return gui_manager.button(text);
}

void gui::init_gui(Render_2D *render_2d, Win32_Info *win32_info, Font *font)
{
	gui_manager.init(render_2d, win32_info, font);
}

void gui::shutdown()
{
	gui_manager.shutdown();
}

void list_box(const char *strings[], u32 len, u32 *item_index)
{
	gui_manager.list_box(strings, len, item_index);
}

void radio_button(const char *name, bool *state)
{
	gui_manager.radio_button(name, state);
}

void edit_field(const char *name, int *value)
{
	gui_manager.edit_field(name, value);
}

void edit_field(const char *name, float *value)
{
	gui_manager.edit_field(name, value);
}

void edit_field(const char *name, String *value)
{
	gui_manager.edit_field(name, value);
}

void begin_frame()
{
	gui_manager.new_frame();
}

void end_frame()
{
	gui_manager.end_frame();
}

bool begin_window(const char *name)
{
	gui_manager.begin_window(name, WINDOW_TYPE_PARENT);
	return true;
}
void end_window()
{
	gui_manager.end_window();
}

void set_next_window_size(s32 width, s32 height)
{
	gui_manager.set_next_window_size(width, height);
}

void set_next_window_pos(s32 x, s32 y)
{
	gui_manager.set_next_window_pos(x, y);
}

struct Enum_String {
	s32 enum_id;
	String string_enum;
};

inline void handle_click(bool *state)
{
	*state = *state ? false : true;
}

void gui::draw_test_gui()
{

	begin_frame();

	//begin_window("Window1");
	//button("Window1");
	//end_window();

	//begin_window("Window2");
	//button("Window2");
	//end_window();
	
	//begin_window("Window3");
	//button("Window3");
	//end_window();

	
	if (begin_window("Test")) {

		const char *str[] = { "test1", "test2", "test3", "test4", "test5", "test6", "test7", };
		static u32 item_index = 123124;
		list_box(str, 7, &item_index);

		static bool state = false;
		static bool state1 = false;
		radio_button("Trun on light", &state1);
		radio_button("Render shadows", &state);
		//if (button("Click")) {
		//	print("Was click by bottom");
		//}
		static int position = 12345;
		edit_field("Position: x", &position);

		static int position1 = 85959;
		edit_field("Position: y", &position1);

		static float temp = 2345.234f;
		edit_field("Float x posiiton", &temp);

		static float temp1 = 0.0;
		edit_field("Float x0 posiiton", &temp1);

		static float temp2 = 10000.0;
		edit_field("Float x2 posiiton", &temp2);

		static float temp3 = 10000.1;
		edit_field("Float x3 posiiton", &temp3);
		
		button("next line1");
		button("next line2");
		button("next line3");
		button("next line4");
		button("next line5");
		button("next line6");
		button("next line7");
		button("next line8");
		button("next line9");

		//const char *str1[] = { "test1", "test2", "test3", "test4", "test5", "test6", "test7", };
		//static u32 item_index1 = 123124;
		//list_box(str1, 7, &item_index1);

		end_window();
	}

	//static u32 id = 0;
	//static bool entity_window = false;
	//if (begin_window("Entities")) {

	//	Entity_Manager *manager = &world.entity_manager;
	//	Entity *entity = NULL;
	//	
	//	#define to_string(x) #x
	//	
	//	For(manager->entities, entity) {
	//		String name = "entity id = " + String((int)entity->id);
	//		
	//		if (button(name.c_str())) {
	//			id = entity->id;
	//			handle_click(&entity_window);
	//		}

	//		if (entity_window) {
	//			begin_window("Entity");
	//			Entity *e = manager->find_entity(id);
	//			if (e) {
	//				edit_field("Entity position x", &e->position.x);
	//				edit_field("Entity position y", &e->position.y);
	//				edit_field("Entity position z", &e->position.z);
	//			}
	//			end_window();
	//		}
	//	}

	//	end_window();
	//}


	//	const char *str2[] = { "first2", "second2", "third2" };
	//	static u32 item_index2 = 0;
	//	list_box(str2, 3, &item_index2);

	end_frame();
}

