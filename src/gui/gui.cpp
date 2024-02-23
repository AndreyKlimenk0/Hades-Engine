#include <assert.h>
#include <stdlib.h>

#include "gui.h"

#include "../render/font.h"

#include "../sys/engine.h"
#include "../sys/sys_local.h"

#include "../win32/win_time.h"
#include "../win32/win_local.h"

#include "../libs/str.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/os/input.h"
#include "../libs/os/event.h"
#include "../libs/ds/stack.h"
#include "../libs/png_image.h"
#include "../libs/key_binding.h"
#include "../libs/ds/hash_table.h"
#include "../libs/ds/linked_list.h"

#define ASSERT_MSG(expr, str_msg) (assert((expr) && (str_msg)))

#ifdef _DEBUG
	static s32 list_line_debug_counter = 0;
	static s32 list_column_debug_counter = 0;
#endif

#define PRINT_GUI_INFO 0
#define DRAW_WINDOW_DEBUG_RECTS 0
#define DRAW_CHILD_WINDOW_DEBUG_RECTS 0

const Element_Alignment HORIZONTALLY_ALIGNMENT = 0x04;
const Element_Alignment VERTICALLY_ALIGNMENT = 0x08;
const Element_Alignment GO_TO_NEW_LINE = 0x10;
const Element_Alignment HORIZONTALLY_ALIGNMENT_JUST_SET = 0x20;
const Element_Alignment HORIZONTALLY_ALIGNMENT_ALREADY_SET = 0x40;
const Element_Alignment VERTICALLY_ALIGNMENT_JUST_SET = 0x80;
const Element_Alignment VERTICALLY_ALIGNMENT_WAS_USED = 0x100;
const Element_Alignment PLACE_FIRST_RECT = 0x200;

const Gui_List_Line_State GUI_LIST_LINE_RESET_STATE = 0x0;
const Gui_List_Line_State GUI_LIST_LINE_SELECTED = 0x1;
const Gui_List_Line_State GUI_LIST_LINE_CLICKED_BY_LEFT_MOUSE = 0x2;
const Gui_List_Line_State GUI_LIST_LINE_CLICKED_BY_RIGHT_MOUSE = 0x4;

const u32 LIST_HASH = fast_hash("list");
const u32 BUTTON_HASH = fast_hash("button");
const u32 IMAGE_BUTTON_HASH = fast_hash("image_button");
const u32 RADIO_BUTTON_HASH = fast_hash("radio_button");
const u32 LIST_BOX_HASH = fast_hash("list_box");
const u32 EDIT_FIELD_HASH = fast_hash("edit_Field");
const u32 SCROLL_BAR_HASH = fast_hash("scroll_bar");
const u32 TAB_HASH = fast_hash("window_tab_hash");

const u32 VECTOR3_EDIT_FIELD_NUMBER = 3;

#define GET_LIST_GUI_ID() (window->gui_id + LIST_HASH + list_count)
#define GET_BUTTON_GUI_ID() (window->gui_id + BUTTON_HASH + button_count)
#define GET_LIST_BOX_GUI_ID() (window->gui_id + LIST_BOX_HASH + list_box_count)
#define GET_EDIT_FIELD_GUI_ID() (window->gui_id + EDIT_FIELD_HASH + edit_field_count)
#define GET_RADIO_BUTTON_GUI_ID() (window->gui_id + RADIO_BUTTON_HASH + radio_button_count)
#define GET_IMAGE_BUTTON_GUI_ID() (window->gui_id + IMAGE_BUTTON_HASH + image_button_count)
#define GET_SCROLL_BAR_GUI_ID() (window->gui_id + SCROLL_BAR_HASH)
#define GET_TAB_GUI_ID() (window->gui_id + TAB_HASH + tab_count)

#define GET_RENDER_LIST() (&window->render_list)

const s32 MIN_WINDOW_WIDTH = 20;
const s32 MIN_WINDOW_HEIGHT = 60;
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

inline bool detect_intersection(Rect_s32 *rect)
{
	if ((Mouse_State::x > rect->x) && (Mouse_State::x < (rect->x + rect->width)) && (Mouse_State::y > rect->y) && (Mouse_State::y < (rect->y + rect->height))) {
		return true;
	}
	return false;
}

inline bool detect_intersection(Rect_s32 *first_rect, Rect_s32 *second_rect)
{
	if ((first_rect->x < second_rect->right()) && (first_rect->right() > second_rect->x) && (first_rect->y < second_rect->bottom()) && (first_rect->bottom() > second_rect->y)) {
		return true;
	}
	return false;
}

template <typename T>
inline bool detect_intersection(Triangle<T> *triangle, Pointv2<T> *point)
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

static Rect_s32 calculate_clip_rect(Rect_s32 *win_rect, Rect_s32 *item_rect)
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
	bool open = true;
	bool tab_was_added = false;
	bool tab_was_drawn = false;
	bool display_vertical_scrollbar = false;
	bool display_horizontal_scrollbar = false;
	Gui_ID gui_id = 0;
	s32 tab_offset = 0;
	u32 edit_field_count = 0;
	u32 max_edit_field_number = 0;
	s32 index_in_windows_array = -1;
	s32 index_in_windows_order = -1;
	s32 max_content_width = 0;

	s32 place_between_rects = 0;
	s32 horizontal_offset_from_sides = 0;
	s32 vertical_offset_from_sides = 0;
	
	Gui_Window *parent_window = NULL;

	Window_Type type;
	Element_Alignment alignment;
	Window_Style style;

	String name;
	Rect_s32 rect;
	Rect_s32 view_rect;
	Rect_s32 clip_rect;
	Rect_s32 content_rect;
	Point_s32 scroll;
	Rect_s32 last_placed_rect;
	
	Array<Gui_Window *> child_windows;
	Array<Gui_Window *> collided_windows;
	Render_Primitive_List render_list;

	void new_frame(Window_Style window_style);
	void set_position(s32 x, s32 y);
	void set_index_with_offset(s32 index);
	u32 get_index_with_offset();
	Rect_s32 get_scrollbar_rect(Axis axis);

	void place_rect_over_window(Rect_s32 *new_rect);
};

inline void draw_debug_rect(Gui_Window *window, Rect_s32 *rect, float alpha_value = 0.2f, Color color = Color::Red)
{
	Render_Primitive_List *list = &window->render_list;
	color.value.w = alpha_value;
	list->add_rect(rect, color);
}

void Gui_Window::place_rect_over_window(Rect_s32 *overlap_rect)
{
	if ((overlap_rect->y == view_rect.y) && (overlap_rect->width == view_rect.width)) {
		view_rect.offset_y(overlap_rect->height);
	} else if ((overlap_rect->y == view_rect.y) && (overlap_rect->height == view_rect.height)) {
		view_rect.width -= overlap_rect->width;
	} else if ((overlap_rect->bottom() == rect.bottom()) && (overlap_rect->width == view_rect.width)) {
		view_rect.height -= overlap_rect->height;
	}
}

void Gui_Window::new_frame(Window_Style window_style)
{
	tab_was_drawn = false;
	style = window_style;
	
	view_rect = rect;
	clip_rect = rect;
	content_rect.set_size(0, 0);
	last_placed_rect = { content_rect.x, content_rect.y, 0, 0 };
	alignment = VERTICALLY_ALIGNMENT | PLACE_FIRST_RECT;
	
	max_edit_field_number = math::max(max_edit_field_number, edit_field_count);
	edit_field_count = 0;
}

inline void Gui_Window::set_position(s32 x, s32 y)
{
	s32 delta_x = x - rect.x;
	s32 delta_y = y - rect.y;

	rect.move(delta_x, delta_y);
	view_rect.move(delta_x, delta_y);
	clip_rect.move(delta_x, delta_y);
	content_rect.move(delta_x, delta_y);
	
	last_placed_rect.move(delta_x, delta_y);
	scroll.move(delta_x, delta_y);
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

Edit_Field_State make_edit_field_state(Rect_s32 *caret, const char *str_value, u32 max_text_width, bool(*symbol_validation)(char symbol))
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
const u32 PLACE_RECT_ON_TOP = 0x8;

enum Edit_Field_Type {
	EDIT_FIELD_TYPE_COMMON,// int, float, string edit fields
	EDIT_FIELD_TYPE_VECTOR3
};

enum Gui_List_Column_State : u32 {
	GUI_LIST_COLUMN_STATE_DEFAULT = 0,
	GUI_LIST_COLUMN_STATE_SORTING_UP = 1,
	GUI_LIST_COLUMN_STATE_SORTING_DOWN = 2,
};

enum List_Line_Info_Type {
	LIST_LINE_INFO_TYPE_TEXT,
	LIST_LINE_INFO_TYPE_IMAGE,
	LIST_LINE_INFO_TYPE_IMAGE_BUTTON
};

struct Column_Rendering_Data {
	List_Line_Info_Type type;
	union {
		const char *text = NULL;
		Texture2D *texture;
	};
};

struct Gui_Line_Column {
	const char *name = NULL;
	Array<Column_Rendering_Data> rendering_data_list;
};

struct Gui_List_Line {
	Gui_List_Line_State *state = NULL;
	Array<Gui_Line_Column> columns;
};

struct Gui_Manager {
	bool active_scrolling;
	bool window_handing_events;
	bool change_active_field;
	bool is_window_order_update;
	bool any_window_was_moved;
	bool handle_events_for_one_window;

	s32 mouse_x;
	s32 mouse_y;
	s32 last_mouse_x;
	s32 last_mouse_y;
	s32 mouse_x_delta;
	s32 mouse_y_delta;
	s32 placing_rect_height;
	Rect_s32 *placed_rect = NULL;

	u32 tab_count;
	u32 list_count;
	u32 button_count;
	u32 image_button_count;
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
	Gui_ID active_window;
	Gui_ID became_just_actived; //window
	Gui_ID probably_resizing_window;
	Gui_ID window_events_handler_id;
	Gui_ID current_list_gui_id;

	const char *current_list_name = NULL;
	
	Font *font = NULL;
	Render_Font *render_font = NULL;
	Render_2D *render_2d = NULL;
	
	Win32_Info *win32_info = NULL;
	
	Texture2D up_texture;
	Texture2D down_texture;
	Texture2D cross_texture;
	Texture2D check_texture;
	Texture2D default_texture;
	Texture2D expand_down_texture;

	Rect_s32 window_rect;
	Cursor_Type cursor_type;

	Gui_List_Column *picked_column = NULL;
	Gui_Line_Column current_list_column;
	Gui_List_Line current_list_line;
	Array<Gui_List_Line> line_list;
	Array<Rect_s32> list_column_rect_list;
	
	Stack<u32> window_stack;
	Array<u32> windows_order;
	Array<Gui_Window> windows;

	Gui_List_Theme list_theme;
	Gui_Tab_Theme tab_theme;
	Gui_Window_Theme window_theme;
	Gui_List_Box_Theme list_box_theme;
	Gui_Window_Theme backup_window_theme;
	Gui_Window_Theme future_window_theme;
	Gui_Text_Button_Theme button_theme;
	Gui_Edit_Field_Theme edit_field_theme;
	Gui_Edit_Field_Theme backup_edit_field_theme;
	Gui_Radio_Button_Theme radio_button_theme;

	Edit_Field_State edit_field_state;

	Key_Bindings key_bindings;

	void init(Engine *engine, const char *font_name, u32 font_size);
	void init_from_save_file();
	void load_and_init_textures(Gpu_Device *gpu_device);
	void handle_events();
	void handle_events(bool *update_editing_value, bool *update_next_time_editing_value, Rect_s32 *rect, Rect_s32 *editing_value_rect);
	void set_font(const char *font_name, u32 font_size);
	void shutdown();
	
	void new_frame();
	void end_frame();

	void update_active_window(Gui_Window *window);
	void update_active_and_hot_state(Gui_ID gui_id, Rect_s32 *rect);
	void update_active_and_hot_state(Gui_Window *window, u32 rect_gui_id, Rect_s32 *rect);

	bool begin_window(const char *name, Window_Style window_style);
	void end_window();
	void render_window(Gui_Window *window);
	bool begin_child(const char *name, Window_Style window_style);
	void end_child();

	void same_line();
	void next_line();
	void set_next_window_pos(s32 x, s32 y);
	void set_next_window_size(s32 width, s32 height);
	void set_next_window_theme(Gui_Window_Theme *theme);
	
	void place_rect_on_window_top(s32 height, Rect_s32 *placed_rect);
	void place_rect_in_window(Gui_Window *window, Rect_s32 *rect, s32 place_between_rects, s32 horizontal_offset_from_sides, s32 vertical_offset_from_sides);
	void set_caret_position_on_mouse_click(Rect_s32 *rect, Rect_s32 *editing_value_rect);
	void make_tab_active(Gui_ID tab_gui_id);
	
	void scrolling(Gui_Window *window, Axis axis);

	void text(const char *some_text);
	void image(Texture2D *texture, s32 width, s32 height);
	void list_box(Array<String> *array, u32 *item_index);
	void edit_field(const char *name, int *value);
	void edit_field(const char *name, float *value);
	void edit_field(const char *name, String *string);
	bool add_tab(const char *tab_name);
	bool button(const char *name, bool *state = NULL);
	void core_button(const char *name, bool *left_mouse_click, bool *right_mouse_click);
	bool radio_button(const char *name, bool *state);
	bool image_button(Rect_s32 *rect, Texture2D *texture, Rect_s32 *window_view_rect);
	
	bool edit_field(const char *name, Vector3 *vector, const char *x, const char *y, const char *z);
	bool edit_field(const char *name, const char *editing_value, u32 max_chars_number, bool(*symbol_validation)(char symbol));
	bool edit_field(const char *name, const char *editing_value, u32 max_chars_number, bool(*symbol_validation)(char symbol), const Color &color, u32 index);

	bool begin_list(const char *name, Gui_List_Column columns[], u32 column_count);
	void end_list();

	bool begin_line(Gui_List_Line_State *list_line);
	void end_line();

	bool begin_column(const char *column_name);
	void end_column();

	void add_text(const char *text, Alignment alignment);
	void add_image(Texture2D *texture, Alignment alignment);
	void add_image_button(Texture2D *texture, Alignment alignment);

	bool handle_edit_field_shortcut_event(Gui_Window *window, Rect_s32 *edit_field_rect, Gui_ID edit_field_gui_id, Edit_Field_Type edit_field_type, u32 vector3_edit_field_index = 0);
	
	bool update_edit_field(Edit_Field_Instance *edit_field_instance);

	bool can_window_be_resized(Gui_Window *window);
	bool detect_collision_window_borders(Rect_s32 *rect, Rect_Side *rect_side);

	Gui_ID get_last_tab_gui_id();

	Size_s32 get_window_size();
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
	Size_u32 size = font->get_text_size(text);
	return Rect_s32(0, 0, (s32)size.width, (s32)size.height);
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
	reset_window_params |= SET_WINDOW_THEME;
}

void Gui_Manager::place_rect_on_window_top(s32 height, Rect_s32 *_placed_rect)
{
	reset_window_params |= PLACE_RECT_ON_TOP;
	placing_rect_height = height;
	placed_rect = _placed_rect;
}

void Gui_Manager::place_rect_in_window(Gui_Window *window, Rect_s32 *rect, s32 place_between_rects, s32 horizontal_offset_from_sides, s32 vertical_offset_from_sides)
{
	assert(!((window->alignment & VERTICALLY_ALIGNMENT) && (window->alignment & HORIZONTALLY_ALIGNMENT)));

	if (window->alignment & PLACE_FIRST_RECT) {
		window->alignment &= ~PLACE_FIRST_RECT;

		rect->x = window->content_rect.x + horizontal_offset_from_sides;
		rect->y = window->content_rect.y + vertical_offset_from_sides;

		window->last_placed_rect = *rect;
		
		window->content_rect.height += rect->height + vertical_offset_from_sides;
		window->content_rect.width = rect->width + horizontal_offset_from_sides;
	
	} else if ((window->alignment & VERTICALLY_ALIGNMENT) || (window->alignment & HORIZONTALLY_ALIGNMENT_JUST_SET)) {
		if (window->alignment & VERTICALLY_ALIGNMENT) { window->alignment |= VERTICALLY_ALIGNMENT_WAS_USED; }
		
		if (window->alignment & HORIZONTALLY_ALIGNMENT_JUST_SET) { 
			window->alignment &= ~HORIZONTALLY_ALIGNMENT_JUST_SET; 
			window->max_content_width = 0;
			window->max_content_width = rect->width + horizontal_offset_from_sides;
			window->content_rect.width = math::max(window->max_content_width, window->content_rect.width);
		}
		rect->x = window->content_rect.x + horizontal_offset_from_sides;
		rect->y = window->last_placed_rect.bottom() + place_between_rects;

		window->last_placed_rect = *rect;
		window->content_rect.height += rect->height + place_between_rects;
		window->content_rect.width = math::max(rect->width + horizontal_offset_from_sides, window->content_rect.width);

	} else if (window->alignment & HORIZONTALLY_ALIGNMENT) {
		rect->x = window->last_placed_rect.right() + place_between_rects;
		rect->y = window->last_placed_rect.y;

		window->last_placed_rect = *rect;
		window->max_content_width += rect->width + place_between_rects;
		window->content_rect.width = math::max(window->max_content_width, window->content_rect.width);
	}
}

Size_s32 Gui_Manager::get_window_size()
{
	Gui_Window *window = get_window();
	return window->view_rect.get_size();
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
	window.open = true;
	window.name = name;
	window.gui_id = fast_hash(name);
	window.type = window_type;

	s32 offset = window_style & WINDOW_HEADER ? window_theme.header_height : 0;
	window.rect = window_rect;
	window.view_rect = window_rect;
	window.content_rect = { window_rect.x, window_rect.y + offset, 0, 0 };
	window.scroll = { window_rect.x, window_rect.y + offset };

	window.render_list = Render_Primitive_List(render_2d, font, render_font);

	windows.push(window);
	window_rect.x += window.rect.width + 40;

	if (window_type == WINDOW_TYPE_PARENT) {
		window_parent_count++;
	}
	return &windows.get_last();
}

Gui_Window *Gui_Manager::create_window(const char *name, Window_Type window_type, Window_Style window_style, Rect_s32 *rect)
{
	Gui_Window window;
	window.open = true;
	window.name = name;
	window.gui_id = fast_hash(name);
	window.type = window_type;

	s32 offset = window_style & WINDOW_HEADER ? window_theme.header_height : 0;
	window.rect = *rect;
	window.view_rect = *rect;
	window.content_rect = { rect->x, rect->y + offset, 0, 0 };
	window.scroll = { rect->x, rect->y + offset };
	
	window.style = window_style;
	window.alignment = VERTICALLY_ALIGNMENT | VERTICALLY_ALIGNMENT_JUST_SET;

	window.render_list = Render_Primitive_List(render_2d, font, render_font);

	window.index_in_windows_array = windows.count;
	windows.push(window);

	if (window_type == WINDOW_TYPE_PARENT) {
		window_parent_count++;
	}
	return &windows.get_last();
}

inline Gui_Window *Gui_Manager::get_window_by_index(Array<u32> *window_indices, u32 index)
{
	u32 window_index = window_indices->get(index);
	return &windows[window_index];
}

void Gui_Manager::update_active_window(Gui_Window *window)
{
	if ((hot_item == window->gui_id) && was_key_just_pressed(KEY_LMOUSE) && (active_window != window->gui_id)) {
		became_just_actived = window->gui_id;
		active_window = window->gui_id;
	}
}

void Gui_Manager::update_active_and_hot_state(Gui_ID gui_id, Rect_s32 *rect)
{
	if (detect_intersection(rect)) {
		hot_item = gui_id;
		if (was_key_just_pressed(KEY_LMOUSE)) {
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

bool Gui_Manager::radio_button(const char *name, bool *state)
{
	bool was_click_by_mouse_key = false;
	Rect_s32 rect = radio_button_theme.rect;
	Rect_s32 radio_rect = radio_button_theme.radio_rect;
	Rect_s32 check_texture_rect = radio_button_theme.check_texture_rect;
	Rect_s32 name_rect = get_text_rect(name);
	rect.width = name_rect.width + radio_rect.width + radio_button_theme.text_shift;

	Gui_Window *window = get_window();
	place_rect_in_window(window, &rect, window->place_between_rects, window->horizontal_offset_from_sides, window->vertical_offset_from_sides);

	if (must_rect_be_drawn(&window->clip_rect, &rect)) {
		place_in_middle_and_by_left(&rect, &name_rect, radio_rect.width + radio_button_theme.text_shift);
		place_in_middle_and_by_left(&rect, &radio_rect, 0);
		place_in_middle(&radio_rect, &check_texture_rect, BOTH_AXIS);

		Gui_ID radio_button_gui_id = GET_RADIO_BUTTON_GUI_ID();
		update_active_and_hot_state(window, radio_button_gui_id, &radio_rect);

		if ((hot_item == radio_button_gui_id) && was_click(KEY_LMOUSE)) {
			was_click_by_mouse_key = true;
			if (*state) {
				*state = false;
			} else {
				*state = true;
			}
		}

		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&window->clip_rect);
		if (*state) {
			render_list->add_rect(&radio_rect, radio_button_theme.true_background_color, radio_button_theme.rounded_border);
			render_list->add_texture(&check_texture_rect, &check_texture);
		} else {
			render_list->add_rect(&radio_rect, radio_button_theme.default_background_color, radio_button_theme.rounded_border);
		}
		render_list->add_text(&name_rect, name);
		render_list->pop_clip_rect();
	}
	radio_button_count++;
	return was_click_by_mouse_key;
}

bool Gui_Manager::image_button(Rect_s32 *rect, Texture2D *texture, Rect_s32 *window_view_rect)
{
	assert(rect);
	assert(texture);
	assert((rect->width > 0) && (rect->height > 0));

	Gui_Window *window = get_window();
	Rect_s32 image_rect = *rect;

	if ((image_rect.x < 0) && (image_rect.y < 0)) {
		place_rect_in_window(window, &image_rect, window->place_between_rects, window->horizontal_offset_from_sides, window->vertical_offset_from_sides);
	}
	Gui_ID image_button_gui_id = GET_IMAGE_BUTTON_GUI_ID();
	if (must_rect_be_drawn(&window->clip_rect, &image_rect)) {
		update_active_and_hot_state(image_button_gui_id, &image_rect);

		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&window->clip_rect);
		render_list->add_texture(&image_rect, texture);
		render_list->pop_clip_rect();
	}
	image_button_count++;
	return ((hot_item == image_button_gui_id) && was_click(KEY_LMOUSE));
}
static bool is_symbol_int_valid(char symbol)
{
	return (isdigit(symbol) || (symbol == '-'));
}

static bool is_symbol_float_valid(char symbol)
{
	return (isdigit(symbol) || (symbol == '-') || (symbol == '.'));
}

static bool is_symbol_string_valid(char symbol)
{
	return !iscntrl(symbol);
}

void Gui_Manager::edit_field(const char *name, int *value)
{
	//char *str_value = to_string(*value);
	//if (edit_field(name, str_value, 10, &is_symbol_int_valid)) {
	//	*value = atoi(edit_field_state.data.c_str());
	//}
	//free_string(str_value);
}

void Gui_Manager::edit_field(const char *name, float *value)
{
	char *str_value = to_string(*value, GUI_FLOAT_PRECISION);
	bool result = edit_field(name, str_value, 15, &is_symbol_float_valid);
	if (result && !edit_field_state.data.is_empty()) {
		*value = (float)atof(edit_field_state.data.c_str());
	}
	free_string(str_value);
}

void Gui_Manager::edit_field(const char *name, String *string)
{
	assert(name);
	assert(string);

	Gui_Edit_Field_Theme *theme = &edit_field_theme;

	Edit_Field_Instance edit_field_instance;
	if (string->is_empty()) {
		edit_field_instance.editing_value = "";
	} else {
		edit_field_instance.editing_value = string->c_str();
		edit_field_instance.value_rect = get_text_rect(string->c_str());
		edit_field_instance.value_rect.height = get_text_rect("A").height;
	}

	edit_field_instance.name = name;
	edit_field_instance.symbol_validation = is_symbol_string_valid;
	edit_field_instance.max_chars_number = 50;
	edit_field_instance.caret_rect = { 0, 0, 1, 14 };
	edit_field_instance.rect = theme->rect;
	edit_field_instance.edit_field_rect = theme->rect;
	edit_field_instance.name_rect = get_text_rect(name);
	
	if (theme->draw_label) {
		edit_field_instance.rect.width = edit_field_instance.name_rect.width + edit_field_instance.edit_field_rect.width + theme->text_shift;
	}

	Gui_Window *window = get_window();
	place_rect_in_window(window, &edit_field_instance.rect, window->place_between_rects, window->horizontal_offset_from_sides, window->vertical_offset_from_sides);

	bool update_value = false;
	if (must_rect_be_drawn(&window->clip_rect, &edit_field_instance.rect)) {
		place_in_middle_and_by_left(&edit_field_instance.rect, &edit_field_instance.edit_field_rect, 0);
		if (theme->draw_label) {
			place_in_middle_and_by_left(&edit_field_instance.rect, &edit_field_instance.name_rect, edit_field_instance.edit_field_rect.width + theme->text_shift);
		}
		place_in_middle_and_by_left(&edit_field_instance.edit_field_rect, &edit_field_instance.value_rect, theme->text_shift);
		place_in_middle_and_by_left(&edit_field_instance.value_rect, &edit_field_instance.caret_rect, edit_field_instance.value_rect.width);

		update_value = update_edit_field(&edit_field_instance);
		Gui_ID edit_field_gui_id = GET_EDIT_FIELD_GUI_ID();
		if (active_edit_field == edit_field_gui_id) {
			bool result = handle_edit_field_shortcut_event(window, &edit_field_instance.edit_field_rect, edit_field_gui_id, EDIT_FIELD_TYPE_COMMON);
			update_value = update_value ? update_value : result;
		}
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&window->clip_rect);
		render_list->add_rect(&edit_field_instance.edit_field_rect, theme->color, theme->rounded_border);
		
		if (theme->draw_label) {
			render_list->add_text(&edit_field_instance.name_rect, edit_field_instance.name);
		}

		if (active_edit_field == edit_field_gui_id) {
			render_list->add_rect(&edit_field_state.caret, Color::White);
			if (!edit_field_state.data.is_empty()) {
				render_list->add_text(&edit_field_instance.value_rect, edit_field_state.data);
			}
		} else {
			if (edit_field_instance.editing_value) {
				render_list->add_text(&edit_field_instance.value_rect, edit_field_instance.editing_value);
			}
		}
		render_list->pop_clip_rect();
		//draw_debug_rect(window, &edit_field_instance.value_rect);
	}
	edit_field_count++;
	window->edit_field_count++;

	if (update_value) {
		print("Update string");
		*string = edit_field_state.data;
	}
}

bool Gui_Manager::edit_field(const char *name, Vector3 *vector, const char *x, const char *y, const char *z)
{
	bool result = false;
	bool was_data_updated = false;
	u32 max_chars_number = 12;
	char *x_str = to_string(vector->x, GUI_FLOAT_PRECISION);
	char *y_str = to_string(vector->y, GUI_FLOAT_PRECISION);
	char *z_str = to_string(vector->z, GUI_FLOAT_PRECISION);
	Gui_Edit_Field_Theme *theme = &edit_field_theme;
	
	same_line();
	result = edit_field(x, x_str, max_chars_number, &is_symbol_float_valid, theme->x_edit_field_color, 0);
	if (result && !edit_field_state.data.is_empty()) {
		vector->x = (float)atof(edit_field_state.data.c_str());
		was_data_updated = true;
	}
	result = edit_field(y, y_str, max_chars_number, &is_symbol_float_valid, theme->y_edit_field_color, 1);
	if (result && !edit_field_state.data.is_empty()) {
		vector->y = (float)atof(edit_field_state.data.c_str());
		was_data_updated = true;
	}
	result = edit_field(z, z_str, max_chars_number, &is_symbol_float_valid, theme->z_edit_field_color, 2);
	if (result && !edit_field_state.data.is_empty()) {
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
	edit_field_instance.edit_field_rect = theme->rect;
	edit_field_instance.name_rect = get_text_rect(name);
	edit_field_instance.value_rect = get_text_rect(editing_value);
	edit_field_instance.rect.width = edit_field_instance.name_rect.width + edit_field_instance.edit_field_rect.width + theme->text_shift;

	Gui_Window *window = get_window();
	place_rect_in_window(window, &edit_field_instance.rect, window->place_between_rects, window->horizontal_offset_from_sides, window->vertical_offset_from_sides);

	bool update_value = false;
	if (must_rect_be_drawn(&window->clip_rect, &edit_field_instance.rect)) {
		place_in_middle_and_by_left(&edit_field_instance.rect, &edit_field_instance.edit_field_rect, 0);
		place_in_middle_and_by_left(&edit_field_instance.rect, &edit_field_instance.name_rect, edit_field_instance.edit_field_rect.width + theme->text_shift);
		place_in_middle_and_by_left(&edit_field_instance.edit_field_rect, &edit_field_instance.value_rect, theme->text_shift);
		place_in_middle_and_by_left(&edit_field_instance.value_rect, &edit_field_instance.caret_rect, edit_field_instance.value_rect.width);

		update_value = update_edit_field(&edit_field_instance);
		Gui_ID edit_field_gui_id = GET_EDIT_FIELD_GUI_ID();
		if (active_edit_field == edit_field_gui_id) {
			bool result = handle_edit_field_shortcut_event(window, &edit_field_instance.edit_field_rect, edit_field_gui_id, EDIT_FIELD_TYPE_COMMON);
			update_value = update_value ? update_value : result;
		}
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&window->clip_rect);
		render_list->add_rect(&edit_field_instance.edit_field_rect, theme->color, theme->rounded_border);
		render_list->add_text(&edit_field_instance.name_rect, edit_field_instance.name);

		if (active_edit_field == edit_field_gui_id) {
			render_list->add_rect(&edit_field_state.caret, Color::White);
			if (!edit_field_state.data.is_empty()) {
				render_list->add_text(&edit_field_instance.value_rect, edit_field_state.data);
			}
		} else {
			render_list->add_text(&edit_field_instance.value_rect, edit_field_instance.editing_value);
		}
		render_list->pop_clip_rect();
	}
	edit_field_count++;
	window->edit_field_count++;
	return update_value;
}

bool Gui_Manager::edit_field(const char *name, const char *editing_value, u32 max_chars_number, bool(*symbol_validation)(char symbol), const Color &color, u32 field_index)
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
	place_rect_in_window(window, &edit_field_instance.rect, window->place_between_rects, window->horizontal_offset_from_sides, window->vertical_offset_from_sides);

	bool update_value = false;
	if (must_rect_be_drawn(&window->clip_rect, &edit_field_instance.rect)) {
		place_in_middle_and_by_left(&edit_field_instance.rect, &color_rect);
		place_in_middle_and_by_left(&edit_field_instance.rect, &edit_field_instance.edit_field_rect, color_rect.width);
		place_in_middle(&color_rect, &edit_field_instance.name_rect, BOTH_AXIS);
		place_in_middle_and_by_left(&edit_field_instance.edit_field_rect, &edit_field_instance.value_rect, theme->text_shift);
		place_in_middle_and_by_left(&edit_field_instance.value_rect, &edit_field_instance.caret_rect, edit_field_instance.value_rect.width);

		update_value = update_edit_field(&edit_field_instance);
		Gui_ID edit_field_gui_id = GET_EDIT_FIELD_GUI_ID();
		if (active_edit_field == edit_field_gui_id) {
			bool result = handle_edit_field_shortcut_event(window, &edit_field_instance.edit_field_rect, edit_field_gui_id, EDIT_FIELD_TYPE_VECTOR3, field_index);
			update_value = update_value ? update_value : result;
		}
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&window->clip_rect);

		render_list->add_rect(&edit_field_instance.rect, edit_field_theme.color, edit_field_theme.rounded_border);
		render_list->add_rect(&color_rect, color, edit_field_theme.rounded_border);
		render_list->add_text(&edit_field_instance.name_rect, edit_field_instance.name);

		if (active_edit_field == edit_field_gui_id) {
			render_list->add_rect(&edit_field_state.caret, Color::White);
			if (!edit_field_state.data.is_empty()) {
				render_list->add_text(&edit_field_instance.value_rect, edit_field_state.data);
			}
		} else {
			render_list->add_text(&edit_field_instance.value_rect, edit_field_instance.editing_value);
		}

		render_list->pop_clip_rect();
	}
	edit_field_count++;
	window->edit_field_count++;
	return update_value;
}

template <typename T>
static T calculate(const T &value, const T &percents)
{
	assert((0 <= percents) && (percents <= 100));

	float ratio = (float)percents / 100.0f;
	return (T)((float)value * ratio);
}

inline int compare_strings_priority(const char *first_string, const char *second_string)
{
	u32 first_string_len = (u32)strlen(first_string);
	u32 second_string_len = (u32)strlen(second_string);

	if (first_string_len == second_string_len) {
		for (u32 i = 0; i < first_string_len; i++) {
			u8 first_char = (u8)first_string[i];
			u8 second_char = (u8)second_string[i];

			if (first_char < second_char) {
				return 1;
			} else if (first_char > second_char) {
				return -1;
			}
		}
		return 0;
	} else {
		u32 min_string_len = math::min(first_string_len, second_string_len);
		if (min_string_len > 0) {
			u32 last_string_char_index = min_string_len - 1;
			for (u32 i = 0; i < last_string_char_index; i++) {
				u8 first_char = (u8)first_string[i];
				u8 second_char = (u8)second_string[i];
				if (first_char < second_char) {
					return 1;
				} else if (first_char > second_char) {
					return -1;
				}
			}
			u8 first_char = (u8)first_string[last_string_char_index];
			u8 second_char = (u8)second_string[last_string_char_index];

			if (first_char > second_char) {
				return first_string_len < second_string_len ? 1 : -1;
			} else if (second_char > first_char) {
				return second_string_len > first_string_len ? 1 : -1;
			} else {
				return first_string_len < second_string_len ? 1 : -1;
			}
		} else {
			return first_string_len == 0 ? 1 : -1;
		}
	}
	return 0;
}

inline int compare_list_lines(void *context, const void *first_line, const void *second_line)
{
	assert(context);
	assert(first_line);
	assert(second_line);

	Gui_Manager *gui_manager = static_cast<Gui_Manager *>(context);
	const Gui_List_Line *first_list_line = static_cast<const Gui_List_Line *>(first_line);
	const Gui_List_Line *second_list_line = static_cast<const Gui_List_Line *>(second_line);

	assert(first_list_line->columns.count == second_list_line->columns.count);

	for (u32 i = 0; i < first_list_line->columns.count; i++) {
		if (!strcmp(first_list_line->columns[i].name, gui_manager->picked_column->name) && !strcmp(second_list_line->columns[i].name, gui_manager->picked_column->name)) {
			Gui_Line_Column *first_line_column = (Gui_Line_Column *)&first_list_line->columns[i];
			Gui_Line_Column *second_line_column = (Gui_Line_Column *)&second_list_line->columns[i];

			assert(first_line_column->rendering_data_list.count == second_line_column->rendering_data_list.count);

			for (u32 j = 0; j < first_line_column->rendering_data_list.count; j++) {
				Column_Rendering_Data *first_line_column_data = &first_line_column->rendering_data_list[j];
				Column_Rendering_Data *second_line_column_data = &second_line_column->rendering_data_list[j];

				assert(first_line_column_data->type == second_line_column_data->type);

				if (first_line_column_data->type == LIST_LINE_INFO_TYPE_TEXT) {
					if (gui_manager->picked_column->state == GUI_LIST_COLUMN_STATE_SORTING_UP) {
						return compare_strings_priority(first_line_column_data->text, second_line_column_data->text);
					} else {
						return -1 * compare_strings_priority(first_line_column_data->text, second_line_column_data->text);
					}
				}
			}
			break;
		}
	}
	return 0;
}

bool Gui_Manager::begin_list(const char *name, Gui_List_Column columns[], u32 columns_count)
{
	assert(name);
	assert(columns_count > 0);

	line_list.count = 0;
	list_column_rect_list.count = 0;

	Gui_Window_Theme window_theme;
	window_theme.background_color = Color(40, 40, 40);
	window_theme.header_color = Color(36, 36, 36);
	window_theme.place_between_rects = 0;
	window_theme.horizontal_offset_from_sides = 0;
	window_theme.vertical_offset_from_sides = 0;
	gui::set_theme(&window_theme);

	Size_s32 window_list_size = list_theme.window_size;
	set_next_window_size(window_list_size.width, window_list_size.height);

	Rect_s32 column_top_rect;
	place_rect_on_window_top(list_theme.filter_rect_height, &column_top_rect);

	u32 window_style = WINDOW_SCROLL_BAR;
	if (begin_child(name, window_style)) {
		line_list.count = 0;

		u32 picked_column_index = UINT32_MAX;
		for (u32 i = 0; i < columns_count; i++) {
			Gui_List_Column_State sorting = columns[i].state;
			if (sorting != GUI_LIST_COLUMN_STATE_DEFAULT) {
				picked_column_index = i;
				break;
			}
		}
		if (picked_column_index == UINT32_MAX) {
			picked_column_index = 0;
			columns[0].state = GUI_LIST_COLUMN_STATE_SORTING_DOWN;
			picked_column = &columns[0];
		}

		Gui_Window *window = get_window();
		window->place_rect_over_window(&column_top_rect);

		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&window->parent_window->clip_rect);
		render_list->add_rect(&column_top_rect, list_theme.filter_rect_color, window_theme.rounded_border, ROUND_TOP_RECT);

		s32 offset_in_persents = 0;
		s32 prev_offset_in_persents = 0;
		s32 half_difference = (list_theme.filter_rect_height - list_theme.split_line_size) / 2;

		for (u32 i = 0; i < columns_count; i++) {
			offset_in_persents += columns[i].size_in_percents;

			Rect_s32 column_rect;
			column_rect.x = window->rect.x + calculate(window_list_size.width, prev_offset_in_persents);
			column_rect.y = window->rect.y;
			column_rect.width = calculate(window_list_size.width, (s32)columns[i].size_in_percents) - (s32)math::ceil(list_theme.split_line_thickness);
			column_rect.height = list_theme.filter_rect_height;
			list_column_rect_list.push(column_rect);

			Rect_s32 column_text_clip_rect = column_rect;
			column_text_clip_rect.width -= list_theme.filter_button_size + list_theme.filter_button_offset;
			column_text_clip_rect = calculate_clip_rect(&window->parent_window->clip_rect, &column_text_clip_rect);
			render_list->push_clip_rect(&column_text_clip_rect);

			Rect_s32 text_rect = get_text_rect(columns[i].name);
			place_in_middle_and_by_left(&column_rect, &text_rect, list_theme.filter_text_offset);
			render_list->add_text(&text_rect, columns[i].name);

			render_list->pop_clip_rect();

			if (detect_intersection(&column_rect) && was_click(KEY_LMOUSE)) {
				if (i != picked_column_index) {
					columns[picked_column_index].state = GUI_LIST_COLUMN_STATE_DEFAULT;
					picked_column_index = i;
				}
				Gui_List_Column_State column_state = columns[picked_column_index].state;
				if (column_state == GUI_LIST_COLUMN_STATE_DEFAULT) {
					columns[i].state = GUI_LIST_COLUMN_STATE_SORTING_DOWN;
				} else if (column_state == GUI_LIST_COLUMN_STATE_SORTING_DOWN) {
					columns[i].state = GUI_LIST_COLUMN_STATE_SORTING_UP;
				} else if (column_state == GUI_LIST_COLUMN_STATE_SORTING_UP) {
					columns[i].state = GUI_LIST_COLUMN_STATE_SORTING_DOWN;
				}
				picked_column = &columns[i];
			}

			if (picked_column_index == i) {
				Rect_s32 button_rect = { 0, 0, list_theme.filter_button_size, list_theme.filter_button_size };
				place_in_middle_and_by_right(&column_rect, &button_rect, list_theme.filter_button_offset);

				if (columns[i].state == GUI_LIST_COLUMN_STATE_SORTING_UP) {
					render_list->add_texture(&button_rect, &up_texture);
				} else if (columns[i].state == GUI_LIST_COLUMN_STATE_SORTING_DOWN) {
					render_list->add_texture(&button_rect, &down_texture);
				}
			}
			if (offset_in_persents < 100) {
				Point_s32 first_point = { window->rect.x + calculate(window_list_size.width, offset_in_persents), window->rect.y + half_difference };
				Point_s32 second_point = { window->rect.x + calculate(window_list_size.width, offset_in_persents), window->rect.y + list_theme.filter_rect_height - half_difference };
				render_list->add_line(&first_point, &second_point, list_theme.split_lines_color, list_theme.split_line_thickness);
			}
			prev_offset_in_persents += columns[i].size_in_percents;

		}
		render_list->pop_clip_rect();
		return true;
	}
	gui::reset_window_theme();
	return false;
}

void Gui_Manager::end_list()
{
	Gui_Window *window = get_window();
	Render_Primitive_List *render_list = GET_RENDER_LIST();
	render_list->push_clip_rect(&window->clip_rect);
	
	qsort_s((void *)line_list.items, line_list.count, sizeof(Gui_List_Line), compare_list_lines, (void *)this);

	Rect_s32 line_rect = { 0, 0, get_window_size().width, list_theme.line_height };
	
	for (u32 line_index = 0; line_index < line_list.count; line_index++) {
		place_rect_in_window(window, &line_rect, window->place_between_rects, window->horizontal_offset_from_sides, window->vertical_offset_from_sides);
		if (must_rect_be_drawn(&window->view_rect, &line_rect)) {
			Gui_List_Line *gui_list_line = &line_list[line_index];

			if (*gui_list_line->state > 0x8) {
				*gui_list_line->state = GUI_LIST_LINE_RESET_STATE;
			}
			if (*gui_list_line->state & GUI_LIST_LINE_CLICKED_BY_LEFT_MOUSE) {
				*gui_list_line->state &= ~GUI_LIST_LINE_CLICKED_BY_LEFT_MOUSE;
			}

			if (was_click(KEY_LMOUSE) && !Keys_State::is_key_down(KEY_CTRL)) {
				if (detect_intersection(&line_rect)) {
					*gui_list_line->state = GUI_LIST_LINE_RESET_STATE;
					*gui_list_line->state |= GUI_LIST_LINE_SELECTED;
					*gui_list_line->state |= GUI_LIST_LINE_CLICKED_BY_LEFT_MOUSE;
				} else {
					*gui_list_line->state = GUI_LIST_LINE_RESET_STATE;
				}
			} else if (detect_intersection(&line_rect) && (key_bindings.was_binding_triggered(KEY_CTRL, KEY_LMOUSE))) {
				*gui_list_line->state |= GUI_LIST_LINE_SELECTED;
				*gui_list_line->state |= GUI_LIST_LINE_CLICKED_BY_LEFT_MOUSE;
			}

			Color line_color = detect_intersection(&line_rect) || (*gui_list_line->state & GUI_LIST_LINE_SELECTED) ? list_theme.hover_line_color : list_theme.line_color;
			render_list->add_rect(&line_rect, line_color);

			for (u32 column_index = 0; column_index < gui_list_line->columns.count; column_index++) {
				Gui_Line_Column *column = &gui_list_line->columns[column_index];
				for (u32 i = 0; i < column->rendering_data_list.count; i++) {
					Column_Rendering_Data *rendering_data = &column->rendering_data_list[i];
					
					if (rendering_data->type == LIST_LINE_INFO_TYPE_TEXT) {
						Rect_s32 *filter_column_rect = &list_column_rect_list[column_index];
						Rect_s32 line_column_rect = { filter_column_rect->x, line_rect.y, filter_column_rect->width, line_rect.height };
						Rect_s32 line_column_text_rect = get_text_rect(rendering_data->text);
						
						place_in_middle_and_by_left(&line_column_rect, &line_column_text_rect, list_theme.line_text_offset);
						
						Rect_s32 clip_rect = calculate_clip_rect(&window->clip_rect, &line_column_rect);
						render_list->push_clip_rect(&clip_rect);
						render_list->add_text(&line_column_text_rect, rendering_data->text);
						render_list->pop_clip_rect();
					}
				}
			}
		}
	}
	
	render_list->pop_clip_rect();
	end_child();	
	gui::reset_window_theme();
}

bool Gui_Manager::begin_line(Gui_List_Line_State *list_line_state)
{
	assert(list_line_state);
#ifdef _DEBUG
	ASSERT_MSG(list_line_debug_counter == 0, "You forgot to use gui::end_line");
	list_line_debug_counter++;
#endif
	current_list_line.state = list_line_state;
	current_list_line.columns.count = 0;
	return true;
}

void Gui_Manager::end_line()
{
#ifdef _DEBUG
	list_line_debug_counter--;
	ASSERT_MSG(list_line_debug_counter == 0, "You forgot to use gui::begin_line");
#endif
	line_list.push(current_list_line);
}

bool Gui_Manager::begin_column(const char *filter_name)
{
#ifdef _DEBUG
	ASSERT_MSG(list_column_debug_counter == 0, "You forgot to use gui::end_column");
	list_column_debug_counter++;
#endif
	current_list_column.name = filter_name;
	current_list_column.rendering_data_list.count = 0;
	return true;
}

void Gui_Manager::end_column()
{
#ifdef _DEBUG
	list_column_debug_counter--;
	ASSERT_MSG(list_column_debug_counter == 0, "You forgot to use gui::begin_column");
#endif
	current_list_line.columns.push(current_list_column);
}

void Gui_Manager::add_text(const char *text, Alignment alignment)
{
	Column_Rendering_Data data;
	data.type = LIST_LINE_INFO_TYPE_TEXT;
	data.text = text;
	current_list_column.rendering_data_list.push(data);
}

void Gui_Manager::add_image(Texture2D *texture, Alignment alignment)
{
}

void Gui_Manager::add_image_button(Texture2D *texture, Alignment alignment)
{
}

bool Gui_Manager::handle_edit_field_shortcut_event(Gui_Window *window, Rect_s32 *edit_field_rect, Gui_ID edit_field_gui_id, Edit_Field_Type edit_field_type, u32 vector3_edit_field_index) {
	bool update_value = false;
	static bool move_vertically = false;

	if (key_bindings.is_binding_down(KEY_CTRL, KEY_BACKSPACE)) {
		if (edit_field_state.caret_index_in_text > -1) {
			String remaining_substring = String(edit_field_state.data, edit_field_state.caret_index_for_inserting, edit_field_state.data.len);
			edit_field_state.caret.x = edit_field_rect->x + edit_field_theme.text_shift;
			edit_field_state.caret_index_for_inserting = 0;
			edit_field_state.caret_index_in_text = -1;
			edit_field_state.data = remaining_substring;
			update_value = true;
		}
	}
	if (!change_active_field) {
		if (edit_field_type == EDIT_FIELD_TYPE_VECTOR3) {
			if (key_bindings.was_binding_triggered(KEY_CTRL, KEY_ARROW_LEFT)) {
				if (((s32)vector3_edit_field_index - 1) >= 0) {
					active_edit_field = window->gui_id + EDIT_FIELD_HASH + edit_field_count - 1;
					update_value = true;
					change_active_field = true;
					move_vertically = false;
				}
			}
			if (key_bindings.was_binding_triggered(KEY_CTRL, KEY_ARROW_RIGHT)) {
				if ((vector3_edit_field_index + 1) < VECTOR3_EDIT_FIELD_NUMBER) {
					active_edit_field = window->gui_id + EDIT_FIELD_HASH + edit_field_count + 1;
					update_value = true;
					change_active_field = true;
					move_vertically = false;
				}
			}
		}
		if (key_bindings.was_binding_triggered(KEY_CTRL, KEY_ARROW_UP)) {
			if (edit_field_type == EDIT_FIELD_TYPE_COMMON) {
				if (window->edit_field_count > 0) {
					active_edit_field = window->gui_id + EDIT_FIELD_HASH + edit_field_count - 1;
					update_value = true;
					change_active_field = true;
				}
			} else if (edit_field_type == EDIT_FIELD_TYPE_VECTOR3) {
				if (((s32)window->edit_field_count - (s32)vector3_edit_field_index) > 0) {
					active_edit_field = window->gui_id + EDIT_FIELD_HASH + edit_field_count - (vector3_edit_field_index + 1);
					update_value = true;
					change_active_field = true;
					move_vertically = true;
				}
			} else {
				loop_print("Gui_Manager::handle_edit_field_shortcuts_event: Failed to handle key binding trigger. Unknown edit field type was passed.");
			}
		}
		if (key_bindings.was_binding_triggered(KEY_CTRL, KEY_ARROW_DOWN)) {
			if (edit_field_type == EDIT_FIELD_TYPE_COMMON) {
				if ((window->edit_field_count + 1) < window->max_edit_field_number) {
					active_edit_field = window->gui_id + EDIT_FIELD_HASH + edit_field_count + 1;
					update_value = true;
					change_active_field = true;
				}
			} else if (edit_field_type == EDIT_FIELD_TYPE_VECTOR3) {
				if ((window->edit_field_count + VECTOR3_EDIT_FIELD_NUMBER) < window->max_edit_field_number) {
					active_edit_field = window->gui_id + EDIT_FIELD_HASH + edit_field_count + (VECTOR3_EDIT_FIELD_NUMBER - vector3_edit_field_index);
					update_value = true;
					change_active_field = true;
					move_vertically = true;
				}
			} else {
				loop_print("Gui_Manager::handle_edit_field_shortcuts_event: Failed to handle key binding trigger. Unknown edit field type was passed.");
			}
		}
	} else {
		if ((vector3_edit_field_index == 2) && move_vertically) {
			active_edit_field = window->gui_id + EDIT_FIELD_HASH + edit_field_count - 2;
			update_value = true;
			change_active_field = true;
		} else {
			change_active_field = false;
		}
	}
	return update_value;
}

bool Gui_Manager::update_edit_field(Edit_Field_Instance *edit_field_instance)
{
	bool update_editing_value = false;
	static bool update_next_time_editing_value = false;

	Gui_Window *window = get_window();

	Gui_ID edit_field_gui_id = GET_EDIT_FIELD_GUI_ID();
	update_active_and_hot_state(window, edit_field_gui_id, &edit_field_instance->edit_field_rect);

	if (was_click(KEY_LMOUSE)) {
		if (hot_item == edit_field_gui_id) {
			active_edit_field = edit_field_gui_id;
			edit_field_state = make_edit_field_state(&edit_field_instance->caret_rect, edit_field_instance->editing_value, edit_field_instance->max_chars_number, edit_field_instance->symbol_validation);
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
		if (change_active_field) {
			edit_field_state = make_edit_field_state(&edit_field_instance->caret_rect, edit_field_instance->editing_value, edit_field_instance->max_chars_number, edit_field_instance->symbol_validation);
		}
		if (!Keys_State::is_key_down(KEY_CTRL)) {
			handle_events(&update_editing_value, &update_next_time_editing_value, &edit_field_instance->edit_field_rect, &edit_field_instance->value_rect);
		}
	}
	return update_editing_value;
}

void Gui_Manager::handle_events(bool *update_editing_value, bool *update_next_time_editing_value, Rect_s32 *rect, Rect_s32 *editing_value_rect)
{
	Queue<Event> *events = get_event_queue();
	for (Queue_Node<Event> *node = events->first; node != NULL; node = node->next) {
		Event *event = &node->item;
		if (event->type == EVENT_TYPE_KEY) {
			if (was_click(KEY_LMOUSE)) {
				set_caret_position_on_mouse_click(rect, editing_value_rect);
			}
			if (event->is_key_down(KEY_BACKSPACE)) {
				if (edit_field_state.caret_index_in_text > -1) {
					*update_editing_value = true;
					char c = edit_field_state.data[edit_field_state.caret_index_in_text];
					edit_field_state.data.remove(edit_field_state.caret_index_in_text);

					edit_field_state.caret.x -= (s32)font->get_char_advance(c);
					edit_field_state.caret_index_in_text -= 1;
					edit_field_state.caret_index_for_inserting -= 1;
				}
			} else if (event->is_key_down(KEY_ARROW_LEFT)) {
				if (edit_field_state.caret_index_in_text > -1) {
					char c = edit_field_state.data[edit_field_state.caret_index_in_text];

					edit_field_state.caret.x -= (s32)font->get_char_advance(c);
					edit_field_state.caret_index_in_text -= 1;
					edit_field_state.caret_index_for_inserting -= 1;
				}
			} else if (event->is_key_down(KEY_ARROW_RIGHT)) {
				if (edit_field_state.caret_index_in_text < ((s32)edit_field_state.data.len - 1)) {
					edit_field_state.caret_index_in_text += 1;
					edit_field_state.caret_index_for_inserting += 1;

					char c = edit_field_state.data[edit_field_state.caret_index_in_text];
					edit_field_state.caret.x += (s32)font->get_char_advance(c);
				}
			} else if (event->is_key_down(KEY_HOME)) {
				Size_u32 size = font->get_text_size(edit_field_state.data);
				edit_field_state.caret.x = editing_value_rect->x;
				edit_field_state.caret_index_for_inserting = 0;
				edit_field_state.caret_index_in_text = -1;

			} else if (event->is_key_down(KEY_END)) {
				Size_u32 size = font->get_text_size(edit_field_state.data);
				edit_field_state.caret.x = editing_value_rect->x + size.width;
				edit_field_state.caret_index_for_inserting = edit_field_state.data.len;
				edit_field_state.caret_index_in_text = edit_field_state.data.len - 1;

			} else if (event->is_key_down(KEY_ENTER)) {
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
				*update_editing_value = true;
				if (edit_field_state.caret_index_in_text == (edit_field_state.data.len - 1)) {
					edit_field_state.data.append(event->char_key);
				} else {
					edit_field_state.data.insert(edit_field_state.caret_index_for_inserting, event->char_key);
				}
				edit_field_state.caret_index_in_text += 1;
				edit_field_state.caret_index_for_inserting += 1;

				edit_field_state.caret.x += (s32)font->get_char_advance(event->char_key);
			}
		}
	}
}

void Gui_Manager::set_caret_position_on_mouse_click(Rect_s32 *rect, Rect_s32 *editing_value_rect)
{
	u32 text_width = font->get_text_width(edit_field_state.data);
	u32 mouse_x_relative_text = (u32)math::abs(mouse_x - rect->x - edit_field_theme.text_shift);

	if (mouse_x_relative_text > text_width) {
		edit_field_state.caret.x = editing_value_rect->x + text_width;
		edit_field_state.caret_index_for_inserting = edit_field_state.data.len;
		edit_field_state.caret_index_in_text = edit_field_state.data.len - 1;
	} else if ((mouse_x >= rect->x) && (mouse_x <= (rect->x + edit_field_theme.text_shift))) {
		edit_field_state.caret.x = rect->x + edit_field_theme.text_shift;
		edit_field_state.caret_index_for_inserting = 0;
		edit_field_state.caret_index_in_text = -1;
	} else {
		u32 chars_width = 0;
		u32 prev_chars_advance_width = 0;

		String *text = &edit_field_state.data;
		for (u32 i = 0; i < text->len; i++) {
			char c = text->data[i];
			chars_width += font->get_char_advance(c);

			if ((mouse_x_relative_text >= prev_chars_advance_width) && (mouse_x_relative_text <= chars_width)) {
				edit_field_state.caret.x = rect->x + edit_field_theme.text_shift + chars_width;
				edit_field_state.caret_index_for_inserting = i + 1;
				edit_field_state.caret_index_in_text = i;
				break;
			}
		}
	}
}

void Gui_Manager::make_tab_active(Gui_ID tab_gui_id)
{
	active_tab = tab_gui_id;
	active_item = tab_gui_id;
	hot_item = tab_gui_id;
}

void Gui_Manager::text(const char *some_text)
{
	Gui_Window *window = get_window();

	Rect_s32 text_rect = get_text_rect(some_text);
	text_rect.height = 20;
	Rect_s32 alignment_text_rect = get_text_rect(some_text);

	place_rect_in_window(window, &text_rect, window->place_between_rects, window->horizontal_offset_from_sides, window->vertical_offset_from_sides);

	if (must_rect_be_drawn(&window->clip_rect, &text_rect)) {
		place_in_middle(&text_rect, &alignment_text_rect, BOTH_AXIS);
		
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&window->clip_rect);
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

	if (must_rect_be_drawn(&window->clip_rect, &tab_rect)) {
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		
		if (!window->tab_was_drawn) {
			window->tab_was_drawn = true;
			render_list->add_rect(&tab_bar_rect, tab_theme.tab_bar_color);
		}
		place_in_middle(&tab_rect, &name_rect, BOTH_AXIS);

		render_list->push_clip_rect(&window->clip_rect);

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

	Rect_s32 image_rect = { 0, 0, width, height };

	Gui_Window *window = get_window();
	place_rect_in_window(window, &image_rect, window->place_between_rects, window->horizontal_offset_from_sides, window->vertical_offset_from_sides);

	if (must_rect_be_drawn(&window->clip_rect, &image_rect)) {
		
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&window->clip_rect);
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
	Rect_s32 list_box_rect = list_box_theme.default_rect;
	Rect_s32 expand_down_texture_rect = list_box_theme.expand_down_texture_rect;
	
	Gui_Window *window = get_window();
	place_rect_in_window(window, &list_box_rect, window->place_between_rects, window->horizontal_offset_from_sides, window->vertical_offset_from_sides);

	if (must_rect_be_drawn(&window->clip_rect, &list_box_rect)) {

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

		if (was_click(KEY_LMOUSE)) {
			if (hot_item == list_box_gui_id) {
				if (active_list_box != active_item) {
					active_list_box = active_item;
				} else {
					active_list_box = 0;
				}
			} else {
				if ((active_list_box == list_box_gui_id) && !detect_intersection(&drop_window_rect)) {
					active_list_box = 0;
				}
			}
		}

		u32 alignment = LEFT_ALIGNMENT;
		if (active_list_box == list_box_gui_id) {
			Gui_Window_Theme win_theme;
			win_theme.horizontal_offset_from_sides = 0;
			win_theme.vertical_offset_from_sides = 0;
			win_theme.outlines_width = 1.0f;
			win_theme.place_between_rects = 0;
			set_next_window_theme(&win_theme);

			set_next_window_pos(drop_window_rect.x, drop_window_rect.y);
			set_next_window_size(drop_window_rect.width, drop_window_rect.height);

			//@Note: May be create string for list_box_gui_id is a temporary decision.
			// I will refactoring this code when make normal working child windows.
			begin_window(String((int)list_box_gui_id), WINDOW_OUTLINES);

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

		Rect_s32 name_rect = get_text_rect(array->get(*item_index));
		
		if (alignment & RIGHT_ALIGNMENT) {
			place_in_middle_and_by_right(&list_box_rect, &name_rect, list_box_theme.shift_from_size);
		} else if (alignment & LEFT_ALIGNMENT) {
			place_in_middle_and_by_left(&list_box_rect, &name_rect, list_box_theme.shift_from_size);
		} else {
			place_in_middle(&list_box_rect, &name_rect, BOTH_AXIS);
		}
		place_in_middle_and_by_right(&list_box_rect, &expand_down_texture_rect, list_box_theme.expand_down_texture_shift);
		
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&window->clip_rect);
		render_list->add_rect(&list_box_rect, list_box_theme.background_color, list_box_theme.rounded_border);
		render_list->add_text(&name_rect, array->get(*item_index));
		render_list->add_texture(&expand_down_texture_rect, &expand_down_texture);
		render_list->pop_clip_rect();
	}
	list_box_count++;
}

bool Gui_Manager::button(const char *name, bool *state)
{
	bool left_mouse_click;
	bool right_mouse_click;
	core_button(name, &left_mouse_click, &right_mouse_click);
	return left_mouse_click;
}

void Gui_Manager::core_button(const char *name, bool *left_mouse_click, bool *right_mouse_click)
{
	*left_mouse_click = false;
	*right_mouse_click = false;

	Rect_s32 button_rect = button_theme.rect;

	Gui_Window *window = get_window();
	place_rect_in_window(window, &button_rect, window->place_between_rects, window->horizontal_offset_from_sides, window->vertical_offset_from_sides);

	bool mouse_hover = false;
	if (must_rect_be_drawn(&window->clip_rect, &button_rect)) {

		u32 button_gui_id = GET_BUTTON_GUI_ID();
		update_active_and_hot_state(window, button_gui_id, &button_rect);

		mouse_hover = (hot_item == button_gui_id);

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
		render_list->push_clip_rect(&window->clip_rect);
		render_list->add_rect(&button_rect, button_color, button_theme.rounded_border);
		render_list->add_text(&name_rect, name);
		render_list->pop_clip_rect();
	}
	if (was_click(KEY_LMOUSE) && mouse_hover) {
		*left_mouse_click = true;
	}
	if (was_click(KEY_RMOUSE) && mouse_hover) {
		*right_mouse_click = true;
	}
	button_count++;
}

bool Gui_Manager::can_window_be_resized(Gui_Window *window)
{
	bool can_resize = true;
	Gui_Window *collided_window = NULL;
	For(window->collided_windows, collided_window) {
		if ((collided_window->index_in_windows_order > window->index_in_windows_order) && detect_intersection(&collided_window->rect)) {
			return false;
		}
	}
	return true;
}

void Gui_Manager::init(Engine *engine, const char *font_name, u32 font_size)
{
	render_2d = &engine->render_sys.render_2d;
	win32_info = &engine->win32_info;

	active_scrolling = false;
	change_active_field = false;
	any_window_was_moved = false;
	handle_events_for_one_window = false;
	resizing_window = 0;
	probably_resizing_window = 0;
	edit_field_count = 0;
	window_parent_count = 0;
	reset_window_params = 0;
	curr_parent_windows_index_sum = 0;
	prev_parent_windows_index_sum = 0;
	window_theme = DEFAULT_WINDOW_THEME;

	key_bindings.bind(KEY_CTRL, KEY_BACKSPACE);
	key_bindings.bind(KEY_CTRL, KEY_ARROW_UP);
	key_bindings.bind(KEY_CTRL, KEY_ARROW_DOWN);
	key_bindings.bind(KEY_CTRL, KEY_ARROW_LEFT);
	key_bindings.bind(KEY_CTRL, KEY_ARROW_RIGHT);
	key_bindings.bind(KEY_CTRL, KEY_LMOUSE);

	set_font(font_name, font_size);
	init_from_save_file();
	load_and_init_textures(&engine->render_sys.gpu_device);
}

void Gui_Manager::init_from_save_file() 
{
	String path_to_save_file;
	build_full_path_to_gui_file("new_gui_data.gui", path_to_save_file);

	File save_file;
	if (!save_file.open(path_to_save_file, FILE_MODE_READ, FILE_OPEN_EXISTING)) {
		print("Gui_Manager::init_from_save_file: Hades gui file was not found.");
		return;
	}

	u32 window_count = 0;
	save_file.read((void *)&window_count, sizeof(u32));

	if (window_count > 0) {
		print("Gui_Manager::init_from_save_file: Create windows from a save file.");
	}

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

		create_window(string, WINDOW_TYPE_PARENT, window_style, &rect);
		print("  Window {} {}x{} was created.", string, rect.width, rect.height);
		free_string(string);
	}
}

struct Name_Texture {
	const char *name = NULL;
	Texture2D *texture = NULL;
};

void Gui_Manager::load_and_init_textures(Gpu_Device *gpu_device) 
{
	assert(gpu_device);

	const u32 TEXTURE_COUNT = 5;
	Name_Texture pairs[TEXTURE_COUNT] = { {"expand_down1.png", &expand_down_texture }, { "cross-small.png", &cross_texture }, { "check2.png", &check_texture }, { "up.png", &up_texture }, {"down.png", &down_texture } };

	Texture2D_Desc texture_desc;
	texture_desc.width = 64;
	texture_desc.height = 64;
	texture_desc.mip_levels = 1;
	
	gpu_device->create_texture_2d(&texture_desc, &default_texture);
	gpu_device->create_shader_resource_view(&texture_desc, &default_texture);
	
	Color transparent_value = Color(255, 255, 255, 0);
	fill_texture((void *)&transparent_value, &default_texture);

	u8 *image_data = NULL;
	u32 width = 0;
	u32 height = 0;
	String full_path;
	for (u32 i = 0; i < TEXTURE_COUNT; i++) {
		build_full_path_to_editor_file(pairs[i].name, full_path);
		if (load_png_file(full_path, &image_data, &width, &height)) {
			texture_desc.width = width;
			texture_desc.height = height;
			texture_desc.data = (void *)image_data;
			gpu_device->create_texture_2d(&texture_desc, pairs[i].texture);
			gpu_device->create_shader_resource_view(&texture_desc, pairs[i].texture);
			DELETE_PTR(image_data);
		} else {
			print("Gui_Manager::load_and_init_textures: Failed to load {}.", pairs[i].name);
			*pairs[i].texture = default_texture;
		}
	}
}

void Gui_Manager::handle_events()
{
	key_bindings.handle_events();
}

void Gui_Manager::set_font(const char *font_name, u32 font_size)
{
	Font *new_font = Engine::get_font_manager()->get_font(font_name, font_size);
	if (!new_font) {
		print(" Gui_Manager::set_font: Failed to set new font '{}' wit size {}. Font was not found.", font_name, font_size);
	}
	font = new_font;
	
	Render_Font *new_render_font = NULL;
	char *formatted_font_name = format("{}_{}", font_name, font_size);
	if (!render_2d->render_fonts.get(formatted_font_name, &new_render_font)) {
		print(" Gui_Manager::set_font: Failed to set new font '{}' wit size {}. Render font was not found.", font_name, font_size);
		free_string(formatted_font_name);
	}
	free_string(formatted_font_name);
	render_font = new_render_font;
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
	mouse_x = Mouse_State::x;
	mouse_y = Mouse_State::y;
	mouse_x_delta = mouse_x - last_mouse_x;
	mouse_y_delta = mouse_y - last_mouse_y;
	hot_item = 0;
	button_count = 0;
	image_button_count = 0;
	radio_button_count = 0;
	tab_count = 0;
	list_count = 0;
	list_box_count = 0;
	edit_field_count = 0;
	became_just_actived = 0;

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
				if ((checking_window->gui_id != checked_window->gui_id) && detect_intersection(&checking_window->rect, &checked_window->rect)) {
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
		if (detect_intersection(&window->rect)) {
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
	} else {
		if (handle_events_for_one_window) {
			window_events_handler_id = 0;
			handle_events_for_one_window = false;
		}
	}
}

void Gui_Manager::end_frame()
{
	if (!Keys_State::is_key_down(KEY_LMOUSE)) {
		active_item = 0;
		resizing_window = 0;
	}
	last_mouse_x = Mouse_State::x;
	last_mouse_y = Mouse_State::y;
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

bool Gui_Manager::begin_window(const char *name, Window_Style window_style)
{	
	u32 window_index = 0;
	Gui_Window *window = find_window(name, &window_index);
	
	if (!window) {
		window = create_window(name, WINDOW_TYPE_PARENT, window_style);
		window_index = windows.count - 1;
		became_just_actived = window->gui_id;		
		active_window = window->gui_id;
	}
	
	if (!window->open) {
		return false;
	}

	Rect_s32 prev_view_rect = window->view_rect;
	Rect_s32 prev_content_rect = window->content_rect;
	
	window_stack.push(window_index);
	window->new_frame(window_style);

	//@Note: Can I get rid of it ?
	update_active_and_hot_state(window, window->gui_id, &window->rect);
	
	update_active_window(window);
	
	Rect_s32 *rect = &window->rect;

	bool mouse_was_moved = (mouse_x_delta != 0) || (mouse_y_delta != 0);
	if (mouse_was_moved && !active_scrolling && (resizing_window == 0) && (active_window == window->gui_id) && Keys_State::is_key_down(KEY_LMOUSE)) {
		s32 min_window_position = 0;
		if (window->style & WINDOW_OUTLINES) {
			min_window_position = (s32)window_theme.outlines_width;
		}
		s32 x = math::clamp(rect->x + mouse_x_delta, min_window_position, (s32)win32_info->window_width - rect->width);
		s32 y = math::clamp(rect->y + mouse_y_delta, min_window_position, (s32)win32_info->window_height - rect->height);
		window->set_position(x, y);
		any_window_was_moved = true;
	}

	static Rect_Side rect_side; 
	static Cursor_Type cursor_type = CURSOR_TYPE_ARROW;
	if ((WINDOW_RESIZABLE & window_style) && !active_scrolling && (resizing_window == 0) && detect_collision_window_borders(rect, &rect_side) && can_window_be_resized(window)) {
		probably_resizing_window = window->gui_id;
		window_handing_events = true;

		if ((rect_side == RECT_SIDE_LEFT) || (rect_side == RECT_SIDE_RIGHT)) {
			cursor_type = CURSOR_TYPE_RESIZE_LEFT_RIGHT;
		} else if ((rect_side == RECT_SIDE_BOTTOM) || (rect_side == RECT_SIDE_TOP)) {
			cursor_type = CURSOR_TYPE_RESIZE_TOP_BUTTOM;
		} else if (rect_side == RECT_SIDE_RIGHT_BOTTOM) {
			cursor_type = CURSOR_TYPE_RESIZE_TOP_LEFT;
		} else if (rect_side == RECT_SIDE_LEFT_BOTTOM) {
			cursor_type = CURSOR_TYPE_RESIZE_TOP_RIGHT;
		}
		set_cursor(cursor_type);
		if (was_key_just_pressed(KEY_LMOUSE)) {
			resizing_window = window->gui_id;
			became_just_actived = window->gui_id;
		}
	} else {
		if (probably_resizing_window == window->gui_id) {
			probably_resizing_window = 0;
			window_handing_events = false;
			set_cursor(CURSOR_TYPE_ARROW);
		}
	}
	
	if (was_key_just_released(KEY_LMOUSE) && (resizing_window == window->gui_id)) {
		window_handing_events = false;
		resizing_window = 0;
		set_cursor(CURSOR_TYPE_ARROW);
	}

	if ((resizing_window == window->gui_id) && Keys_State::is_key_down(KEY_LMOUSE)) {
		set_cursor(cursor_type);
		window_handing_events = true;
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
			if (window->display_vertical_scrollbar) {
				if (prev_view_rect.bottom() == prev_content_rect.bottom()) {
					s32 delta = rect->bottom() - prev_content_rect.bottom();
					window->content_rect.y += delta;
				}
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
	
	render_window(window);
	window->clip_rect = window->view_rect;


	window->place_between_rects = window_theme.place_between_rects;
	window->horizontal_offset_from_sides = window_theme.horizontal_offset_from_sides;
	window->vertical_offset_from_sides = window_theme.vertical_offset_from_sides;

	return window->open;
}

void Gui_Manager::end_window()
{
	u32 window_index = window_stack.top();
	Gui_Window *window = get_window();
	
	if (window->index_in_windows_order < 0) {
		window->set_index_with_offset(windows_order.count);
		windows_order.push(window_index);
	
	} else if ((became_just_actived == window->gui_id) && (window->index_in_windows_order != -1)) {
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

	window->content_rect.width += window_theme.horizontal_offset_from_sides;
	window->content_rect.height += window_theme.vertical_offset_from_sides;

	if (window->display_vertical_scrollbar) {
		scrolling(window, Y_AXIS);
	}

	if (window->display_horizontal_scrollbar) {
		scrolling(window, X_AXIS);
	}

	if ((window->style & WINDOW_SCROLL_BAR) && (window->content_rect.height > window->view_rect.height)) {
		window->display_vertical_scrollbar = true;
	} else {
		window->display_vertical_scrollbar = false;
	}

	if ((window->style & WINDOW_SCROLL_BAR) && ((window->content_rect.width > window->view_rect.width) || (window->scroll[X_AXIS] > window->view_rect.x))) {
		window->display_horizontal_scrollbar = true;
	} else {
		window->display_horizontal_scrollbar = false;
	}

#if DRAW_WINDOW_DEBUG_RECTS
	draw_debug_rect(window, &window->view_rect, 0.1f);
	draw_debug_rect(window, &window->content_rect, 0.2f, Color::Green);
#endif
	window_rect = DEFAULT_WINDOW_RECT;
	
	if (reset_window_params & SET_WINDOW_THEME) {
		reset_window_params &= ~SET_WINDOW_THEME;
		window_theme = DEFAULT_WINDOW_THEME;
	}
	window_stack.pop();
}

void Gui_Manager::render_window(Gui_Window *window)
{
	Rect_s32 *rect = &window->rect;

	Render_Primitive_List *render_list = GET_RENDER_LIST();
	render_list->add_rect(&window->rect, window_theme.background_color, window_theme.rounded_border);

	if (window->style & WINDOW_OUTLINES) {
		render_list->add_outlines(rect->x, rect->y, rect->width, rect->height, window_theme.outlines_color, window_theme.outlines_width, window_theme.rounded_border);
	}
	if (window->style & WINDOW_HEADER) {
		Rect_s32 header_rect = { rect->x, rect->y, rect->width, window_theme.header_height };
		window->place_rect_over_window(&header_rect);
		render_list->add_rect(&header_rect, window_theme.header_color, window_theme.rounded_border, ROUND_TOP_RECT);

		if (window->type == WINDOW_TYPE_CHILD) {
			Rect_s32 temp = calculate_clip_rect(&window->parent_window->clip_rect, &header_rect);
			render_list->push_clip_rect(&temp);
		} else {
			render_list->push_clip_rect(&header_rect);
		}

		Rect_s32 name_rect = get_text_rect(window->name);
		place_in_middle(&header_rect, &name_rect, BOTH_AXIS);
		render_list->add_text(&name_rect, window->name);

		if (window->style & WINDOW_CLOSE_BUTTON) {
			Rect_s32 cross_texture_rect = { 0, 0, header_rect.height, header_rect.height };
			place_in_middle_and_by_right(&header_rect, &cross_texture_rect, 5);
			if (image_button(&cross_texture_rect, &cross_texture, &window->rect)) {
				window->open = false;
			}
		}
		render_list->pop_clip_rect();
	}
	if (window->display_vertical_scrollbar) {
		Rect_s32 scroll_bar_rect = window->get_scrollbar_rect(Y_AXIS);
		window->place_rect_over_window(&scroll_bar_rect);
		u32 flags = window->style & WINDOW_HEADER ? ROUND_BOTTOM_RECT : ROUND_RECT;
		render_list->add_rect(&scroll_bar_rect, window_theme.scroll_bar_color, window_theme.rounded_border, flags);
	}

	if (window->display_horizontal_scrollbar) {
		Rect_s32 scroll_bar_rect = window->get_scrollbar_rect(X_AXIS);
		window->place_rect_over_window(&scroll_bar_rect);
		u32 flags = window->style & WINDOW_HEADER ? ROUND_BOTTOM_RECT : ROUND_RECT;
		render_list->add_rect(&scroll_bar_rect, window_theme.scroll_bar_color, window_theme.rounded_border, ROUND_RECT);
	}
}

bool Gui_Manager::begin_child(const char *name, Window_Style window_style)
{
	assert(!window_stack.is_empty());

	u32 window_index = 0;
	Gui_Window *parent_window = get_window();
	Gui_Window *child_window = find_window(name, &window_index);
	
	if (!child_window) {
		Rect_s32 rect = { 10, 10, 200, 200 };
		child_window = create_window(name, WINDOW_TYPE_CHILD, window_style, &rect);
		window_index = child_window->index_in_windows_array;
		child_window->type = WINDOW_TYPE_CHILD;
		//Windows array could be resized.
		parent_window = get_window();
		parent_window->child_windows.push(child_window);
		child_window->parent_window = parent_window;
	}
	
	Rect_s32 child_rect = { 0, 0, child_window->rect.width, child_window->rect.height };
	place_rect_in_window(parent_window, &child_rect, parent_window->place_between_rects, parent_window->horizontal_offset_from_sides, parent_window->vertical_offset_from_sides);
	if (must_rect_be_drawn(&parent_window->view_rect, &child_rect)) {

		child_window->set_position(child_rect.x, child_rect.y);
		child_window->new_frame(window_style);

		update_active_and_hot_state(child_window, child_window->gui_id, &child_window->rect);
		update_active_window(child_window);

		if (reset_window_params & SET_WINDOW_POSITION) {
			child_window->set_position(window_rect.x, window_rect.y);
			reset_window_params &= ~SET_WINDOW_POSITION;
		}

		if (reset_window_params & SET_WINDOW_SIZE) {
			child_window->rect.set_size(window_rect.width, window_rect.height);
			child_window->view_rect = child_window->rect;
			reset_window_params &= ~SET_WINDOW_SIZE;
		}

		if (reset_window_params & SET_WINDOW_THEME) {
			backup_window_theme = window_theme;
			window_theme = future_window_theme;
			reset_window_params &= ~SET_WINDOW_THEME;
		}

		if (reset_window_params & PLACE_RECT_ON_TOP) {
			child_window->style |= WINDOW_HEADER;
			placed_rect->x = child_window->view_rect.x;
			placed_rect->y = child_window->view_rect.y;
			placed_rect->width = child_window->view_rect.width;
			placed_rect->height = placing_rect_height;
			
			if ((child_window->display_vertical_scrollbar) && (child_window->view_rect.y == child_window->scroll.y)) {
				child_window->scroll.y += placing_rect_height;

				if (child_window->view_rect.y == child_window->content_rect.y) {
					child_window->content_rect.y += placing_rect_height;
				}
			}
			child_window->view_rect.offset_y(placing_rect_height);
			reset_window_params &= ~PLACE_RECT_ON_TOP;
		}

		window_stack.push(window_index);

		Render_Primitive_List *render_list = &child_window->render_list;
		render_list->push_clip_rect(&parent_window->clip_rect);
		render_window(child_window);
		render_list->pop_clip_rect();
		
		child_window->clip_rect = calculate_clip_rect(&parent_window->clip_rect, &child_window->view_rect);

		child_window->place_between_rects = window_theme.place_between_rects;
		child_window->horizontal_offset_from_sides = window_theme.horizontal_offset_from_sides;
		child_window->vertical_offset_from_sides = window_theme.vertical_offset_from_sides;

		return true;
	}
	return false;
}

void Gui_Manager::end_child()
{
	Gui_Window *window = get_window();

	window->content_rect.width += window_theme.horizontal_offset_from_sides;
	window->content_rect.height += window_theme.vertical_offset_from_sides;

	if (window->display_vertical_scrollbar) {
		scrolling(window, Y_AXIS);
	}

	if (window->display_horizontal_scrollbar) {
		scrolling(window, X_AXIS);
	}

	if ((window->style & WINDOW_SCROLL_BAR) && (window->content_rect.height > window->view_rect.height)) {
		window->display_vertical_scrollbar = true;
	} else {
		window->display_vertical_scrollbar = false;
	}

	if ((window->style & WINDOW_SCROLL_BAR) && (window->content_rect.width > window->view_rect.width)) {
		window->display_horizontal_scrollbar = true;
	} else {
		window->display_horizontal_scrollbar = false;
	}

	if (reset_window_params & SET_WINDOW_THEME) {
		window_theme = backup_window_theme;
		reset_window_params &= ~SET_WINDOW_THEME;
	}

#if DRAW_CHILD_WINDOW_DEBUG_RECTS
	draw_debug_rect(window, &window->view_rect, 0.1f);
	draw_debug_rect(window, &window->content_rect, 0.2f, Color::Green);
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

void Gui_Manager::scrolling(Gui_Window *window, Axis axis)
{
	Gui_Window_Theme *theme = &window_theme;

	s32 window_view_size = window->view_rect.get_size()[axis];
	s32 window_content_size = window->content_rect.get_size()[axis];
	float ratio = (float)(window_view_size) / (float)window_content_size;

	if (ratio < 1.0f) {
		s32 scroll_size = math::max((s32)((float)window_view_size * ratio), theme->min_scroll_size);

		Rect_s32 scroll_rect;
		if (axis == Y_AXIS) {
			scroll_rect = Rect_s32(window->view_rect.right(), window->scroll.y, theme->scroll_size, scroll_size);
			if (scroll_rect.bottom() > window->view_rect.bottom()) {
				scroll_rect.y += window->view_rect.bottom() - scroll_rect.bottom();
			}
		} else {
			scroll_rect = Rect_s32(window->scroll.x, window->rect.bottom() - theme->scroll_size, scroll_size, theme->scroll_size);
		}
		Gui_ID scroll_bar_gui_id = GET_SCROLL_BAR_GUI_ID() + (u32)axis;
		update_active_and_hot_state(window, scroll_bar_gui_id, &scroll_rect);

		if ((active_item == scroll_bar_gui_id) && was_key_just_pressed(KEY_LMOUSE)) {
			active_scrolling = true;
		}
		if (active_scrolling && was_key_just_released(KEY_LMOUSE)) {
			active_scrolling = false;
		}

		s32 mouse_delta = 0;
		bool update_scroll_position = false;
		if ((active_item == scroll_bar_gui_id) && Keys_State::is_key_down(KEY_LMOUSE)) {
			mouse_delta = (axis == Y_AXIS) ? mouse_y_delta : mouse_x_delta;
			update_scroll_position = true;
		
		} else if ((active_window == window->gui_id) && (axis == Y_AXIS)) {
			Array<s32> *mouse_wheels = Async_Input::get_mouse_wheels();
			for (u32 i = 0; i < mouse_wheels->count; i++) {
				mouse_delta += (mouse_wheels->get(i) > 0) ? -window_theme.mouse_wheel_spped : window_theme.mouse_wheel_spped;
				update_scroll_position = true;
			}
		} 

		if (update_scroll_position) {
			s32 window_side = (axis == Y_AXIS) ? window->view_rect.bottom() : window->view_rect.right();
			scroll_rect[axis] = math::clamp(scroll_rect[axis] + mouse_delta, window->view_rect[axis], window_side - scroll_rect.get_size()[axis]);
			s32 scroll_position_relative_win = scroll_rect[axis] - window->view_rect[axis];
			s32 available_scrolling_distance = window->view_rect.get_size()[axis] - scroll_size;
			s32 content_view_rect_difference = window->content_rect.get_size()[axis] - window->view_rect.get_size()[axis];

			float scroll_ration = (float)scroll_position_relative_win / (float)available_scrolling_distance;
			window->content_rect[axis] = window->view_rect[axis] - (s32)math::ceil((float)content_view_rect_difference * scroll_ration);
		}
		
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		Rect_s32 clip_rect = (window->type == WINDOW_TYPE_PARENT) ? window->rect : calculate_clip_rect(&window->parent_window->rect, &window->rect);
		render_list->push_clip_rect(&clip_rect);
		render_list->add_rect(&scroll_rect, window_theme.scroll_color, scroll_rect.get_size()[math::abs((int)axis - 1)] / 2);
		render_list->pop_clip_rect();

		window->scroll[axis] = scroll_rect[axis];
	}
}

bool Gui_Manager::detect_collision_window_borders(Rect_s32 *rect, Rect_Side *rect_side)
{
	static s32 offset_from_border = 5;
	static s32 tri_size = 10;
	
	Point_s32 mouse = { mouse_x, mouse_y };
	Triangle<s32> left_triangle = Triangle<s32>(Point_s32(rect->x, rect->bottom()), Point_s32(rect->x, rect->bottom() - tri_size), Point_s32(rect->x + tri_size, rect->bottom()));
	Triangle<s32> right_triangle = Triangle<s32>(Point_s32(rect->right() - tri_size, rect->bottom()), Point_s32(rect->right(), rect->bottom() - tri_size), Point_s32(rect->right(), rect->bottom()));

	if (detect_intersection(&left_triangle, &mouse)) {
		*rect_side = RECT_SIDE_LEFT_BOTTOM;
		return true;
	} else if (detect_intersection(&right_triangle, &mouse)) {
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

Gui_ID Gui_Manager::get_last_tab_gui_id()
{
	Gui_Window *window = get_window();
	Gui_ID next_tab_gui_id = GET_TAB_GUI_ID();
	// Calculate current a tab Gui_ID.
	return next_tab_gui_id - 1;
}

static Gui_Manager gui_manager;


void gui::init_gui(Engine *engine, const char *font_name, u32 font_size)
{
	gui_manager.init(engine, font_name, font_size);
}

void gui::handle_events()
{
	gui_manager.handle_events();
}

void gui::shutdown()
{
	gui_manager.shutdown();
}

void gui::set_font(const char *font_name, u32 font_size)
{
	gui_manager.set_font(font_name, font_size);
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

bool gui::image_button(u32 width, u32 height, Texture2D *texture) 
{
	Gui_Window *window = gui_manager.get_window();
	Rect_s32 rect = { -1, -1, (s32)width, (s32)height };
	return gui_manager.image_button(&rect, texture, &window->view_rect);
}

void gui::edit_field(const char *name, int *value)
{
	gui_manager.edit_field(name, value);
}

void gui::edit_field(const char *name, float *value)
{
	gui_manager.edit_field(name, value);
}

void gui::edit_field(const char *name, String *string)
{
	gui_manager.edit_field(name, string);
}

bool gui::edit_field(const char *name, Vector3 *vector, const char *x, const char *y, const char *z)
{
	return gui_manager.edit_field(name, vector, x, y, z);
}

bool gui::begin_list(const char *name, Gui_List_Column filters[], u32 filter_count)
{
	return gui_manager.begin_list(name, filters, filter_count);
}

void gui::end_list()
{
	gui_manager.end_list();
}

bool gui::begin_line(Gui_List_Line_State *list_line)
{
	return gui_manager.begin_line(list_line);
}

void gui::end_line()
{
	gui_manager.end_line();
}

bool gui::selected(Gui_List_Line_State list_line_state)
{
	return list_line_state & GUI_LIST_LINE_SELECTED;
}

bool gui::left_mouse_click(Gui_List_Line_State list_line_state)
{
	return false;
}

bool gui::right_mouse_click(Gui_List_Line_State list_line_state)
{
	return false;
}

bool gui::begin_column(const char *filter_name)
{
	return gui_manager.begin_column(filter_name);
}

void gui::end_column()
{
	return gui_manager.end_column();
}

void gui::add_text(const char *text, Alignment alignment)
{
	gui_manager.add_text(text, alignment);
}

void gui::add_image(Texture2D *texture, Alignment alignment)
{
	gui_manager.add_image(texture, alignment);
}

void gui::add_image_button(Texture2D *texture, Alignment alignment)
{
	gui_manager.add_image_button(texture, alignment);
}

Size_s32 gui::get_window_size()
{
	Gui_Window *window = gui_manager.get_window();
	return window->rect.get_size();
}

Gui_ID gui::get_last_tab_gui_id()
{
	return gui_manager.get_last_tab_gui_id();
}

void gui::make_tab_active(Gui_ID tab_gui_id)
{
	return gui_manager.make_tab_active(tab_gui_id);
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
	return gui_manager.begin_window(name, window_style);
}
void gui::end_window()
{
	gui_manager.end_window();
}

bool gui::begin_child(const char *name, Window_Style window_style)
{
	return gui_manager.begin_child(name, window_style);
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
	gui_manager.backup_window_theme = gui_manager.window_theme;
	gui_manager.window_theme = *gui_window_theme;
}

void gui::set_theme(Gui_Text_Button_Theme *gui_button_theme)
{
	gui_manager.button_theme = *gui_button_theme;
}

void gui::set_theme(Gui_Edit_Field_Theme *gui_edit_field_theme)
{
	gui_manager.backup_edit_field_theme = gui_manager.edit_field_theme;
	gui_manager.edit_field_theme = *gui_edit_field_theme;
}

void gui::reset_window_theme()
{
	gui_manager.window_theme = gui_manager.backup_window_theme;
}

void gui::reset_button_theme()
{
	gui_manager.button_theme = DEFAULT_BUTTON_THEME;
}

void gui::reset_edit_field_theme()
{
	gui_manager.edit_field_theme = gui_manager.backup_edit_field_theme;
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
	if (gui_manager.window_handing_events) {
		return true;
	}
	for (u32 i = 0; i < gui_manager.windows_order.count; i++) {
		Gui_Window * window = gui_manager.get_window_by_index(&gui_manager.windows_order, i);
		if (detect_intersection(&window->rect)) {
			return true;
		}
	}
	return false;
}

void gui::draw_test_gui()
{
	begin_frame();
	if (begin_window("Test window")) {
		//begin_list("World Entities");
		//for (int i = 0; i < 10; i++) {
		//	item_list("Entity");
		//}
		//end_list();
		static bool state;
		radio_button("Turn on shadows", &state);
		static u32 index = 0;
		Array<String> array;
		array.push("String 1");
		array.push("String 2");
		array.push("String 3");
		list_box(&array, &index);
		static Vector3 position1 = Vector3::zero;
		static Vector3 position2 = Vector3::zero;
		static Vector3 position3 = Vector3::zero;
		//set_theme(&theme);
		//edit_field("Position", &position1);
		//reset_window_theme();
		static float value1;
		static float value2;
		static float value3;
		static float value4;
		static float value5;
		static float value6;
		edit_field("Value1", &value1);
		edit_field("Value2", &value2);
		edit_field("Value3", &value3);
		edit_field("Position", &position2);
		edit_field("Position", &position3);
		edit_field("Value4", &value4);
		edit_field("Value5", &value5);
		edit_field("Value6", &value6);

		static bool init = false;
		static Array<Gui_List_Line_State> list_line;
		static Array<String> file_names;
		static Array<String> file_types;
		static Array<String> file_sizes;
		if (!init) {
			init = true;
			list_line.reserve(20);
			for (int i = 0; i < 20; i++) {
				char *file_name = format("file_name_{}", i);
				file_names.push(file_name);
				char *file_type = format("hlsl_{}", i);
				file_types.push(file_type);
				file_sizes.push("12034A");
				free_string(file_name);
				free_string(file_type);
			}
			
		}
		
		static Gui_List_Column filters[] = { {"File Name", 50}, {"File Type", 25}, {"File Size", 25} };
		if (begin_list("File list", filters, 3)) {
			for (u32 i = 0; i < list_line.count; i++) {

				begin_line(&list_line[i]);
				
				begin_column("File Name");
				add_text(file_names[i].c_str(), RECT_LEFT_ALIGNMENT);
				end_column();

				begin_column("File Type");
				add_text(file_types[i].c_str(), RECT_LEFT_ALIGNMENT);
				end_column();

				begin_column("File Size");
				add_text(file_sizes[i].c_str(), RECT_LEFT_ALIGNMENT);
				end_column();
				
				end_line();
			}
			end_list();
		}

		//button("Button1");
		//button("Button2");
		//button("Button3");
		//button("Button4");
		//button("Button5");
		//button("Button6");
		//button("Button7");
		//button("Button8");
		//button("Button9");
		//button("Button10");
		//button("Button11");
		//button("Button12");
		//same_line();
		//button("Button13");
		//button("Button14");
		//button("Button15");
		//button("Button16");
		//button("Button17");

		if (begin_child("Temp child window")) {
			button("Button1");
			button("Button2");
			button("Button3");
			button("Button4");
			button("Button5");
			end_child();
		}
		button("Button2345");
		end_window();
	}

	//if (begin_window("Test window  2")) {
	//	//static Vector3 position = Vector3::zero;
	//	//set_theme(&theme);
	//	//edit_field("Position", &position);
	//	//reset_window_theme();
	//	static float value1;
	//	static float value2;
	//	static float value3;
	//	static float value4;
	//	static float value5;
	//	static float value6;
	//	edit_field("Value1", &value1);
	//	edit_field("Value2", &value2);
	//	edit_field("Value3", &value3);
	//	edit_field("Value4", &value4);
	//	edit_field("Value5", &value5);
	//	edit_field("Value6", &value6);

	//	u8 *image_data = NULL;
	//	u32 width = 0;
	//	u32 height = 0;
	//	String full_path;
	//	build_full_path_to_editor_file("search.png", full_path);
	//	if (load_png_file(full_path, &image_data, &width, &height)) {
	//		Texture2D texture;
	//		Texture2D_Desc texture_desc;
	//		texture_desc.mip_levels = 1;
	//		texture_desc.width = width;
	//		texture_desc.height = height;
	//		texture_desc.data = (void *)image_data;
	//		Engine::get_render_system()->gpu_device.create_texture_2d(&texture_desc, &texture);
	//		Engine::get_render_system()->gpu_device.create_shader_resource_view(&texture_desc, &texture);
	//		DELETE_PTR(image_data);
	//		image_button(width, height, &texture);
	//	} else {
	//	}

	//	end_window();
	//}

	//if (begin_window("Temp window 33")) {
	//	//same_line();
	//	button("sameline1");
	//	button("sameline2");
	//	button("sameline3");
	//	button("sameline4");
	//	//next_line();
	//	Array<String> temp;
	//	temp.push("AAAA");
	//	temp.push("AAAAB");
	//	temp.push("AAAAC");
	//	static u32 index = 0;
	//	list_box(&temp, &index);
	//	
	//	same_line();
	//	button("Sameline1");
	//	button("Sameline2");
	//	button("Sameline3");
	//	button("Sameline4");
	//	end_window();
	//}
	
	end_frame();
}
