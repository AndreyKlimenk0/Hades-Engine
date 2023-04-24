#include <stdlib.h>
#include "gui.h"

#include "../win32/win_local.h"
#include "../win32/win_time.h"
#include "../win32/win_types.h"
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

#define PRINT_GUI_INFO 0
#define DRAW_WINDOW_DEBUG_RECTS 0
#define DRAW_CHILD_WINDOW_DEBUG_RECTS 0

typedef u32 Gui_ID;

const u32 BUTTON_HASH = fast_hash("button");
const u32 RADIO_BUTTON_HASH = fast_hash("radio_button");
const u32 LIST_BOX_HASH = fast_hash("list_box");
const u32 EDIT_FIELD_HASH = fast_hash("edit_Field");
const u32 SCROLL_BAR_HASH = fast_hash("scroll_bar");
const u32 TAB_HASH = fast_hash("window_tab_hash");

#define GET_BUTTON_GUI_ID() (window->gui_id + BUTTON_HASH + button_count)
#define GET_LIST_BOX_GUI_ID() (window->gui_id + LIST_BOX_HASH + list_box_count)
#define GET_EDIT_FIELD_GUI_ID() (window->gui_id + EDIT_FIELD_HASH + edit_field_count)
#define GET_RADIO_BUTTON_GUI_ID() (window->gui_id + RADIO_BUTTON_HASH + radio_button_count)
#define GET_SCROLL_BAR_GUI_ID() (window->gui_id + SCROLL_BAR_HASH)
#define GET_TAB_GUI_ID() (window->gui_id + TAB_HASH + tab_count)

#define GET_RENDER_LIST() (&window->render_list)

typedef u32 Element_Alignment;
const Element_Alignment HORIZONTALLY_ALIGNMENT = 0x01;
const Element_Alignment VERTICALLY_ALIGNMENT = 0x02;
const Element_Alignment RIGHT_ALIGNMENT = 0x04;
const Element_Alignment LEFT_ALIGNMENT = 0x08;
const Element_Alignment GO_TO_NEW_LINE = 0x10;
const Element_Alignment HORIZONTALLY_ALIGNMENT_JUST_SET = 0x20;
const Element_Alignment HORIZONTALLY_ALIGNMENT_ALREADY_SET = 0x40;
const Element_Alignment VERTICALLY_ALIGNMENT_JUST_SET = 0x80;
const Element_Alignment VERTICALLY_ALIGNMENT_WAS_USED = 0x100;

const s32 MIN_WINDOW_WIDTH = 20;
const s32 MIN_WINDOW_HEIGHT = 40;
const u32 GUI_FLOAT_PRECISION = 2;

const s32 TAB_HEIGHT = 26;
const s32 TAB_BAR_HEIGHT = 30;

const Rect_s32 DEFAULT_WINDOW_RECT = { 50, 50, 300, 300 };

const Gui_Window_Theme DEFAULT_WINDOW_THEME;
const Gui_Text_Button_Theme DEFAULT_BUTTON_THEME;

inline void reverse_state(bool *state)
{
	if (*state) {
		*state = false;
	} else {
		*state = true;
	}
}

inline bool detect_collision(Rect_s32 *rect)
{
	if ((Mouse_Input::x > rect->x) && (Mouse_Input::x < (rect->x + rect->width)) && (Mouse_Input::y > rect->y) && (Mouse_Input::y < (rect->y + rect->height))) {
		return true;
	}
	return false;
}

inline bool detect_collision(Rect_s32 *first_rect, Rect_s32 *second_rect)
{
	if ((first_rect->x < second_rect->right()) && (first_rect->right() > second_rect->x) && (first_rect->y < second_rect->bottom()) && (first_rect->bottom() > second_rect->y)) {
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

inline bool is_draw_caret(s32 blink_time)
{
	s64 show_time = blink_time / 2;
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

enum Axis {
	X_AXIS = 0,
	Y_AXIS = 1,
	BOTH_AXIS = 2
};

inline void place_in_middle(Rect_s32 *in_element_place, Rect_s32 *placed_element, Axis axis)
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
	place_in_middle(placing_element, placed_element, Y_AXIS);
}

inline void place_in_middle_and_by_right(Rect_s32 *placing_element, Rect_s32 *placed_element, int offset_from_right = 0)
{
	placed_element->x = placing_element->x + placing_element->width - placed_element->width - offset_from_right;
	place_in_middle(placing_element, placed_element, Y_AXIS);
}

enum Window_Type {
	WINDOW_TYPE_PARENT,
	WINDOW_TYPE_CHILD,
};

struct Gui_Window {
	bool tab_was_added = false;
	bool tab_was_drawn;
	Gui_ID gui_id;
	s32 index_in_windows_array = -1;
	s32 index_in_windows_order = -1;
	s32 prev_rect_height = 0;
	s32 tab_offset = 0;

	Window_Type type;
	Element_Alignment alignment;
	Window_Style style;

	Rect_s32 rect;
	Rect_s32 view_rect;
	Rect_s32 content_rect;
	Point_s32 rect_place;
	Point_s32 scroll;
	String name;
	
	Array<Gui_Window *> child_windows;
	Array<Gui_Window *> collided_windows;
	Render_Primitive_List render_list;

	void start_new_frame(Window_Style window_style);
	void set_position(s32 x, s32 y);
	void set_index_with_offset(s32 index);
	u32 get_index_with_offset();
	Rect_s32 get_scrollbar_rect(Axis axis);

	void place_rect_over_window(Rect_s32 *new_rect);
};

inline void draw_debug_rect(Gui_Window *window, Rect_s32 *rect, Color color = Color::Red)
{
	Render_Primitive_List *list = &window->render_list;
	color.value.w = 0.2f;
	list->add_rect(rect, color);
}

void Gui_Window::place_rect_over_window(Rect_s32 *additional_rect)
{
	static Point_s32 view_point = { 0, 0 };
	if ((additional_rect->y == view_rect.y) && (additional_rect->width == view_rect.width) && (additional_rect->height != view_rect.height)) {
		view_rect.offset_y(additional_rect->height);
	} else if ((additional_rect->y == view_rect.y) && (additional_rect->height == view_rect.height) && (additional_rect->width != view_rect.width)) {
		view_rect.width -= additional_rect->width;
	} else if ((additional_rect->bottom() == rect.bottom()) && (additional_rect->width == view_rect.width) && (additional_rect->height != view_rect.height)) {
		view_rect.height -= additional_rect->height;
	} else {
		print("Gui_Window::add_rect_on_window: There is no an option to add {} to window {}.", additional_rect, name);
	}

	if (view_rect.y > view_point.y) {
		view_point.y = view_rect.y;
		content_rect.y = view_rect.y;
		scroll[Y_AXIS] = view_rect.y;
	}
	if (view_rect.x > view_point.x) {
		view_point.x = view_rect.x;
		content_rect.x = view_rect.x;
		scroll[X_AXIS] = view_rect.x;
	}
}

void Gui_Window::start_new_frame(Window_Style window_style)
{
	view_rect = rect;
	tab_was_drawn = false;
	tab_offset = view_rect.x;
	style = window_style;
	rect_place.x = content_rect.x;
	rect_place.y = content_rect.y;
	alignment = VERTICALLY_ALIGNMENT | VERTICALLY_ALIGNMENT_JUST_SET;
	prev_rect_height = 0;
}

inline void Gui_Window::set_position(s32 x, s32 y)
{
	s32 old_win_y = rect.y;
	s32 old_win_x = rect.x;

	rect.set(x, y);
	view_rect.set(x, y);

	content_rect.x += rect.x - old_win_x;
	content_rect.y += rect.y - old_win_y;

	rect_place.x = content_rect.x;
	rect_place.y = content_rect.y;

	scroll.x += rect.x - old_win_x;
	scroll.y += rect.y - old_win_y;
}

inline void Gui_Window::set_index_with_offset(s32 index)
{
	index_in_windows_order = index + 1;
}

inline u32 Gui_Window::get_index_with_offset()
{
	return index_in_windows_order - 1;;
}

inline Rect_s32 Gui_Window::get_scrollbar_rect(Axis axis)
{
	const static s32 scroll_bar_width = 8;
	if (axis == Y_AXIS) {
		return Rect_s32(rect.right() - scroll_bar_width, view_rect.y, scroll_bar_width, view_rect.height);
	} 
	return Rect_s32(view_rect.x, view_rect.bottom() - scroll_bar_width, view_rect.width, scroll_bar_width);
}

struct Edit_Field_Instance {
	const char *name = NULL;
	const char *editing_value = NULL;
	bool (*symbol_validation)(char symbol) = NULL;

	u32 max_chars_number = 0;
	Rect_s32 caret_rect;
	Rect_s32 rect;
	Rect_s32 edit_field_rect;
	Rect_s32 name_rect;
	Rect_s32 value_rect;
};

struct Edit_Field_State {
	bool(*symbol_validation)(char symbol);
	u32 max_symbol_number;
	s32 caret_index_in_text; // this caret index specifies get character is placed befor the caret.
	s32 caret_index_for_inserting; // this caret index specifies get character is placed after the caret.
	String data;
	Rect_s32 caret;
};

Edit_Field_State make_edit_field(Rect_s32 *caret, const char *str_value, u32 max_text_width, bool(*symbol_validation)(char symbol))
{
	Edit_Field_State edit_field;
	edit_field.data = str_value;
	edit_field.caret = *caret;
	edit_field.max_symbol_number = max_text_width;
	edit_field.caret_index_for_inserting = edit_field.data.len;
	edit_field.caret_index_in_text = edit_field.data.len - 1;
	edit_field.symbol_validation = symbol_validation;
	return edit_field;
}

const u32 SET_WINDOW_POSITION = 0x1;
const u32 SET_WINDOW_SIZE = 0x2;
const u32 SET_WINDOW_THEME = 0x4;

#define METHOD_PTR(method_name) (Gui_Manager::*method_name)

struct Gui_Manager {
	bool is_window_order_update;
	bool any_window_was_moved;
	bool handle_events_for_one_window;

	s32 mouse_x;
	s32 mouse_y;
	s32 last_mouse_x;
	s32 last_mouse_y;
	s32 mouse_x_delta;
	s32 mouse_y_delta;

	u32 tab_count;
	u32 button_count;
	u32 radio_button_count;
	u32 list_box_count;
	u32 edit_field_count;
	u32 reset_window_params;
	u32 window_parent_count;

	s32 curr_parent_windows_index_sum;
	s32 prev_parent_windows_index_sum;

	Gui_ID hot_item;
	Gui_ID active_item;
	Gui_ID resizing_window;
	Gui_ID active_list_box;
	Gui_ID active_edit_field;
	Gui_ID active_tab;
	Gui_ID focused_window;
	Gui_ID became_just_focused; //window
	Gui_ID probably_resizing_window;
	Gui_ID window_events_handler_id;

	Font *font = NULL;
	Win32_Info *win32_info = NULL;
	//2D render api
	Render_2D *render_2d = NULL;

	Rect_s32 window_rect;
	Cursor_Type cursor_type;
	
	Array<Gui_Window> windows;
	Stack<u32> window_stack;
	Array<u32> windows_order;

	Gui_Tab_Theme tab_theme;
	Gui_Window_Theme window_theme;
	Gui_Window_Theme backup_window_theme;
	Gui_Window_Theme future_window_theme;
	Gui_Text_Button_Theme button_theme;
	Gui_Edit_Field_Theme edit_field_theme;
	Gui_Radio_Button_Theme radio_button_theme;

	Edit_Field_State edit_field_state;

	void handle_events(bool *update_editing_value, bool *update_next_time_editing_value, Rect_s32 *rect, Rect_s32 *editing_value_rect);

	void init(Render_2D *_render_2d, Win32_Info *_win32_info, Font *_font);
	void shutdown();
	
	void new_frame();
	void end_frame();

	void update_active_and_hot_state(Gui_ID gui_id, Rect_s32 *rect);
	void update_active_and_hot_state(Gui_Window *window, u32 rect_gui_id, Rect_s32 *rect);

	void begin_window(const char *name, Window_Type window_type, Window_Style window_style);
	void end_window();
	void begin_child(const char *name, Window_Style window_style);
	void end_child();

	void same_line();
	void next_line();
	void set_next_window_pos(s32 x, s32 y);
	void set_next_window_size(s32 width, s32 height);
	void set_next_window_theme(Gui_Window_Theme *theme);
	
	void place_rect_in_window(Gui_Window *window, Rect_s32 *rect);

	void set_caret_position_on_mouse_click(Rect_s32 *rect, Rect_s32 *editing_value_rect);

	void text(const char *some_text);
	bool add_tab(const char *tab_name);
	void image(Texture2D *texture, s32 width, s32 height);
	void list_box(Array<String> *array, u32 *item_index);
	void scroll_bar(Gui_Window *window, Axis axis, Rect_s32 *scroll_bar);
	bool radio_button(const char *name, bool *state);
	
	void edit_field(const char *name, int *value);
	void edit_field(const char *name, float *value);
	void edit_field(const char *name, String *value);
	bool edit_field(const char *name, Vector3 *vector, const char *x, const char *y, const char *z);
	bool edit_field(const char *name, const char *editing_value, u32 max_chars_number, bool(*symbol_validation)(char symbol));
	bool edit_field(const char *name, const char *editing_value, u32 max_chars_number, bool(*symbol_validation)(char symbol), const Color &color);
	
	bool update_edit_field(Edit_Field_Instance *edit_field_instance);
	void draw_edit_field(Edit_Field_Instance *edit_field_instance);
	
	bool button(const char *name, bool *state = NULL);

	bool can_window_be_resized(Gui_Window *window);
	bool detect_collision_window_borders(Rect_s32 *rect, Rect_Side *rect_side);

	Rect_s32 get_win32_rect();
	Rect_s32 get_text_rect(const char *text);

	Gui_Window *get_window();
	Gui_Window *find_window(const char *name, u32 *window_index);
	Gui_Window *find_window_in_order(const char *name, int *window_index);

	Gui_Window *create_window(const char *name, Window_Type window_type, Window_Style window_style);
	Gui_Window *create_window(const char *name, Window_Type window_type, Window_Style window_style, Rect_s32 *rect);

	Gui_Window *get_window_by_index(Array<u32> *window_indices, u32 index);
};

Rect_s32 Gui_Manager::get_text_rect(const char *text)
{
	Rect_s32 name_rect;
	name_rect.x = 0;
	name_rect.y = 0;
	Size_u32 size = font->get_text_size(text);
	name_rect.width = (s32)size.width;
	name_rect.height = (s32)size.height;
	return name_rect;
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
	future_window_theme = *theme;
	reset_window_params |= SET_WINDOW_THEME;;
}

void Gui_Manager::place_rect_in_window(Gui_Window *window, Rect_s32 *rect)
{
	assert(!((window->alignment & VERTICALLY_ALIGNMENT) && (window->alignment & HORIZONTALLY_ALIGNMENT)));

	u32 place_between_elements = window_theme.place_between_elements;

	if (window->alignment & VERTICALLY_ALIGNMENT) {
		window->alignment |= VERTICALLY_ALIGNMENT_WAS_USED;
		
		if (window->alignment & VERTICALLY_ALIGNMENT_JUST_SET) {
			window->alignment &= ~VERTICALLY_ALIGNMENT_JUST_SET;
			
			window->rect_place.x = window->content_rect.x + place_between_elements;
			window->rect_place.y += window->prev_rect_height;
			window->content_rect.height += window->prev_rect_height;
		}
		rect->x = window->rect_place.x;
		rect->y = window->rect_place.y + place_between_elements;
		
		window->rect_place.y += rect->height + place_between_elements;
		window->content_rect.height += rect->height + place_between_elements;
		
		if (rect->width > window->content_rect.width) {
			window->content_rect.width = rect->width;
		}
		window->prev_rect_height = rect->height;
	} 
	
	static s32 max_window_content_width = 0;
	if (window->alignment & HORIZONTALLY_ALIGNMENT) {
		if (window->alignment & HORIZONTALLY_ALIGNMENT_JUST_SET) {
			window->alignment &= ~HORIZONTALLY_ALIGNMENT_JUST_SET;
			max_window_content_width = 0;

			if (!(window->alignment & VERTICALLY_ALIGNMENT_WAS_USED)) {
				window->rect_place.y += window->prev_rect_height;
				window->content_rect.height += window->prev_rect_height;
			}
			if (window->alignment & HORIZONTALLY_ALIGNMENT_ALREADY_SET) {
				window->alignment &= ~HORIZONTALLY_ALIGNMENT_ALREADY_SET;
				
				window->rect_place.y += window->prev_rect_height;
				window->content_rect.height += window->prev_rect_height;
			}
			
			window->rect_place.y += place_between_elements;
			window->rect_place.x = window->content_rect.x + place_between_elements;
			window->content_rect.height += place_between_elements;
			
			rect->y = window->rect_place.y;
			rect->x = window->rect_place.x;
			window->rect_place.x += rect->width;
			
			max_window_content_width += rect->width;
			if (max_window_content_width > window->content_rect.width) {
				window->content_rect.width += max_window_content_width - window->content_rect.width;
			}
			return;
		}
		rect->x = window->rect_place.x + place_between_elements;
		rect->y = window->rect_place.y;
		window->rect_place.x += rect->width + place_between_elements;
		window->prev_rect_height = rect->height;

		max_window_content_width += rect->width + place_between_elements;
		if (max_window_content_width > window->content_rect.width) {
			window->content_rect.width += max_window_content_width - window->content_rect.width;
		}
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
	return &windows[window_stack.top()];
}

Gui_Window *Gui_Manager::find_window(const char *name, u32 *window_index)
{
	Gui_ID window_id = fast_hash(name);

	for (u32 i = 0; i < windows.count; i++) {
		if (name == windows[i].name) {
			*window_index = i;
			return &windows[i];
		}
	}
	return NULL;
}

Gui_Window *Gui_Manager::find_window_in_order(const char *name, int *window_index)
{
	Gui_ID window_id = fast_hash(name);
	
	for (u32 i = 0; i < windows_order.count; i++) {
		Gui_Window *window = get_window_by_index(&windows_order, i);
		if (name == window->name) {
			*window_index = i;
			return window;
		}
	}
	return NULL;
}

Gui_Window *Gui_Manager::create_window(const char *name, Window_Type window_type, Window_Style window_style)
{
	Gui_Window window;
	window.name = name;
	window.gui_id = fast_hash(name);
	window.type = window_type;
	window.rect = window_rect;
	window.view_rect = window_rect;
	window.alignment = VERTICALLY_ALIGNMENT | VERTICALLY_ALIGNMENT_JUST_SET;
	window.content_rect.set(window_rect.x, window_rect.y);
	window.scroll = { window_rect.x, window_rect.y };

	window.render_list = Render_Primitive_List(render_2d);

	windows.push(window);
	window_rect.x += window.rect.width + 40;

	if (window_type == WINDOW_TYPE_PARENT) {
		window_parent_count++;
	}

	return &windows.last_item();
}

Gui_Window *Gui_Manager::create_window(const char *name, Window_Type window_type, Window_Style window_style, Rect_s32 *rect)
{
	Gui_Window window;
	window.name = name;
	window.gui_id = fast_hash(name);
	window.type = window_type;
	window.rect = *rect;
	window.view_rect = *rect;
	window.style = window_style;
	window.alignment = 0;
	window.alignment = VERTICALLY_ALIGNMENT | VERTICALLY_ALIGNMENT_JUST_SET;
	window.content_rect.set(rect->x, rect->y);
	window.scroll = { rect->x, rect->y };

	window.render_list = Render_Primitive_List(render_2d);

	window.index_in_windows_array = windows.count;
	windows.push(window);

	if (window_type == WINDOW_TYPE_PARENT) {
		window_parent_count++;
	}

	return &windows.last_item();
}

inline Gui_Window *Gui_Manager::get_window_by_index(Array<u32> *window_indices, u32 index)
{
	u32 window_index = window_indices->get(index);
	return &windows[window_index];
}

void Gui_Manager::update_active_and_hot_state(Gui_ID gui_id, Rect_s32 *rect)
{
	if (detect_collision(rect)) {
		hot_item = gui_id;
		if (was_left_mouse_button_just_pressed()) {
			active_item = gui_id;
		}
	}
}

void Gui_Manager::update_active_and_hot_state(Gui_Window *window, u32 rect_gui_id, Rect_s32 *rect)
{
	if (handle_events_for_one_window) {
		if (window->gui_id == window_events_handler_id) {
			update_active_and_hot_state(rect_gui_id, rect);
		} 
	} else {
		update_active_and_hot_state(rect_gui_id, rect);
	}
}

inline bool must_rect_be_drawn(Rect_s32 *win_rect, Rect_s32 *item_rect)
{
	if ((item_rect->x > win_rect->right()) || (item_rect->y > win_rect->bottom())) {
		return false;
	}
	return true;
}

Rect_s32 calculate_clip_rect(Rect_s32 *win_rect, Rect_s32 *item_rect)
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

bool Gui_Manager::radio_button(const char *name, bool *state)
{
	bool was_click = false;
	Rect_s32 true_rect = radio_button_theme.true_rect;
	Rect_s32 radio_rect = radio_button_theme.radio_rect;
	Rect_s32 rect = radio_button_theme.rect;
	Rect_s32 name_rect = get_text_rect(name);
	rect.width = name_rect.width + radio_rect.width + radio_button_theme.text_shift;

	Gui_Window *window = get_window();
	place_rect_in_window(window, &rect);

	if (must_rect_be_drawn(&window->rect, &rect)) {
		Rect_s32 clip_rect = calculate_clip_rect(&window->view_rect, &rect);

		place_in_middle_and_by_left(&rect, &name_rect, radio_rect.width + radio_button_theme.text_shift);
		place_in_middle_and_by_left(&rect, &radio_rect, 0);
		place_in_middle(&radio_rect, &true_rect, BOTH_AXIS);

		Gui_ID radio_button_gui_id = GET_RADIO_BUTTON_GUI_ID();
		update_active_and_hot_state(window, radio_button_gui_id, &radio_rect);

		if ((hot_item == radio_button_gui_id) && (was_click_by_left_mouse_button())) {
			was_click = true;
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
		render_list->add_text(&name_rect, name);
		render_list->pop_clip_rect();
	}
	radio_button_count++;
	return was_click;
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
	if (edit_field(name, str_value, 10, &is_symbol_int_valid)) {
		*value = atoi(edit_field_state.data.c_str());
	}
	free_string(str_value);
}

void Gui_Manager::edit_field(const char *name, float *value)
{
	char *str_value = to_string(*value, GUI_FLOAT_PRECISION);
	if (edit_field(name, str_value, 15, &is_symbol_float_valid)) {
		*value = (float)atof(edit_field_state.data.c_str());
	}
	free_string(str_value);
}

void Gui_Manager::edit_field(const char *name, String *value)
{
	assert(false);
}

bool Gui_Manager::edit_field(const char *name, Vector3 *vector, const char *x, const char *y, const char *z)
{
	bool was_data_updated = false;
	u32 color = 180;
	char *x_str = to_string(vector->x, GUI_FLOAT_PRECISION);
	char *y_str = to_string(vector->y, GUI_FLOAT_PRECISION);
	char *z_str = to_string(vector->z, GUI_FLOAT_PRECISION);
	
	same_line();
	if (edit_field(x, x_str, 12, &is_symbol_float_valid, Color(color, 0, 0))) {
		vector->x = (float)atof(edit_field_state.data.c_str());
		was_data_updated = true;
	}
	if (edit_field(y, y_str, 12, &is_symbol_float_valid, Color(0, color, 0))) {
		vector->y = (float)atof(edit_field_state.data.c_str());
		was_data_updated = true;
	}
	if (edit_field(z, z_str, 12, &is_symbol_float_valid, Color(0, 0, color))) {
		vector->z = (float)atof(edit_field_state.data.c_str());
		was_data_updated = true;
	}
	text(name);
	next_line();

	free_string(x_str);
	free_string(y_str);
	free_string(z_str);
	return was_data_updated;
}

bool Gui_Manager::edit_field(const char *name, const char *editing_value, u32 max_chars_number, bool(*symbol_validation)(char symbol), const Color &color)
{
	Gui_Edit_Field_Theme *theme = &edit_field_theme;

	Edit_Field_Instance edit_field_instance;
	edit_field_instance.name = (const char *)name;
	edit_field_instance.editing_value = editing_value;
	edit_field_instance.symbol_validation = symbol_validation;
	edit_field_instance.max_chars_number = max_chars_number;
	edit_field_instance.caret_rect = { 0, 0, 1, 14 };
	edit_field_instance.rect = { 0, 0, 80, 20 };
	edit_field_instance.edit_field_rect = edit_field_instance.rect;
	edit_field_instance.name_rect = get_text_rect(name);
	edit_field_instance.value_rect = get_text_rect(editing_value);

	Rect_s32 color_rect = { 0, 0, (s32)font->get_text_width(name) + 12, 20 };
	edit_field_instance.edit_field_rect.width -= color_rect.width;
	
	Gui_Window *window = get_window();
	place_rect_in_window(window, &edit_field_instance.rect);

	bool update_value = false;
	if (must_rect_be_drawn(&window->rect, &edit_field_instance.rect)) {
		place_in_middle_and_by_left(&edit_field_instance.rect, &color_rect);
		place_in_middle_and_by_left(&edit_field_instance.rect, &edit_field_instance.edit_field_rect, color_rect.width);
		place_in_middle(&color_rect, &edit_field_instance.name_rect, BOTH_AXIS);
		place_in_middle_and_by_left(&edit_field_instance.edit_field_rect, &edit_field_instance.value_rect, theme->text_shift);
		place_in_middle_and_by_left(&edit_field_instance.value_rect, &edit_field_instance.caret_rect, edit_field_instance.value_rect.width);

		update_value = update_edit_field(&edit_field_instance);

		Render_Primitive_List *render_list = GET_RENDER_LIST();
		Rect_s32 clip_rect = calculate_clip_rect(&window->rect, &edit_field_instance.rect);
		render_list->push_clip_rect(&clip_rect);

		render_list->add_rect(&edit_field_instance.rect, edit_field_theme.color, edit_field_theme.rounded_border);
		render_list->add_rect(&color_rect, color, edit_field_theme.rounded_border);
		render_list->add_text(&edit_field_instance.name_rect, edit_field_instance.name);

		Gui_ID edit_field_gui_id = GET_EDIT_FIELD_GUI_ID();
		if (active_edit_field == edit_field_gui_id) {
			render_list->add_rect(&edit_field_state.caret, Color::White);
		}

		if (!edit_field_state.data.is_empty() && (active_edit_field == edit_field_gui_id)) {
			render_list->add_text(&edit_field_instance.value_rect, edit_field_state.data);
		} else {
			render_list->add_text(&edit_field_instance.value_rect, edit_field_instance.editing_value);
		}

		render_list->pop_clip_rect();
	}
	edit_field_count++;
	
	return update_value;
}

bool Gui_Manager::edit_field(const char *name, const char *editing_value, u32 max_chars_number, bool(*symbol_validation)(char symbol))
{
	Gui_Edit_Field_Theme *theme = &edit_field_theme;
	
	Edit_Field_Instance edit_field_instance;
	edit_field_instance.name = name;
	edit_field_instance.editing_value = editing_value;
	edit_field_instance.symbol_validation = symbol_validation;
	edit_field_instance.max_chars_number = max_chars_number;
	edit_field_instance.caret_rect = { 0, 0, 1, 14 };
	edit_field_instance.rect = theme->rect;
	edit_field_instance.edit_field_rect = theme->edit_field_rect;
	edit_field_instance.name_rect = get_text_rect(name);
	edit_field_instance.value_rect = get_text_rect(editing_value);
	edit_field_instance.rect.width = edit_field_instance.name_rect.width + edit_field_instance.edit_field_rect.width + theme->text_shift;
	
	Gui_Window *window = get_window();
	place_rect_in_window(window, &edit_field_instance.rect);
	
	bool update_value = false;
	if (must_rect_be_drawn(&window->rect, &edit_field_instance.rect)) {
		place_in_middle_and_by_left(&edit_field_instance.rect, &edit_field_instance.edit_field_rect, 0);
		place_in_middle_and_by_left(&edit_field_instance.rect, &edit_field_instance.name_rect, edit_field_instance.edit_field_rect.width + theme->text_shift);
		place_in_middle_and_by_left(&edit_field_instance.edit_field_rect, &edit_field_instance.value_rect, theme->text_shift);
		place_in_middle_and_by_left(&edit_field_instance.value_rect, &edit_field_instance.caret_rect, edit_field_instance.value_rect.width);

		update_value = update_edit_field(&edit_field_instance);
		draw_edit_field(&edit_field_instance);
	}
	edit_field_count++;
	return update_value;
}

bool Gui_Manager::update_edit_field(Edit_Field_Instance *edit_field_instance)
{
	bool update_editing_value = false;
	static bool update_next_time_editing_value = false;
	
	Gui_Window *window = get_window();

	Gui_ID edit_field_gui_id = GET_EDIT_FIELD_GUI_ID();
	update_active_and_hot_state(window, edit_field_gui_id, &edit_field_instance->edit_field_rect);

	if (was_click_by_left_mouse_button()) {
		if (hot_item == edit_field_gui_id) {
			active_edit_field = edit_field_gui_id;
			edit_field_state = make_edit_field(&edit_field_instance->caret_rect, edit_field_instance->editing_value, edit_field_instance->max_chars_number, edit_field_instance->symbol_validation);
		} else {
			if (active_edit_field == edit_field_gui_id) {
				active_edit_field = 0;
				update_editing_value = true;
			}
		}
	}

	if (active_edit_field == edit_field_gui_id) {
		if (update_next_time_editing_value) {
			edit_field_state.data = edit_field_instance->editing_value;
			update_next_time_editing_value = false;
		}
		handle_events(&update_editing_value, &update_next_time_editing_value, &edit_field_instance->edit_field_rect, &edit_field_instance->value_rect);
	}
	return update_editing_value;
}

void Gui_Manager::draw_edit_field(Edit_Field_Instance *edit_field_instance)
{
	Gui_Window *window = get_window();
	Gui_Edit_Field_Theme *theme = &edit_field_theme;
	Gui_ID edit_field_gui_id = GET_EDIT_FIELD_GUI_ID();
	Render_Primitive_List *render_list = GET_RENDER_LIST();
	
	Rect_s32 clip_rect = calculate_clip_rect(&window->view_rect, &edit_field_instance->rect);
	render_list->push_clip_rect(&clip_rect);
	render_list->add_rect(&edit_field_instance->edit_field_rect, theme->color, theme->rounded_border);
	render_list->add_text(&edit_field_instance->name_rect, edit_field_instance->name);

	if (active_edit_field == edit_field_gui_id) {
		render_list->add_rect(&edit_field_state.caret, Color::White);
	}
	if (!edit_field_state.data.is_empty() && (active_edit_field == edit_field_gui_id)) {
		render_list->add_text(&edit_field_instance->value_rect, edit_field_state.data);
	} else {
		render_list->add_text(&edit_field_instance->value_rect, edit_field_instance->editing_value);
	}
	render_list->pop_clip_rect();
}

void Gui_Manager::handle_events(bool *update_editing_value, bool *update_next_time_editing_value, Rect_s32 *rect, Rect_s32 *editing_value_rect)
{
	Queue<Event> *events = get_event_queue();
	for (Queue_Node<Event> *node = events->first; node != NULL; node = node->next) {
		Event *event = &node->item;
		
		if (event->type == EVENT_TYPE_KEY) {
			if (was_click_by_left_mouse_button()) {
				set_caret_position_on_mouse_click(rect, editing_value_rect);
			}
			if (event->is_key_down(VK_BACK)) {
				if (edit_field_state.caret_index_in_text > -1) {

					char c = edit_field_state.data[edit_field_state.caret_index_in_text];
					edit_field_state.data.remove(edit_field_state.caret_index_in_text);

					u32 char_width = font->get_char_width(c);
					edit_field_state.caret.x -= (s32)char_width;

					edit_field_state.caret_index_in_text -= 1;
					edit_field_state.caret_index_for_inserting -= 1;
				}

			} else if (event->is_key_down(VK_LEFT)) {
				if (edit_field_state.caret_index_in_text > -1) {

					char c = edit_field_state.data[edit_field_state.caret_index_in_text];
					u32 char_width = font->get_char_width(c);
					if (c == '.') {
						char_width = font->get_char_advance(c);
					}
					edit_field_state.caret.x -= (s32)char_width;

					edit_field_state.caret_index_in_text -= 1;
					edit_field_state.caret_index_for_inserting -= 1;
				}
			} else if (event->is_key_down(VK_RIGHT)) {
				if (edit_field_state.caret_index_in_text < (s32)edit_field_state.data.len) {

					edit_field_state.caret_index_in_text += 1;
					edit_field_state.caret_index_for_inserting += 1;

					char c = edit_field_state.data[edit_field_state.caret_index_in_text];
					u32 char_width = font->get_char_width(c);
					if (c == '.') {
						char_width = font->get_char_advance(c);
					}
					edit_field_state.caret.x += (s32)char_width;
				}
			} else if (event->is_key_down(VK_HOME)) {
				Size_u32 size = font->get_text_size(edit_field_state.data);
				edit_field_state.caret.x = editing_value_rect->x;
				edit_field_state.caret_index_for_inserting = 0;
				edit_field_state.caret_index_in_text = -1;

			} else if (event->is_key_down(VK_END)) {
				Size_u32 size = font->get_text_size(edit_field_state.data);
				edit_field_state.caret.x = editing_value_rect->x + size.width;
				edit_field_state.caret_index_for_inserting = edit_field_state.data.len;
				edit_field_state.caret_index_in_text = edit_field_state.data.len - 1;

			} else if (event->is_key_down(VK_RETURN)) {
				*update_editing_value = true;
				*update_next_time_editing_value = true;
			}
		} else if (event->type == EVENT_TYPE_CHAR) {
			if ((edit_field_state.max_symbol_number > edit_field_state.data.len) && edit_field_state.symbol_validation(event->char_key)) {

				int point_index = edit_field_state.data.find_text(".");
				if ((point_index != -1) && ((edit_field_state.data.len - (point_index + 1)) == edit_field_theme.float_precision) && (edit_field_state.caret_index_in_text == (edit_field_state.data.len - 1))) {
					return;
				}

				// The 'if' was added from float edit_field in order to not let add more then one '-' symbol
				//@Note: May be 'if' should be placed in another place
				if ((event->char_key == '-') && (edit_field_state.data.len > 0) && (edit_field_state.data[0] == '-')) {
					return;
				}

				if (edit_field_state.caret_index_in_text == (edit_field_state.data.len - 1)) {
					edit_field_state.data.append(event->char_key);
				} else {
					edit_field_state.data.insert(edit_field_state.caret_index_for_inserting, event->char_key);
				}
				edit_field_state.caret_index_in_text += 1;
				edit_field_state.caret_index_for_inserting += 1;

				u32 char_width = font->get_char_width(event->char_key);
				edit_field_state.caret.x += (s32)char_width;
			}
		}
	}
}

void Gui_Manager::set_caret_position_on_mouse_click(Rect_s32 *rect, Rect_s32 *editing_value_rect)
{
	u32 text_width = font->get_text_size(edit_field_state.data).width;
	u32 mouse_x_relative_text = (u32)math::abs(mouse_x - rect->x - edit_field_theme.text_shift);

	if (mouse_x_relative_text > text_width) {
		edit_field_state.caret.x = editing_value_rect->x + text_width;
		edit_field_state.caret_index_for_inserting = edit_field_state.data.len;
		edit_field_state.caret_index_in_text = edit_field_state.data.len - 1;
		return;
	}

	if ((mouse_x >= rect->x) && (mouse_x <= (rect->x + edit_field_theme.text_shift))) {
		edit_field_state.caret.x = rect->x + edit_field_theme.text_shift;
		edit_field_state.caret_index_for_inserting = 0;
		edit_field_state.caret_index_in_text = 0;
		return;
	}

	u32 chars_width = 0;
	u32 chars_advance_width = 0;
	u32 prev_chars_advance_width = 0;

	String *text = &edit_field_state.data;
	for (u32 i = 0; i < text->len; i++) {
		char c = text->data[i];
		if (c == '.') {
			chars_width += font->get_char_advance(c);
		} else {
			chars_width += font->get_char_width(c);
		}
		chars_advance_width += font->get_char_advance(c);

		if ((mouse_x_relative_text >= prev_chars_advance_width) && (mouse_x_relative_text <= chars_advance_width)) {
			edit_field_state.caret.x = rect->x + edit_field_theme.text_shift + chars_width;
			edit_field_state.caret_index_for_inserting = i + 1;
			edit_field_state.caret_index_in_text = i;
			break;
		}
	}
}

void Gui_Manager::text(const char *some_text)
{
	Gui_Window *window = get_window();

	Rect_s32 text_rect = get_text_rect(some_text);
	text_rect.height = 20;
	Rect_s32 alignment_text_rect = get_text_rect(some_text);

	place_rect_in_window(window, &text_rect);

	if (must_rect_be_drawn(&window->rect, &text_rect)) {
		place_in_middle(&text_rect, &alignment_text_rect, BOTH_AXIS);
		
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		Rect_s32 clip_rect = calculate_clip_rect(&window->rect, &text_rect);
		render_list->push_clip_rect(&clip_rect);
		render_list->add_text(&alignment_text_rect, some_text);
		render_list->pop_clip_rect();
	}
}

bool Gui_Manager::add_tab(const char *tab_name)
{
	Gui_Window *window = get_window();

	Rect_s32 name_rect = get_text_rect(tab_name);
	Rect_s32 tab_rect = { 0, 0, name_rect.width + tab_theme.additional_space_in_tab, tab_theme.tab_height };
	Rect_s32 tab_bar_rect = { window->view_rect.x, window->view_rect.y, window->rect.width, tab_theme.tab_bar_height };

	if (!window->tab_was_drawn) {
		window->place_rect_over_window(&tab_bar_rect);
	}
	tab_rect.set(window->tab_offset, window->view_rect.y - TAB_HEIGHT);
	window->tab_offset += tab_rect.width;

	Gui_ID tab_gui_id = GET_TAB_GUI_ID();
	update_active_and_hot_state(window, tab_gui_id, &tab_rect);

	if (tab_gui_id == active_item) {
		active_tab = active_item;
	}

	if (must_rect_be_drawn(&window->rect, &tab_rect)) {
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		
		if (!window->tab_was_drawn) {
			window->tab_was_drawn = true;
			render_list->add_rect(&tab_bar_rect, tab_theme.tab_bar_color);
		}
		place_in_middle(&tab_rect, &name_rect, BOTH_AXIS);

		Rect_s32 clip_rect = calculate_clip_rect(&window->rect, &tab_rect);
		render_list->push_clip_rect(&clip_rect);

		if (active_tab != tab_gui_id) {
			render_list->add_rect(&tab_rect, tab_theme.tab_color);
		} else {
			render_list->add_rect(&tab_rect, tab_theme.active_tab_color, ROUND_TOP_RECT);
		}
		render_list->add_text(&name_rect, tab_name);
		render_list->pop_clip_rect();
	}
	tab_count++;
	return (active_tab == tab_gui_id);
}

void Gui_Manager::image(Texture2D *texture, s32 width, s32 height)
{
	assert(texture);

	Rect_s32 image_rect = { 0, 0, (s32)texture->width, (s32)texture->height };
	if (width > 0) {
		image_rect.width = width;
	}
	if (height > 0) {
		image_rect.height = height;
	}

	Gui_Window *window = get_window();
	place_rect_in_window(window, &image_rect);

	if (must_rect_be_drawn(&window->rect, &image_rect)) {
		Rect_s32 clip_rect = calculate_clip_rect(&window->view_rect, &image_rect);
		
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&clip_rect);
		render_list->add_texture(image_rect.x, image_rect.y, image_rect.width, image_rect.height, texture);
		render_list->pop_clip_rect();
	}
}

void Gui_Manager::list_box(Array<String> *array, u32 *item_index)
{
	assert(array->count > 0);

	if (*item_index >= array->count) {
		*item_index = 0;
	}

	Rect_s32 list_box_rect = { 0, 0, 220, 20 };
	
	Gui_Window *window = get_window();
	place_rect_in_window(window, &list_box_rect);

	if (must_rect_be_drawn(&window->rect, &list_box_rect)) {

		Gui_ID list_box_gui_id = GET_LIST_BOX_GUI_ID();
		update_active_and_hot_state(window, list_box_gui_id, &list_box_rect);

		Rect_s32 drop_window_rect;
		s32 window_box_height = array->count * button_theme.rect.height;
		s32 window_box_default_position = list_box_rect.bottom() + 5;
		
		if ((window_box_default_position + window_box_height) > window->rect.bottom()) {
			drop_window_rect.set(list_box_rect.x, list_box_rect.y - 5 - window_box_height);
		} else {
			drop_window_rect.set(list_box_rect.x, window_box_default_position);
		}
		drop_window_rect.set_size(list_box_rect.width, window_box_height);

		if (was_click_by_left_mouse_button()) {
			if (hot_item == list_box_gui_id) {
				if (active_list_box != active_item) {
					active_list_box = active_item;
				} else {
					active_list_box = 0;
				}
			} else {
				if ((active_list_box == list_box_gui_id) && !detect_collision(&drop_window_rect)) {
					active_list_box = 0;
				}
			}
		}

		u32 alignment = LEFT_ALIGNMENT;
		if (active_list_box == list_box_gui_id) {
			Gui_Window_Theme win_theme;
			win_theme.shift_element_from_window_side = 0;
			win_theme.outlines_width = 1.0f;
			win_theme.place_between_elements = 0;
			set_next_window_theme(&win_theme);

			set_next_window_pos(drop_window_rect.x, drop_window_rect.y);
			set_next_window_size(drop_window_rect.width, drop_window_rect.height);

			//@Note: May be create string for list_box_gui_id is a temporary decision.
			// I will refactoring this code when make normal working child windows.
			begin_window(String((int)list_box_gui_id), WINDOW_TYPE_PARENT, WINDOW_WITH_OUTLINES);

			Gui_Text_Button_Theme theme;
			theme.rect.width = list_box_rect.width;
			theme.color = window_theme.background_color;
			theme.aligment |= alignment;

			button_theme = theme;
			for (u32 i = 0; i < array->count; i++) {
				if (button(array->get(i))) {
					*item_index = i;
					active_list_box = 0;
				}
			}
			button_theme = DEFAULT_BUTTON_THEME;
			end_window();
		}

		window = get_window();

		Rect_s32 clip_rect = calculate_clip_rect(&window->view_rect, &list_box_rect);
		Rect_s32 name_rect = get_text_rect(array->get(*item_index));
		
		if (alignment & RIGHT_ALIGNMENT) {
			place_in_middle_and_by_right(&list_box_rect, &name_rect, button_theme.shift_from_size);
		} else if (alignment & LEFT_ALIGNMENT) {
			place_in_middle_and_by_left(&list_box_rect, &name_rect, button_theme.shift_from_size);
		} else {
			place_in_middle(&list_box_rect, &name_rect, BOTH_AXIS);
		}
		
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&clip_rect);
		render_list->add_rect(&list_box_rect, Color(66, 70, 75), button_theme.rounded_border);
		render_list->add_text(&name_rect, array->get(*item_index));
		render_list->pop_clip_rect();
	}
	list_box_count++;
}

bool Gui_Manager::button(const char *name, bool *state)
{
	Rect_s32 button_rect = button_theme.rect;	
	
	Gui_Window *window = get_window();
	place_rect_in_window(window, &button_rect);

	bool mouse_hover = false;
	if (must_rect_be_drawn(&window->rect, &button_rect)) {
		
		u32 button_gui_id = GET_BUTTON_GUI_ID();
		update_active_and_hot_state(window, button_gui_id, &button_rect);

		if ((state != NULL) && (hot_item == button_gui_id) && (was_click_by_left_mouse_button())) {
			reverse_state(state);
		}

		mouse_hover = (hot_item == button_gui_id);

		Rect_s32 clip_rect = calculate_clip_rect(&window->view_rect, &button_rect);
		Rect_s32 name_rect = get_text_rect(name);

		if (button_theme.aligment & RIGHT_ALIGNMENT) {
			place_in_middle_and_by_right(&button_rect, &name_rect, button_theme.shift_from_size);
		} else if (button_theme.aligment & LEFT_ALIGNMENT) {
			place_in_middle_and_by_left(&button_rect, &name_rect, button_theme.shift_from_size);
		} else {
			place_in_middle(&button_rect, &name_rect, BOTH_AXIS);
		}
		
		Color button_color = mouse_hover ? button_theme.hover_color : button_theme.color;
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&clip_rect);
		render_list->add_rect(&button_rect, button_color, button_theme.rounded_border);
		render_list->add_text(&name_rect, name);
		render_list->pop_clip_rect();
	}
	button_count++;
	return (was_click_by_left_mouse_button() && mouse_hover);
}

bool Gui_Manager::can_window_be_resized(Gui_Window *window)
{
	bool can_resize = true;
	Gui_Window *collided_window = NULL;
	For(window->collided_windows, collided_window) {
		if ((collided_window->index_in_windows_order > window->index_in_windows_order) && detect_collision(&collided_window->rect)) {
			return false;
		}
	}
	return true;
}

void Gui_Manager::init(Render_2D *_render_2d, Win32_Info *_win32_info, Font *_font)
{
	print("[Init Hades gui]");
	render_2d = _render_2d;
	win32_info = _win32_info;
	font = _font;

	any_window_was_moved = false;
	handle_events_for_one_window = false;
	edit_field_count = 0;
	window_parent_count = 0;
	reset_window_params = 0;
	curr_parent_windows_index_sum = 0;
	prev_parent_windows_index_sum = 0;
	window_theme = DEFAULT_WINDOW_THEME;

	String path_to_save_file;
	build_full_path_to_gui_file("new_gui_data.gui", path_to_save_file);

	File save_file;
	if (!save_file.open(path_to_save_file, FILE_MODE_READ, FILE_OPEN_EXISTING)) {
		print("Gui_Manager::init: Hades gui file was not found.");
		return;
	}

	u32 window_count = 0;
	save_file.read((void *)&window_count, sizeof(u32));

	for (u32 i = 0; i < window_count; i++) {
		u32 window_style = 0;
		save_file.read((void *)&window_style, sizeof(u32));
		
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
		print("Window styles:");
		if (window_style & WINDOW_WITH_OUTLINES) {
			print("    Window with outlines");
		}
		
		if (window_style & WINDOW_WITH_HEADER) {
			print("    Window with header");
		}
		
		if (window_style & WINDOW_WITH_SCROLL_BAR) {
			print("    Window with scroll bar");
		}

		create_window(string, WINDOW_TYPE_PARENT, window_style, &rect);
		free_string(string);
	}
}

void Gui_Manager::shutdown()
{
	String path_to_save_file;
	build_full_path_to_gui_file("new_gui_data.gui", path_to_save_file);

	File save_file;
	if (!save_file.open(path_to_save_file, FILE_MODE_WRITE, FILE_CREATE_ALWAYS)) {
		print("Gui_Manager::shutdown: Hades gui data can not be save in file by path {}.", path_to_save_file);
		return;
	}

	save_file.write((void *)&window_parent_count, sizeof(u32));

	Gui_Window *window = NULL;
	int i = 0;
	For(windows, window) {
		if (window->type == WINDOW_TYPE_PARENT) {

			save_file.write((void *)&window->style, sizeof(u32));
			save_file.write((void *)&window->name.len, sizeof(int));
			save_file.write((void *)&window->name.data[0], window->name.len);
			save_file.write((void *)&window->rect, sizeof(Rect_s32));
		}
	}
}

void Gui_Manager::new_frame()
{
	//@TODO: can I not use frame_count var ?
	static s64 frame_count = 0;

	window_rect = DEFAULT_WINDOW_RECT;

	is_window_order_update = false;
	mouse_x = Mouse_Input::x;
	mouse_y = Mouse_Input::y;
	mouse_x_delta = mouse_x - last_mouse_x;
	mouse_y_delta = mouse_y - last_mouse_y;
	hot_item = 0;
	button_count = 0;
	radio_button_count = 0;
	tab_count = 0;
	list_box_count = 0;
	edit_field_count = 0;
	became_just_focused = 0;

	curr_parent_windows_index_sum = 0;

	frame_count++;
	// Using frame_count in order to update collided windows in not moved windows.
	if (any_window_was_moved || (frame_count == 2)) {
		Gui_Window *checking_window = NULL;
		for (u32 i = 0; i < windows_order.count; i++) {
			checking_window = get_window_by_index(&windows_order, i);
			checking_window->collided_windows.count = 0;

			for (u32 j = 0; j < windows_order.count; j++) {
				Gui_Window *checked_window = get_window_by_index(&windows_order, j);
				if ((checking_window->gui_id != checked_window->gui_id) && detect_collision(&checking_window->rect, &checked_window->rect)) {
					checking_window->collided_windows.push(checked_window);
				}
			}
		}
		any_window_was_moved = false;
	}

	u32 max_windows_order_index = 0;
	u32 mouse_interception_count = 0;
	Gui_ID first_drawing_window_id;
	for (u32 i = 0; i < windows_order.count; i++) {
		Gui_Window *window = get_window_by_index(&windows_order, i);
		if (detect_collision(&window->rect)) {
			if (window->index_in_windows_order > (s32)max_windows_order_index) {
				max_windows_order_index = window->index_in_windows_order;
				first_drawing_window_id = window->gui_id;
			}
			mouse_interception_count++;
		}
	}

	if (mouse_interception_count > 1) {
		window_events_handler_id = first_drawing_window_id;
		handle_events_for_one_window = true;
	}
	else {
		if (handle_events_for_one_window) {
			window_events_handler_id = 0;
			handle_events_for_one_window = false;
		}
	}
}

void Gui_Manager::end_frame()
{
	if (!is_left_mouse_button_down()) {
		active_item = 0;
		resizing_window = 0;
	}
	last_mouse_x = Mouse_Input::x;
	last_mouse_y = Mouse_Input::y;
#if PRINT_GUI_INFO
	if (curr_parent_windows_index_sum != prev_parent_windows_index_sum) {
		print("Gui_Manager::end_frame: curr_parent_windows_index_sum = ", curr_parent_windows_index_sum);
		print("Gui_Manager::end_frame: prev_parent_windows_index_sum = ", prev_parent_windows_index_sum);
	}
#endif
	if ((prev_parent_windows_index_sum - curr_parent_windows_index_sum) > 0) {
		s32 window_index = prev_parent_windows_index_sum - curr_parent_windows_index_sum - 1;
		Gui_Window *window = get_window_by_index(&windows_order, window_index);
		window->index_in_windows_order = -1;
#if PRINT_GUI_INFO
		print("Gui_Manager::end_frame: Remove window with name = {}; window index = {}", window->name, window_index);
#endif
		windows_order.remove(window_index);
		for (u32 i = 0; i < windows_order.count; i++) {
			Gui_Window *window = get_window_by_index(&windows_order, i);
			window->set_index_with_offset(i);
		}
	}

	prev_parent_windows_index_sum = curr_parent_windows_index_sum;

	for (u32 i = 0; i < windows_order.count; i++) {
		Gui_Window *window = get_window_by_index(&windows_order, i);
		render_2d->add_render_primitive_list(&window->render_list);
		for (u32 i = 0; i < window->child_windows.count; i++) {
			render_2d->add_render_primitive_list(&window->child_windows[i]->render_list);
		}
	}
}

void Gui_Manager::begin_window(const char *name, Window_Type window_type, Window_Style window_style)
{	
	u32 window_index = 0;
	Gui_Window *window = find_window(name, &window_index);
	
	if (!window) {
		window = create_window(name, window_type, window_style);
		window_index = windows.count - 1;
		became_just_focused = window->gui_id;
		
		if (window->type != WINDOW_TYPE_CHILD) {
			focused_window = window->gui_id;
		}
		
		if (window_type == WINDOW_TYPE_CHILD) {
			Gui_Window *parent_window = get_window();
			parent_window->child_windows.push(window);
		}
	}
	window_stack.push(window_index);
	
	window->start_new_frame(window_style);

	Rect_s32  *rect = &window->rect;
	update_active_and_hot_state(window, window->gui_id, &window->rect);

	if ((hot_item == window->gui_id) && was_left_mouse_button_just_pressed() && (focused_window != window->gui_id)) {
		became_just_focused = window->gui_id;
		if (window->type != WINDOW_TYPE_CHILD) {
			focused_window = window->gui_id;
		}
	}

	if ((active_item == window->gui_id) && is_left_mouse_button_down() && !was_left_mouse_button_just_pressed()) {
		s32 min_window_position = 0;
		if (window->style & WINDOW_WITH_OUTLINES) {
			min_window_position = (s32)window_theme.outlines_width;
		}
		s32 x = math::clamp(rect->x + mouse_x_delta, min_window_position, (s32)win32_info->window_width - rect->width);
		s32 y = math::clamp(rect->y + mouse_y_delta, min_window_position, (s32)win32_info->window_height - rect->height);
		window->set_position(x, y);
		any_window_was_moved = true;
	}

	static Rect_Side rect_side;
	if (detect_collision_window_borders(rect, &rect_side) && can_window_be_resized(window)) {
		probably_resizing_window = window->gui_id;

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
	} else {
		if (probably_resizing_window == window->gui_id) {
			probably_resizing_window = 0;
			set_cursor(CURSOR_TYPE_ARROW);
		}
	}

	if ((resizing_window == window->gui_id) && is_left_mouse_button_down()) {

		if ((rect_side == RECT_SIDE_LEFT) || (rect_side == RECT_SIDE_LEFT_BOTTOM)) {
			if ((mouse_x >= 0) && ((rect->width - mouse_x_delta) > MIN_WINDOW_WIDTH)) {
				s32 x = math::max(rect->x + mouse_x_delta, 0);
				window->set_position(x, rect->y);
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
				s32 y = math::max(rect->y + mouse_y_delta, 0);
				window->set_position(rect->x, y);
				rect->height = math::max(rect->height - mouse_y_delta, MIN_WINDOW_HEIGHT);
			}
		}
		window->view_rect.set_size(rect->width, rect->height);
	}

	if (reset_window_params & SET_WINDOW_POSITION) {
		window->set_position(window_rect.x, window_rect.y);
		reset_window_params &= ~SET_WINDOW_POSITION;
	}

	if (reset_window_params & SET_WINDOW_SIZE) {
		rect->set_size(window_rect.width, window_rect.height);
		reset_window_params &= ~SET_WINDOW_SIZE;
	}

	if (reset_window_params & SET_WINDOW_THEME) {
		backup_window_theme = window_theme;
		window_theme = future_window_theme;
	}
	
	Render_Primitive_List *render_list = GET_RENDER_LIST();
	render_list->add_rect(&window->rect, window_theme.background_color, window_theme.rounded_border);
	
	if (window->style & WINDOW_WITH_OUTLINES) {
		render_list->add_outlines(rect->x, rect->y, rect->width, rect->height, window_theme.outlines_color, window_theme.outlines_width, window_theme.rounded_border);
	}
	if (window->style & WINDOW_WITH_HEADER) {
		Rect_s32 header_rect = { rect->x, rect->y, rect->width, window_theme.header_height };
		window->place_rect_over_window(&header_rect);
		render_list->add_rect(&header_rect, window_theme.header_color, window_theme.rounded_border, ROUND_TOP_RECT);

		Rect_s32 name_rect = get_text_rect(window->name);
		place_in_middle(&header_rect, &name_rect, BOTH_AXIS);
		render_list->add_text(&name_rect, window->name);
	}
}

void Gui_Manager::end_window()
{
	u32 window_index = window_stack.top();
	Gui_Window *window = get_window();;
	if (window->index_in_windows_order < 0) {
		window->set_index_with_offset(windows_order.count);
		windows_order.push(window_index);
	
	} else if ((became_just_focused == window->gui_id) && (window->index_in_windows_order != -1)) {
		windows_order.remove(window->get_index_with_offset());
		windows_order.push(window_index);
		curr_parent_windows_index_sum = 0;
		for (u32 i = 0; i < windows_order.count; i++) {
			Gui_Window *local_window = get_window_by_index(&windows_order, i);
			local_window->set_index_with_offset(i);
			curr_parent_windows_index_sum += local_window->index_in_windows_order;
		}
		is_window_order_update = true;
	}

	if (!is_window_order_update) {
		curr_parent_windows_index_sum += window->index_in_windows_order;
	}
	
	window->content_rect.height += window_theme.place_between_elements;
	window->content_rect.width += window_theme.place_between_elements;

	bool draw_right_scroll_bar = false;
	bool draw_bottom_scroll_bar = false;
	Rect_s32 right_scroll_bar;
	Rect_s32 bottom_scroll_bar;
	
	if ((window->style & WINDOW_WITH_SCROLL_BAR) && ((window->content_rect.height > window->view_rect.height) || (window->scroll[Y_AXIS] > window->view_rect.y))) {
		right_scroll_bar = window->get_scrollbar_rect(Y_AXIS);
		window->place_rect_over_window(&right_scroll_bar);
		draw_right_scroll_bar = true;
	}

	if ((window->style & WINDOW_WITH_SCROLL_BAR) && ((window->content_rect.width > window->view_rect.width) || (window->scroll[X_AXIS] > window->view_rect.x))) {
		bottom_scroll_bar = window->get_scrollbar_rect(X_AXIS);
		window->place_rect_over_window(&bottom_scroll_bar);
		draw_bottom_scroll_bar = true;
	}

	if (draw_right_scroll_bar) {
		scroll_bar(window, Y_AXIS, &right_scroll_bar);
	}
	if (draw_bottom_scroll_bar) {
		scroll_bar(window, X_AXIS, &bottom_scroll_bar);
	}

#if DRAW_WINDOW_DEBUG_RECTS
	draw_debug_rect(window, &window->view_rect);
	draw_debug_rect(window, &window->content_rect, Color::Green);
#endif
	
	//Reset a window state
	window->content_rect.set_size(0, 0);
	
	window_rect = DEFAULT_WINDOW_RECT;
	
	if (reset_window_params & SET_WINDOW_THEME) {
		reset_window_params &= ~SET_WINDOW_THEME;
		window_theme = DEFAULT_WINDOW_THEME;
	}
	window_stack.pop();
}

void Gui_Manager::begin_child(const char *name, Window_Style window_style)
{
	assert(!window_stack.is_empty());

	u32 window_index = 0;
	Gui_Window *parent_window = get_window();
	Gui_Window *child_window = find_window(name, &window_index);
	
	if (!child_window) {
		Rect_s32 rect = { 10, 10, 100, 100 };
		child_window = create_window(name, WINDOW_TYPE_CHILD, window_style, &rect);
		window_index = child_window->index_in_windows_array;
		child_window->type = WINDOW_TYPE_CHILD;
		//Windows array could be resized.
		parent_window = get_window();
		parent_window->child_windows.push(child_window);
	}
	Rect_s32 window_position = { 0, 0, child_window->rect.width, child_window->rect.height };
	place_rect_in_window(parent_window, &window_position);
	child_window->set_position(window_position.x, window_position.y);
	child_window->start_new_frame(window_style);

	Rect_s32 *rect = &child_window->rect;
	Rect_s32 *view_rect = &child_window->view_rect;
	if (reset_window_params & SET_WINDOW_POSITION) {
		child_window->set_position(window_rect.x, window_rect.y);
		reset_window_params &= ~SET_WINDOW_POSITION;
	}

	if (reset_window_params & SET_WINDOW_SIZE) {
		rect->set_size(window_rect.width, window_rect.height);
		*view_rect = *rect;
		reset_window_params &= ~SET_WINDOW_SIZE;
	}

	if (reset_window_params & SET_WINDOW_THEME) {
		backup_window_theme = window_theme;
		window_theme = future_window_theme;
	}
	
	// If an parent view window less than an child view window gui must clip the child view window
	// so that clipping works for rects inside the child window.
	child_window->view_rect = calculate_clip_rect(&parent_window->view_rect, &child_window->view_rect);
	
	Render_Primitive_List *render_list = &child_window->render_list;
	Rect_s32 clip_rect = calculate_clip_rect(&parent_window->rect, &child_window->rect);
	if (child_window->style & WINDOW_WITH_OUTLINES) {
		clip_rect.x -= (s32)window_theme.outlines_width;
		clip_rect.y -= (s32)window_theme.outlines_width;
		clip_rect.width += (s32)window_theme.outlines_width * 2;
		clip_rect.height += (s32)window_theme.outlines_width * 2;
	}
	render_list->push_clip_rect(&clip_rect);
	render_list->add_rect(&child_window->rect, window_theme.background_color, window_theme.rounded_border);
	
	if (child_window->style & WINDOW_WITH_OUTLINES) {
		render_list->add_outlines(rect->x, rect->y, rect->width, rect->height, window_theme.outlines_color, window_theme.outlines_width, window_theme.rounded_border);
	}

	if (child_window->style & WINDOW_WITH_HEADER) {
		Rect_s32 header_rect = { view_rect->x, view_rect->y, view_rect->width, window_theme.header_height };
		child_window->place_rect_over_window(&header_rect);
		render_list->add_rect(&header_rect, window_theme.header_color, window_theme.rounded_border, ROUND_TOP_RECT);
		
		Rect_s32 name_rect = get_text_rect(child_window->name);
		place_in_middle(&header_rect, &name_rect, BOTH_AXIS);
		render_list->add_text(&name_rect, child_window->name);
	}
	render_list->pop_clip_rect();
	window_stack.push(window_index);
}

void Gui_Manager::end_child()
{
	Gui_Window *window = get_window();
	window->content_rect.height += window_theme.place_between_elements;
	window->content_rect.width += window_theme.place_between_elements;

	bool draw_right_scroll_bar = false;
	bool draw_bottom_scroll_bar = false;
	Rect_s32 right_scroll_bar;
	Rect_s32 bottom_scroll_bar;

	if (((window->style & WINDOW_WITH_SCROLL_BAR) && ((window->content_rect.height > window->view_rect.height)) || (window->scroll[Y_AXIS] > window->view_rect.y))) {
		print("Content rect", &window->content_rect);
		print("View rect", &window->view_rect);
		right_scroll_bar = window->get_scrollbar_rect(Y_AXIS);
		draw_right_scroll_bar = true;
	}

	if (((window->style & WINDOW_WITH_SCROLL_BAR) && ((window->content_rect.width > window->view_rect.width)) || (window->scroll[X_AXIS] > window->view_rect.x))) {
		bottom_scroll_bar = window->get_scrollbar_rect(X_AXIS);
		draw_bottom_scroll_bar = true;
	}

	Render_Primitive_List *render_list = GET_RENDER_LIST();
	render_list->push_clip_rect(&window->view_rect);
	
	if (draw_right_scroll_bar) {
		scroll_bar(window, Y_AXIS, &right_scroll_bar);
		window->place_rect_over_window(&right_scroll_bar);
	}
	if (draw_bottom_scroll_bar) {
		scroll_bar(window, X_AXIS, &bottom_scroll_bar);
		window->place_rect_over_window(&bottom_scroll_bar);
	}
	
	render_list->pop_clip_rect();

	if (reset_window_params & SET_WINDOW_THEME) {
		window_theme = backup_window_theme;
		reset_window_params &= ~SET_WINDOW_THEME;
	}

#if DRAW_CHILD_WINDOW_DEBUG_RECTS
	draw_debug_rect(window, &window->view_rect);
	draw_debug_rect(window, &window->content_rect, Color::Green);
#endif

	window->content_rect.set_size(0, 0);
	window_stack.pop();
}

void Gui_Manager::same_line()
{
	Gui_Window *window = get_window();
	if (window->alignment & VERTICALLY_ALIGNMENT) {
		window->alignment &= ~VERTICALLY_ALIGNMENT;
		window->alignment &= ~VERTICALLY_ALIGNMENT_JUST_SET;
	}
	if (window->alignment & HORIZONTALLY_ALIGNMENT) {
		window->alignment |= HORIZONTALLY_ALIGNMENT_ALREADY_SET;
	}
	window->alignment |= HORIZONTALLY_ALIGNMENT;
	window->alignment |= HORIZONTALLY_ALIGNMENT_JUST_SET;
}

void Gui_Manager::next_line()
{
	Gui_Window *window = get_window();
	if (window->alignment & HORIZONTALLY_ALIGNMENT) {
		window->alignment &= ~HORIZONTALLY_ALIGNMENT;
		window->alignment &= ~HORIZONTALLY_ALIGNMENT_JUST_SET;
		window->alignment &= ~VERTICALLY_ALIGNMENT_WAS_USED;
	}
	window->alignment |= VERTICALLY_ALIGNMENT;
	window->alignment |= VERTICALLY_ALIGNMENT_JUST_SET;
}

void Gui_Manager::scroll_bar(Gui_Window *window, Axis axis, Rect_s32 *scroll_bar)
{
	s32 window_size = window->view_rect.get_size()[axis];
	s32 content_size = window->content_rect.get_size()[axis];

	float ratio = (float)(window_size) / (float)content_size;
	s32 scroll_size = (ratio < 1.0f) ? (s32)((float)window_size * ratio) : window_size;

	s32 scroll_bar_width = 8;
	s32 scroll_offset = math::abs(window->scroll[axis] - window->view_rect[axis]);
	scroll_size = math::clamp(scroll_size, 10, math::abs(window->view_rect.get_size()[axis] - scroll_offset));

	Rect_s32 scroll_rect;
	if (axis == Y_AXIS) {
		scroll_rect = Rect_s32(scroll_bar->x, window->scroll[axis], scroll_bar_width, scroll_size);
	} else {
		scroll_rect = Rect_s32(window->scroll[axis], scroll_bar->y, scroll_size, scroll_bar_width);
	}
	
	Gui_ID scroll_bar_gui_id = GET_SCROLL_BAR_GUI_ID();
	update_active_and_hot_state(window, scroll_bar_gui_id, &scroll_rect);

	s32 mouse_delta = 0;
	bool update_scroll_position = false;
	if ((active_item == scroll_bar_gui_id) && is_left_mouse_button_down()) {
		mouse_delta = (axis == Y_AXIS) ? mouse_y_delta : mouse_x_delta;
		update_scroll_position = true;
		//@TODO: This is hard code ?
		// close list box window.
		active_list_box = 0;
	
	} else if ((focused_window == window->gui_id) && (axis == Y_AXIS)) {
		Queue<Event> *events = get_event_queue();
		for (Queue_Node<Event> *node = events->first; node != NULL; node = node->next) {
			Event *event = &node->item;
			if (event->type == EVENT_TYPE_MOUSE_WHEEL) {
				mouse_delta = (event->first_value > 0) ? -window_theme.mouse_wheel_spped : window_theme.mouse_wheel_spped;
				//@TODO: This is hard code ?
				// close list box window.
				active_list_box = 0;
				update_scroll_position = true;
			}
		}
	}

	if (update_scroll_position) {
		s32 window_side = (axis == Y_AXIS) ? window->view_rect.bottom() : window->view_rect.right();

		scroll_rect[axis] = math::clamp(scroll_rect[axis] + mouse_delta, window->view_rect[axis], window_side - scroll_rect.get_size()[axis]);

		s32 scroll_pos = scroll_rect[axis] - window->view_rect[axis];
		float scroll_ration = (float)scroll_pos / window->view_rect.get_size()[axis];
		s32 result = (s32)(window->content_rect.get_size()[axis] * scroll_ration);
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


void gui::init_gui(Render_2D *render_2d, Win32_Info *win32_info, Font *font)
{
	gui_manager.init(render_2d, win32_info, font);
}

void gui::shutdown()
{
	gui_manager.shutdown();
}

bool gui::add_tab(const char *tab_name)
{
	return gui_manager.add_tab(tab_name);
}

void gui::image(Texture2D *texture, s32 width, s32 height)
{
	gui_manager.image(texture, width, height);
}

void gui::list_box(Array<String> *array, u32 *item_index)
{
	gui_manager.list_box(array, item_index);
}

bool gui::radio_button(const char *name, bool *state)
{
	return gui_manager.radio_button(name, state);
}

void gui::edit_field(const char *name, int *value)
{
	gui_manager.edit_field(name, value);
}

void gui::edit_field(const char *name, float *value)
{
	gui_manager.edit_field(name, value);
}

void gui::edit_field(const char *name, String *value)
{
	gui_manager.edit_field(name, value);
}

bool gui::edit_field(const char *name, Vector3 *vector, const char *x, const char *y, const char *z)
{
	return gui_manager.edit_field(name, vector, x, y, z);
}

Size_s32 gui::get_window_size()
{
	Gui_Window *window = gui_manager.get_window();
	return window->rect.get_size();
}

void gui::begin_frame()
{
	gui_manager.new_frame();
}

void gui::end_frame()
{
	gui_manager.end_frame();
}

bool gui::begin_window(const char *name, Window_Style window_style)
{
	gui_manager.begin_window(name, WINDOW_TYPE_PARENT, window_style);
	return true;
}
void gui::end_window()
{
	gui_manager.end_window();
}

bool gui::begin_child(const char *name, Window_Style window_style)
{
	gui_manager.begin_child(name, window_style);
	return true;
}

void gui::end_child()
{
	gui_manager.end_child();
}

void gui::set_next_theme(Gui_Window_Theme *gui_window_theme)
{
	gui_manager.set_next_window_theme(gui_window_theme);
}

void gui::set_theme(Gui_Window_Theme *gui_window_theme)
{
	gui_manager.window_theme = *gui_window_theme;
}

void gui::set_theme(Gui_Text_Button_Theme *gui_button_theme)
{
	gui_manager.button_theme = *gui_button_theme;
}

void gui::reset_window_theme()
{
	gui_manager.window_theme = DEFAULT_WINDOW_THEME;
}

void gui::reset_button_theme()
{
	gui_manager.button_theme = DEFAULT_BUTTON_THEME;
}

void gui::set_next_window_size(s32 width, s32 height)
{
	gui_manager.set_next_window_size(width, height);
}

void gui::set_next_window_pos(s32 x, s32 y)
{
	gui_manager.set_next_window_pos(x, y);
}

void gui::same_line()
{
	gui_manager.same_line();
}

void gui::next_line()
{
	gui_manager.next_line();
}

bool gui::button(const char *text, bool *state)
{
	return gui_manager.button(text, state);
}

void gui::text(const char *some_text)
{
	gui_manager.text(some_text);
}

bool gui::were_events_handled()
{
	for (u32 i = 0; i < gui_manager.windows_order.count; i++) {
		Gui_Window * window = gui_manager.get_window_by_index(&gui_manager.windows_order, i);
		if (detect_collision(&window->rect)) {
			return true;
		}
	}
	return false;
}

void gui::draw_test_gui()
{
	begin_frame();

	if (begin_window("Line window")) {
		static float x = 0.0f;
		edit_field("Label", &x);
	}
	end_window();
	
	end_frame();
}
