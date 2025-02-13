#include <assert.h>
#include <stdlib.h>

#include "gui.h"
#include "../render/font.h"
#include "../render/renderer.h"
#include "../render/render_system.h"

#include "../sys/sys.h"
#include "../sys/utils.h"
#include "../sys/engine.h"
#include "../win32/win_time.h"

#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/os/input.h"
#include "../libs/os/event.h"
#include "../libs/str.h"
#include "../libs/key_binding.h"
#include "../libs/math/functions.h"
#include "../libs/math/structures.h"
#include "../libs/structures/tree.h"
#include "../libs/structures/stack.h"
#include "../libs/structures/array.h"

#ifdef _DEBUG
static s32 list_line_debug_counter = 0;
static s32 list_column_debug_counter = 0;
static s32 tree_debug_counter = 0;
static s32 tree_node_debug_counter = 0;
#endif

#define PRINT_GUI_INFO 0
#define DRAW_WINDOW_DEBUG_RECTS 0
#define DRAW_CHILD_WINDOW_DEBUG_RECTS 0
#define OUTLINE_ACTIVE_WINDOW 0

const Gui_Layout HORIZONTAL_LAYOUT_JUST_SET = 0x80;

const Gui_List_Line_State GUI_LIST_LINE_RESET_STATE = 0x0;
const Gui_List_Line_State GUI_LIST_LINE_SELECTED = 0x1;
const Gui_List_Line_State GUI_LIST_LINE_CLICKED_BY_LEFT_MOUSE = 0x2;
const Gui_List_Line_State GUI_LIST_LINE_CLICKED_BY_RIGHT_MOUSE = 0x4;
const Gui_List_Line_State GUI_LIST_LINE_CLICKED_BY_ENTER_KEY = 0x8;

const u32 LIST_HASH = fast_hash("list");
const u32 BUTTON_HASH = fast_hash("button");
const u32 IMAGE_BUTTON_HASH = fast_hash("image_button");
const u32 RADIO_BUTTON_HASH = fast_hash("radio_button");
const u32 LIST_BOX_HASH = fast_hash("list_box");
const u32 EDIT_FIELD_HASH = fast_hash("edit_Field");
const u32 SCROLL_BAR_HASH = fast_hash("scroll_bar");
const u32 TAB_HASH = fast_hash("window_tab_hash");
const u32 MENU_ITEM_HASH = fast_hash("menu_item_hash");

const u32 VECTOR3_EDIT_FIELD_NUMBER = 3;

#define GET_LIST_GUI_ID() (window->gui_id + LIST_HASH + list_count)
#define GET_BUTTON_GUI_ID() (window->gui_id + BUTTON_HASH + button_count)
#define GET_LIST_BOX_GUI_ID() (window->gui_id + LIST_BOX_HASH + list_box_count)
#define GET_EDIT_FIELD_GUI_ID() (window->gui_id + EDIT_FIELD_HASH + edit_field_count)
#define GET_RADIO_BUTTON_GUI_ID() (window->gui_id + RADIO_BUTTON_HASH + radio_button_count)
#define GET_IMAGE_BUTTON_GUI_ID() (window->gui_id + IMAGE_BUTTON_HASH + image_button_count)
#define GET_SCROLL_BAR_GUI_ID() (window->gui_id + SCROLL_BAR_HASH)
#define GET_TAB_GUI_ID() (window->gui_id + TAB_HASH + tab_count)
#define GET_MENU_ITEM_GUI_ID() (window->gui_id + MENU_ITEM_HASH + menu_item_count)

#define GET_RENDER_LIST() (&window->render_list)

const s32 MIN_WINDOW_WIDTH = 20;
const s32 MIN_WINDOW_HEIGHT = 60;
const u32 GUI_FLOAT_PRECISION = 2;

const s32 TAB_HEIGHT = 26;
const s32 TAB_BAR_HEIGHT = 30;

const Rect_s32 DEFAULT_WINDOW_RECT = { 50, 50, 400, 400 };
const Rect_s32 DEFAULT_CHILD_WINDOW_RECT = { 10, 10, 200, 200 };

const String MENU_WINDOW_PREFIX = "Menu Window ";

#define GET_DEFAULT_WINDOW_RECT(window_type) ((window_type == WINDOW_TYPE_PARENT) ? DEFAULT_WINDOW_RECT : DEFAULT_CHILD_WINDOW_RECT)

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
inline bool detect_intersection(const Triangle<T> &triangle, const Point3D<T> &point)
{
	T triangle_area = triangle.find_area();

	T area1 = Triangle<T>(triangle.a, triangle.b, point).find_area();
	T area2 = Triangle<T>(point, triangle.b, triangle.c).find_area();
	T area3 = Triangle<T>(triangle.a, point, triangle.c).find_area();

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
	Rect_s32 clip_rect = *item_rect;

	if ((item_rect->x >= win_rect->right()) || (item_rect->right() <= win_rect->x) || (item_rect->y >= win_rect->bottom()) || (item_rect->bottom() <= win_rect->y)) {
		return *win_rect;
	}

	if ((item_rect->x < win_rect->x) && (item_rect->right() > win_rect->x)) {
		clip_rect.offset_x(win_rect->x - item_rect->x);
	}

	if ((item_rect->y < win_rect->y) && (item_rect->bottom() > win_rect->y)) {
		clip_rect.offset_y(win_rect->y - item_rect->y);
	}

	if ((item_rect->y < win_rect->bottom()) && (item_rect->bottom() > win_rect->bottom())) {
		clip_rect.height -= item_rect->bottom() - win_rect->bottom();
	}

	if ((item_rect->x < win_rect->right()) && (item_rect->right() > win_rect->right())) {
		clip_rect.width -= item_rect->right() - win_rect->right();
	}

	return clip_rect;
}

enum Axis {
	X_AXIS = 0,
	Y_AXIS = 1,
	BOTH_AXIS = 2
};

inline void place_in_middle(Rect_s32 *rect, Rect_s32 *placing_rect, Axis axis)
{
	if ((axis == X_AXIS) || (axis == BOTH_AXIS)) {
		placing_rect->x = ((rect->width / 2) - (placing_rect->width / 2)) + rect->x;
	}

	if ((axis == Y_AXIS) || (axis == BOTH_AXIS)) {
		placing_rect->y = ((rect->height / 2) - (placing_rect->height / 2)) + rect->y;
	}
}

inline void place_in_middle_and_by_left(Rect_s32 *rect, Rect_s32 *placing_rect, int offset_from_left = 0)
{
	placing_rect->x = rect->x + offset_from_left;
	place_in_middle(rect, placing_rect, Y_AXIS);
}

inline void place_in_middle_and_by_right(Rect_s32 *rect, Rect_s32 *placing_rect, int offset_from_right = 0)
{
	placing_rect->x = rect->x + rect->width - placing_rect->width - offset_from_right;
	place_in_middle(rect, placing_rect, Y_AXIS);
}

inline void update_list_line_state(Gui_List_Line_State *list_line_state, Rect_s32 *line_rect, Gui_List_Line_State clicked_mouse_button)
{
	if (detect_intersection(line_rect)) {
		*list_line_state = GUI_LIST_LINE_RESET_STATE;
		*list_line_state |= GUI_LIST_LINE_SELECTED;
		*list_line_state |= clicked_mouse_button;
	} else {
		*list_line_state = GUI_LIST_LINE_RESET_STATE;
	}
}

enum Window_Type {
	WINDOW_TYPE_PARENT,
	WINDOW_TYPE_CHILD,
};

struct Placing_Info {
	Point_s32 scroll;
	Rect_s32 content_rect;
};

struct Gui_Window_Tab {
	bool draw;
	Gui_ID gui_id;
	Placing_Info placing_info;
};

struct Window_Context;

typedef u32 Gui_Tree_Node_State;

struct Gui_Tree_Node {
	String name;
	Gui_Tree_Node_State state;
};

static const Gui_Tree_Node_State GUI_TREE_NODE_NO_STATE = 0x0;
static const Gui_Tree_Node_State GUI_TREE_NODE_OPEN = 0x1;
static const Gui_Tree_Node_State GUI_TREE_NODE_SELECTED = 0x2;

struct Gui_Window {
	bool open = true;
	bool tab_was_added = false;
	bool tab_was_drawn = false;
	bool display_vertical_scrollbar = false;
	bool display_horizontal_scrollbar = false;
	Gui_ID gui_id = 0;
	u32 edit_field_count = 0;
	u32 max_edit_field_number = 0;
	s32 index_in_windows_array = -1;
	s32 index_in_windows_order = -1;
	s32 max_content_width = 0;

	u32 parent_window_idx = 0;

	Window_Context *context = NULL;
	Window_Type type;
	Window_Style style;

	String name;
	Point_s32 scroll;
	Point_s32 tab_position;
	Rect_s32 rect;
	Rect_s32 view_rect;
	Rect_s32 clip_rect;
	Rect_s32 content_rect;
	Rect_s32 tab_bar_clip_rect;

	Array<u32> child_windows;
	Array<Gui_Window_Tab> tabs;
	Array<Ordered_Tree<Gui_Tree_Node> *> trees;

	Render_Primitive_List render_list;

	void new_frame(Window_Style window_style);
	void set_position(s32 x, s32 y);
	void place_rect_over_window(Rect_s32 *new_rect);

	Rect_s32 get_scrollbar_rect(Axis axis);
	Gui_Window_Tab *find_tab(Gui_ID tab_id, u32 *index = NULL);
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

Gui_Window_Tab *Gui_Window::find_tab(Gui_ID tab_id, u32 *index)
{
	for (u32 i = 0; i < tabs.count; i++) {
		if (tabs[i].gui_id == tab_id) {
			if (index) {
				*index = i;
			}
			return &tabs[i];
		}
	}
	return NULL;
}

void Gui_Window::new_frame(Window_Style window_style)
{
	tab_was_drawn = false;
	style = window_style;

	view_rect = rect;
	clip_rect = rect;
	content_rect.set_size(0, 0);

	max_edit_field_number = math::max(max_edit_field_number, edit_field_count);
	edit_field_count = 0;
	max_content_width = 0;
}

inline void Gui_Window::set_position(s32 x, s32 y)
{
	s32 delta_x = x - rect.x;
	s32 delta_y = y - rect.y;

	rect.move(delta_x, delta_y);
	view_rect.move(delta_x, delta_y);
	clip_rect.move(delta_x, delta_y);
	content_rect.move(delta_x, delta_y);

	scroll.move(delta_x, delta_y);

	for (u32 i = 0; i < tabs.count; i++) {
		tabs[i].placing_info.scroll.move(delta_x, delta_y);
		tabs[i].placing_info.content_rect.move(delta_x, delta_y);
	}
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

enum Moving_Direction {
	MOVE_UP,
	MOVE_DOWN,
};

struct Column_Rendering_Data {
	List_Line_Info_Type type;
	union {
		const char *text = NULL;
		//Texture2D *texture;
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

struct Padding {
	s32 rects = 0;
	s32 horizontal = 0;
	s32 vertical = 0;
};

struct Context {
	Gui_Layout layout = 0;
	Padding padding;
	Rect_s32 clip_rect;
	Rect_s32 paint_rect;
	Rect_s32 last_placed_rect;

	virtual void setup(Gui_Layout layout_flags, Rect_s32 *_rect, Rect_s32 *_clip_rect, Padding *_padding);
	virtual void place_rect(Rect_s32 *placing_rect) = 0;
};

void Context::setup(Gui_Layout layout_flags, Rect_s32 *_rect, Rect_s32 *_clip_rect, Padding *_padding)
{
	assert(_rect && _clip_rect && _padding);
	layout = layout_flags;
	paint_rect = *_rect;
	clip_rect = *_clip_rect;
	padding = *_padding;
	last_placed_rect = { paint_rect.x, paint_rect.y, padding.horizontal - padding.rects, padding.vertical - padding.rects };
}

struct Rect_Context : Context {
	void place_rect(Rect_s32 *rect);
};

void Rect_Context::place_rect(Rect_s32 *placing_rect)
{
	if ((layout & LAYOUT_VERTICALLY) || (layout & HORIZONTAL_LAYOUT_JUST_SET)) {
		if (layout & HORIZONTAL_LAYOUT_JUST_SET) {
			layout &= ~HORIZONTAL_LAYOUT_JUST_SET;
		}
		placing_rect->x = paint_rect.x + padding.horizontal;
		placing_rect->y = last_placed_rect.bottom() + padding.rects;

		last_placed_rect = *placing_rect;

	} else if (layout & LAYOUT_HORIZONTALLY) {
		placing_rect->x = last_placed_rect.right() + padding.rects;
		placing_rect->y = last_placed_rect.y;

		last_placed_rect = *placing_rect;
	} else if (layout & LAYOUT_HORIZONTALLY_CENTER) {
		placing_rect->x = last_placed_rect.right() + padding.rects;
		place_in_middle(&clip_rect, placing_rect, Y_AXIS);

		last_placed_rect = *placing_rect;
	}
}

struct Tree_State {
	Context *window_context = NULL;
	Rect_Context context;
	s32 tree_level_depth = -1;
	Ordered_Tree<Gui_Tree_Node>::Node *parent_node = NULL;
	Ordered_Tree<Gui_Tree_Node> *current_tree = NULL;
	Array<u32> level_node_count_stack;
};

struct Window_Placing_State {
	s32 max_content_width = 0;
	Rect_s32 content_rect;
	Rect_s32 last_placed_rect;
};

struct Window_Context : Rect_Context {
	Gui_Window *window = NULL;

	void setup(Gui_Layout _layout_flags, Gui_Window *window, Padding *_padding);
	void place_rect(Rect_s32 *placing_rect);
	void set_placing_state(Window_Placing_State *placing_state);
	Window_Placing_State get_placing_state();
};

void Window_Context::setup(Gui_Layout _layout_flags, Gui_Window *_window, Padding *_padding)
{
	assert(_window && _padding);
	window = _window;
	Context::setup(_layout_flags, &window->content_rect, &window->clip_rect, _padding);
}

void Window_Context::place_rect(Rect_s32 *placing_rect)
{
	Gui_Layout prev_layout = layout;
	Rect_Context::place_rect(placing_rect);
	if ((prev_layout & LAYOUT_VERTICALLY) || (prev_layout & HORIZONTAL_LAYOUT_JUST_SET)) {
		if (prev_layout & HORIZONTAL_LAYOUT_JUST_SET) {
			window->max_content_width = placing_rect->width + padding.horizontal;
			window->content_rect.width = math::max(window->max_content_width, window->content_rect.width);
		}
		window->content_rect.height += placing_rect->height + padding.rects;
		window->content_rect.width = math::max(placing_rect->width + padding.horizontal, window->content_rect.width);

	} else if (prev_layout & LAYOUT_HORIZONTALLY) {
		window->max_content_width += placing_rect->width + padding.rects;
		window->content_rect.width = math::max(window->max_content_width, window->content_rect.width);
	}
}

void Window_Context::set_placing_state(Window_Placing_State *placing_state)
{
	last_placed_rect = placing_state->last_placed_rect;
	window->content_rect = placing_state->content_rect;
	window->max_content_width = placing_state->max_content_width;
}

Window_Placing_State Window_Context::get_placing_state()
{
	Window_Placing_State placing_state;
	placing_state.last_placed_rect = last_placed_rect;
	placing_state.content_rect = window->content_rect;
	placing_state.max_content_width = window->max_content_width;

	return placing_state;
}

struct Gui_Manager {
	bool active_scrolling = false;
	bool window_handing_events = false;
	bool change_active_field = false;
	bool next_list_active = false;
	bool next_ui_element_active = false;

	s32 mouse_x;
	s32 mouse_y;
	s32 last_mouse_x;
	s32 last_mouse_y;
	s32 mouse_x_delta;
	s32 mouse_y_delta;
	s32 placing_rect_height;
	Rect_s32 *placed_rect = NULL;

	u32 tab_count = 0;
	u32 list_count = 0;
	u32 button_count = 0;
	u32 menu_item_count = 0;
	u32 image_button_count = 0;
	u32 radio_button_count = 0;
	u32 list_box_count = 0;
	u32 edit_field_count = 0;

	u32 reset_window_params = 0;

	Gui_ID hot_item = 0;
	Gui_ID active_list = 0;
	Gui_ID active_item = 0;
	Gui_ID resizing_window = 0;
	Gui_ID active_list_box = 0;
	Gui_ID active_edit_field = 0;
	Gui_ID active_tab = 0;
	Gui_ID active_window = 0;
	Gui_ID became_just_actived = 0; //window
	Gui_ID probably_resizing_window = 0;
	Gui_ID hover_window = 0;

	Gui_Layout current_layout = LAYOUT_LEFT | LAYOUT_VERTICALLY;

	const char *current_list_name = NULL;

	Font *font = NULL;
	Render_Font *render_font = NULL;
	Render_2D *render_2d = NULL;

	//Texture2D up_texture;
	//Texture2D down_texture;
	//Texture2D check_texture;
	//Texture2D default_texture;
	//Texture2D expand_down_texture;

	Rect_s32 window_rect;
	Array<Context *> context_stack;

	} pre_setup;
		Array<Pair<String, bool>> windows_params;
		Rect_s32 window_rect;
	struct Pre_Setup {
	Rect_s32 last_drawn_rect;
	Cursor_Type cursor_type;

	Gui_List_Column *picked_column = NULL;
	Gui_Line_Column current_list_column;
	Gui_List_Line current_list_line;
	Array<Gui_List_Line> line_list;
	Array<Rect_s32> list_column_rect_list;

	Stack<u32> window_stack;
	Array<u32> windows_order;
	Array<Gui_Window> windows;
	Array<Pair<String, Rect_s32>> saved_windows_rects;

	Gui_Tree_Theme tree_theme;
	Array<Gui_Tree_Theme> backup_tree_themes;

	Gui_Menu_Theme menu_theme;
	Array<Gui_Menu_Theme> backup_menu_themes;

	Gui_Tab_Theme tab_theme;
	Gui_Tab_Theme backup_tab_theme;
	Gui_List_Theme list_theme;
	Gui_List_Theme backup_list_theme;
	Gui_Tree_List_Theme tree_list_theme;
	Gui_Tree_List_Theme backup_tree_list_theme;
	Gui_List_Box_Theme list_box_theme;
	Gui_Window_Theme window_theme;
	Array<Gui_Window_Theme> backup_window_themes;
	Gui_Window_Theme future_window_theme;
	Gui_Text_Button_Theme button_theme;
	Gui_Image_Button_Theme image_button_theme;
	Gui_Radio_Button_Theme radio_button_theme;
	Gui_Edit_Field_Theme edit_field_theme;
	Gui_Edit_Field_Theme backup_edit_field_theme;

	Tree_State tree_state;
	Edit_Field_State edit_field_state;

	Key_Bindings key_bindings;

	void init(Engine *engine);
	void init_from_save_file();
	void load_and_init_textures(Gpu_Device *gpu_device);
	void handle_events();
	void handle_events(bool *update_editing_value, bool *update_next_time_editing_value, Rect_s32 *rect, Rect_s32 *editing_value_rect);
	void set_font(const char *font_name, u32 font_size);
	void shutdown();

	void new_frame();
	void end_frame();

	void setup_active_window(Gui_Window *window);
	void update_active_and_hot_state(Gui_ID gui_id, Rect_s32 *rect);
	void update_active_and_hot_state(Gui_Window *window, u32 rect_gui_id, Rect_s32 *rect);

	void remove_window_from_rendering_order(Gui_Window *window);
	void move_window_on_rendering_order_top(Gui_Window *window);

	bool begin_window(const char *name, Window_Style window_style, bool window_open = true);
	void end_window();
	void render_window(Gui_Window *window);
	void open_window(const char *name);
	void close_window(const char *name);

	bool begin_child(const char *name, Window_Style window_style);
	void end_child();

	void same_line();
	void next_line();
	void set_next_window_pos(s32 x, s32 y);
	void set_next_window_size(s32 width, s32 height);
	void set_next_window_theme(Gui_Window_Theme *theme);

	void place_rect_on_window_top(s32 height, Rect_s32 *placed_rect);
	void set_caret_position_on_mouse_click(Rect_s32 *rect, Rect_s32 *editing_value_rect);

	void make_tab_active(Gui_ID tab_gui_id);
	void make_next_list_active();

	void scrolling(Gui_Window *window, Axis axis);
	void move_window_content(Gui_Window *window, u32 distance, Moving_Direction moving_direction);

	void text(const char *some_text);

	//void image(Texture2D *texture, s32 width, s32 height);
	void list_box(Array<String> *array, u32 *item_index);
	void edit_field(const char *name, int *value);
	void edit_field(const char *name, float *value);
	void edit_field(const char *name, String *string);
	bool add_tab(const char *tab_name);
	bool button(const char *name, bool *state = NULL);
	void core_button(const char *name, bool *left_mouse_click, bool *right_mouse_click);
	bool radio_button(const char *name, bool *state);
	//bool image_button(Rect_s32 *rect, Texture2D *texture, Rect_s32 *window_view_rect);
	bool image_button(Image *image);
	bool base_image_button(Rect_s32 *button_rect, Image *image);

	bool edit_field(const char *name, Vector3 *vector, const char *x, const char *y, const char *z);
	bool edit_field(const char *name, const char *editing_value, u32 max_chars_number, bool(*symbol_validation)(char symbol));
	bool edit_field(const char *name, const char *editing_value, u32 max_chars_number, bool(*symbol_validation)(char symbol), const Color &color, u32 index);

	bool begin_menu(const char *name);
	void end_menu();
	bool menu_item(Image *image, const char *text, const char *shortcut = NULL, bool submenu = false);
	void segment();

	bool begin_tree(const char *name);
	void end_tree();

	bool begin_tree_node(const char *name, Gui_Tree_Style node_flags);
	void end_tree_node();

	bool begin_list(const char *name, Gui_List_Column columns[], u32 column_count);
	void end_list();

	bool begin_line(Gui_List_Line_State *list_line);
	void end_line();

	bool begin_column(const char *column_name);
	void end_column();

	void add_text(const char *text, Alignment alignment);
	//void add_image(Texture2D *texture, Alignment alignment);
	//void add_image_button(Texture2D *texture, Alignment alignment);

	bool element_clicked(Key key);
	bool window_active(Gui_Window *window);
	bool handle_edit_field_shortcut_event(Gui_Window *window, Rect_s32 *edit_field_rect, Gui_ID edit_field_gui_id, Edit_Field_Type edit_field_type, u32 vector3_edit_field_index = 0);
	bool update_edit_field(Edit_Field_Instance *edit_field_instance);
	bool detect_window_borders_intersection(Rect_s32 *rect, Rect_Side *rect_side);

	Gui_ID get_last_tab_gui_id();

	Size_s32 get_window_size();
	Size_s32 get_window_size_with_padding();
	Rect_s32 get_win32_rect();
	Rect_s32 get_text_rect(const char *text);
	Rect_s32 get_parent_window_clip_rect(Gui_Window *window);

	Gui_Window *get_window();
	Gui_Window *get_parent_window(Gui_Window *window);
	Gui_Window *find_window(const char *name, u32 *window_index = NULL);
	Gui_Window *find_window_in_order(const char *name, int *window_index);

	Gui_Window *create_window(const char *name, Window_Type window_type, Window_Style window_style, bool window_open);

	Gui_Window *get_window_by_index(Array<u32> *window_indices, u32 index);

	Context *get_context();
};

Rect_s32 Gui_Manager::get_text_rect(const char *text)
{
	Size_u32 size = font->get_text_size(text);
	return Rect_s32(0, 0, (s32)size.width, (s32)size.height);
}

Rect_s32 Gui_Manager::get_parent_window_clip_rect(Gui_Window *window)
{
	assert(window);
	return get_parent_window(window)->clip_rect;
}

void Gui_Manager::set_next_window_pos(s32 x, s32 y)
{
	pre_setup.window_rect.set(x, y);
	reset_window_params |= SET_WINDOW_POSITION;
}

void Gui_Manager::set_next_window_size(s32 width, s32 height)
{
	pre_setup.window_rect.set_size(width, height);
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

Size_s32 Gui_Manager::get_window_size()
{
	Gui_Window *window = get_window();
	return window->view_rect.get_size();
}

Size_s32 Gui_Manager::get_window_size_with_padding()
{
	Gui_Window *window = get_window();
	s32 horizontal_padding = window->context->padding.horizontal * 2;
	s32 vertical_padding = window->context->padding.vertical * 2;
	Size_s32 window_size = get_window_size();
	s32 width = window_size.width > horizontal_padding ? window_size.width - horizontal_padding : 0;
	s32 height = window_size.height > vertical_padding ? window_size.height - vertical_padding : 0;
	return { width, height };
}

Rect_s32 Gui_Manager::get_win32_rect()
{
	//return Rect_s32(0, 0, (s32)Render_System::screen_width, (s32)Render_System::screen_height);
	assert(false);
	return Rect_s32(0, 0, 0, 0);
}

Gui_Window *Gui_Manager::get_window()
{
	if (window_stack.is_empty()) {
		error("Hades gui error: Window stask is empty");
	}
	return &windows[window_stack.top()];
}

Gui_Window *Gui_Manager::get_parent_window(Gui_Window *window)
{
	assert(window);
	assert(window->type == WINDOW_TYPE_CHILD);
	assert(window->parent_window_idx < windows.count);

	return &windows[window->parent_window_idx];
}

Gui_Window *Gui_Manager::find_window(const char *name, u32 *window_index)
{
	Gui_ID window_id = fast_hash(name);

	for (u32 i = 0; i < windows.count; i++) {
		if (name == windows[i].name) {
			if (window_index) {
				*window_index = i;
			}
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

static bool find_saved_window_rect(const Pair<String, Rect_s32> &name_rect_pair, const String &name)
{
	return name_rect_pair.first == name;
}

static bool find_window_pre_setup(const Pair<String, bool> &window_pre_setup, const String &name)
{
	return window_pre_setup.first == name;
}

Gui_Window *Gui_Manager::create_window(const char *name, Window_Type window_type, Window_Style window_style, bool window_open)
{
	Find_Result<Pair<String, bool>> result1 = find_in_array(pre_setup.windows_params, name, find_window_pre_setup);
	if (result1.found) {
		window_open = result1.data.second;
	}

	Gui_Window window;
	window.open = window_open;
	window.name = name;
	window.gui_id = fast_hash(name);
	window.type = window_type;

	s32 offset = 0;
	offset += window_style & WINDOW_HEADER ? window_theme.header_height : 0;
	offset += window_style & WINDOW_TAB_BAR ? tab_theme.tab_bar_height : 0;

	Rect_s32 rect = GET_DEFAULT_WINDOW_RECT(window_type);
	Find_Result<Pair<String, Rect_s32>> result2 = find_in_array(saved_windows_rects, name, find_saved_window_rect);
	if (result2.found) {
		rect = result2.data.second;
	}

	window.rect = rect;
	window.view_rect = rect;
	window.content_rect = { rect.x, rect.y + offset, 0, 0 };
	window.scroll = Point_s32(rect.x, rect.y + offset);

	window.style = window_style;
	window.context = new Window_Context();

	window.render_list = Render_Primitive_List(render_2d, font, render_font);
#ifdef _DEBUG
	window.name = name;
#endif
	window.index_in_windows_array = windows.count;
	window.index_in_windows_order = windows_order.count;

	windows.push(window);
	if (window_open && (window_type == WINDOW_TYPE_PARENT)) {
		windows_order.push(window.index_in_windows_array);
	}
	return &windows.last();
}

inline Gui_Window *Gui_Manager::get_window_by_index(Array<u32> *window_indices, u32 index)
{
	u32 window_index = window_indices->get(index);
	return &windows[window_index];
}

Context *Gui_Manager::get_context()
{
	assert(!context_stack.is_empty());
	return context_stack.last();;
}

void Gui_Manager::setup_active_window(Gui_Window *window)
{
	if (active_window != window->gui_id) {
		active_window = window->gui_id;
		became_just_actived = window->gui_id;
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
	Gui_ID window_gui_id = window->gui_id;
	//if (window->type == WINDOW_TYPE_CHILD) {
	//	window_gui_id = get_parent_window(window)->gui_id;
	//}
	if (hover_window == window_gui_id) {
		update_active_and_hot_state(rect_gui_id, rect);
	} else {
		if (window->type == WINDOW_TYPE_CHILD) {
			window_gui_id = get_parent_window(window)->gui_id;
		}
		if (hover_window == window_gui_id) {
			update_active_and_hot_state(rect_gui_id, rect);
		}
	}
}

void Gui_Manager::remove_window_from_rendering_order(Gui_Window *window)
{
	if (window->index_in_windows_order > -1) {
		windows_order.remove(window->index_in_windows_order);
		for (u32 i = 0; i < windows_order.count; i++) {
			Gui_Window *window = get_window_by_index(&windows_order, i);
			window->index_in_windows_order = i;
		}
		window->index_in_windows_order = -1;
	}
}

void Gui_Manager::move_window_on_rendering_order_top(Gui_Window *window)
{
	assert(window->index_in_windows_array > -1);
	if (windows_order.is_empty() && (window->index_in_windows_order > -1) && ((windows_order.count - 1) != window->index_in_windows_order)) {
		windows_order.remove(window->index_in_windows_order);
		for (u32 i = 0; i < windows_order.count; i++) {
			Gui_Window *window = get_window_by_index(&windows_order, i);
			window->index_in_windows_order = i;
		}
	}
	window->index_in_windows_order = windows_order.count;
	windows_order.push(window->index_in_windows_array);
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

	Context *context = get_context();
	Gui_Window *window = get_window();
	context->place_rect(&rect);

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
			//render_list->add_texture(&check_texture_rect, &check_texture);
		} else {
			render_list->add_rect(&radio_rect, radio_button_theme.default_background_color, radio_button_theme.rounded_border);
		}
		render_list->add_text(&name_rect, name);
		render_list->pop_clip_rect();
	}
	radio_button_count++;
	return was_click_by_mouse_key;
}

//bool Gui_Manager::image_button(Rect_s32 *rect, Texture2D *texture, Rect_s32 *window_view_rect)
//{
//	assert(rect);
//	assert(texture);
//	assert((rect->width > 0) && (rect->height > 0));
//
//	Gui_Window *window = get_window();
//	Rect_s32 image_rect = *rect;
//
//	if ((image_rect.x < 0) && (image_rect.y < 0)) {
//		place_rect_in_window(window, &image_rect, window->place_between_rects, window->horizontal_offset_from_sides, window->vertical_offset_from_sides);
//	}
//	Gui_ID image_button_gui_id = GET_IMAGE_BUTTON_GUI_ID();
//	if (must_rect_be_drawn(&window->clip_rect, &image_rect)) {
//		update_active_and_hot_state(image_button_gui_id, &image_rect);
//
//		Render_Primitive_List *render_list = GET_RENDER_LIST();
//		render_list->push_clip_rect(&window->clip_rect);
//		render_list->add_texture(&image_rect, texture);
//		render_list->pop_clip_rect();
//	}
//	image_button_count++;
//	return ((hot_item == image_button_gui_id) && was_click(KEY_LMOUSE));
//}
bool Gui_Manager::image_button(Image *image)
{
	assert(image);
	assert((image->width > 0) && (image->height > 0));

	Size_s32 button_size = image_button_theme.button_size;
	if ((button_size.width < 0) || (button_size.height < 0)) {
		button_size = image_button_theme.image_size;
	}
	Rect_s32 button_rect = button_size;
	Context *context = get_context();
	context->place_rect(&button_rect);
	return base_image_button(&button_rect, image);
}

bool Gui_Manager::base_image_button(Rect_s32 *button_rect, Image *image)
{
	Size_s32 image_size = image_button_theme.image_size;
	if ((image_size.width < 0) || (image_size.height < 0)) {
		image_size = Size_s32(image->width, image->height);
	}
	image_size.width = math::min(image_size.width, button_rect->width);
	image_size.height = math::min(image_size.height, button_rect->height);
	Rect_s32 image_rect = image_size;

	Gui_Window *window = get_window();
	Gui_ID image_button_gui_id = GET_IMAGE_BUTTON_GUI_ID();
	if (must_rect_be_drawn(&window->clip_rect, button_rect)) {
		update_active_and_hot_state(image_button_gui_id, button_rect);

		if (image_button_theme.image_alignment == RECT_RIGHT_ALIGNMENT) {
			place_in_middle_and_by_right(button_rect, &image_rect, 0);
		} else if (image_button_theme.image_alignment == RECT_LEFT_ALIGNMENT) {
			place_in_middle_and_by_left(button_rect, &image_rect, 0);
		} else if (image_button_theme.image_alignment == RECT_CENTER_ALIGNMENT) {
			place_in_middle(button_rect, &image_rect, BOTH_AXIS);
		}
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&window->clip_rect);
		Color button_color = (hot_item == image_button_gui_id) ? image_button_theme.hover_color : image_button_theme.color;
		render_list->add_rect(button_rect, button_color, image_button_theme.rounded_border, image_button_theme.rect_rounding);
		//render_list->add_texture(&image_rect, &image->texture);
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
		edit_field_instance.value_rect = { 0, 0, 0, (s32)font->max_alphabet_height };
	} else {
		edit_field_instance.editing_value = string->c_str();
		edit_field_instance.value_rect = get_text_rect(string->c_str());
		edit_field_instance.value_rect.height = (s32)font->max_alphabet_height;
	}

	edit_field_instance.name = name;
	edit_field_instance.symbol_validation = is_symbol_string_valid;
	edit_field_instance.max_chars_number = 50;
	edit_field_instance.caret_rect = { 0, 0, 2, (s32)font->max_alphabet_height + 4 };
	edit_field_instance.rect = theme->rect;
	edit_field_instance.edit_field_rect = theme->rect;
	edit_field_instance.name_rect = get_text_rect(name);

	if (theme->draw_label) {
		edit_field_instance.rect.width = edit_field_instance.name_rect.width + edit_field_instance.edit_field_rect.width + theme->text_shift;
	}
	Context *context = get_context();
	context->place_rect(&edit_field_instance.rect);

	bool update_value = false;
	Gui_Window *window = get_window();
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
				render_list->add_text(&edit_field_instance.value_rect, edit_field_state.data, ALIGN_TEXT_BY_MAX_ALPHABET);
			}
		} else {
			int len = (int)strlen(edit_field_instance.editing_value);
			if (edit_field_instance.editing_value && (len > 0)) {
				render_list->add_text(&edit_field_instance.value_rect, edit_field_instance.editing_value, ALIGN_TEXT_BY_MAX_ALPHABET);
			}
		}
		render_list->pop_clip_rect();
	}
	edit_field_count++;
	window->edit_field_count++;

	if (update_value) {
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

	Context *context = get_context();
	context->place_rect(&edit_field_instance.rect);

	bool update_value = false;
	Gui_Window *window = get_window();
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

	Context *context = get_context();
	context->place_rect(&edit_field_instance.rect);

	bool update_value = false;
	Gui_Window *window = get_window();
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

bool Gui_Manager::begin_menu(const char *name)
{
	static bool close_window_in_next_frame = false;
	static Gui_ID window_id = 0;

	String menu_window_name = MENU_WINDOW_PREFIX + name;

	if (close_window_in_next_frame) {
	}

	Gui_Window_Theme menu_window_theme;
	menu_window_theme.background_color = menu_theme.background_color;
	menu_window_theme.rects_padding = menu_theme.rects_padding;
	menu_window_theme.horizontal_padding = menu_theme.horizontal_padding;
	menu_window_theme.vertical_padding = menu_theme.vertical_padding;

	gui::set_theme(&menu_window_theme);
	gui::set_next_window_size(menu_theme.menu_window_width, -1);
	if (begin_window(menu_window_name, WINDOW_AUTO_HEIGHT, false)) {
		Gui_Window *window = get_window();
		if (window->gui_id == became_just_actived) {
			window_id = 0;
		} else if ((hot_item == window->gui_id) && (window_id != window->gui_id)) {
			window_id = window->gui_id;
		} else if ((hot_item != window->gui_id) && (window_id == window->gui_id)) {
			close_window_in_next_frame = false;
			window_id = 0;
			close_window(menu_window_name);
		}
		return true;
	}
	gui::reset_window_theme();
	return false;
}

bool Gui_Manager::menu_item(Image *image, const char *text, const char *shortcut, bool submenu)
{
	Context *context = get_context();
	Gui_Window *window = get_window();

	Rect_s32 menu_item_rect = { 0, 0, get_window_size_with_padding().width, menu_theme.menu_item_height };
	context->place_rect(&menu_item_rect);

	Gui_ID menu_item_gui_id = 0;
	if (must_rect_be_drawn(&window->clip_rect, &menu_item_rect)) {
		menu_item_gui_id = GET_MENU_ITEM_GUI_ID();
		update_active_and_hot_state(window, menu_item_gui_id, &menu_item_rect);

		if ((became_just_actived != window->gui_id) && was_click(KEY_LMOUSE)) {
			window->open = false;
			remove_window_from_rendering_order(window);
		}
		Color menu_item_color = (hot_item == menu_item_gui_id) ? menu_theme.item_hover_color : menu_theme.background_color;

		Rect_s32 image_rect = { 0, 0, 0, 0 };
		s32 image_padding = 0;
		if (image) {
			image_rect.set(menu_theme.image_size);
			place_in_middle_and_by_left(&menu_item_rect, &image_rect, menu_theme.image_padding);
		}
		if (image || menu_theme.layout_by_image) {
			image_padding = menu_theme.image_padding + menu_theme.image_size.width;
		}

		Rect_s32 text_rect = get_text_rect(text);
		place_in_middle_and_by_left(&menu_item_rect, &text_rect, menu_theme.text_padding + image_padding);

		Render_Primitive_List *render_list = GET_RENDER_LIST();
		Rect_s32 clip_rect = calculate_clip_rect(&window->clip_rect, &menu_item_rect);
		render_list->push_clip_rect(&clip_rect);
		render_list->add_rect(&menu_item_rect, menu_item_color, 5);
		if (image) {
			//render_list->add_texture(&image_rect, &image->texture);
		}
		render_list->add_text(&text_rect, text);
		render_list->pop_clip_rect();

	}
	menu_item_count++;
	return (hot_item == menu_item_gui_id) && was_click(KEY_LMOUSE);
}

void Gui_Manager::segment()
{
	Context *context = get_context();
	Gui_Window *window = get_window();

	Rect_s32 space_rect = { 0, 0, get_window_size_with_padding().width, menu_theme.vertical_segment_padding + (s32)menu_theme.segment_line_thickness };
	context->place_rect(&space_rect);
	if (must_rect_be_drawn(&window->clip_rect, &space_rect)) {
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		Rect_s32 clip_rect = calculate_clip_rect(&window->clip_rect, &space_rect);
		render_list->push_clip_rect(&clip_rect);
		Point_s32 p1 = Point_s32(window->view_rect.x + menu_theme.horizontal_segment_padding, space_rect.y + (menu_theme.vertical_segment_padding / 2));
		Point_s32 p2 = Point_s32(window->view_rect.right() - menu_theme.horizontal_segment_padding, space_rect.y + (menu_theme.vertical_segment_padding / 2));
		render_list->add_line(p1, p2, Color(45), menu_theme.segment_line_thickness);
		render_list->pop_clip_rect();
	}
}

void Gui_Manager::end_menu()
{
	end_window();
	gui::reset_window_theme();
}

bool Gui_Manager::begin_tree(const char *name)
{
#ifdef _DEBUG
	if (tree_debug_counter != 0) {
		ASSERT_MSG(tree_debug_counter > 0, "You forgot to use 'gui::begin_tree'.");
		ASSERT_MSG(tree_debug_counter < 0, "You forgot to use 'gui::end_tree'.");
	}
	if (tree_node_debug_counter != 0) {
		ASSERT_MSG(tree_node_debug_counter > 0, "You forgot to use 'gui::begin_node_tree'.");
		ASSERT_MSG(tree_node_debug_counter < 0, "You forgot to use 'gui::end_node_tree'.");
	}
#endif
	Gui_Window_Theme tree_window_theme;
	tree_window_theme.background_color = tree_theme.background_color;
	tree_window_theme.rects_padding = 0;
	tree_window_theme.horizontal_padding = 0;
	tree_window_theme.vertical_padding = 0;
	tree_window_theme.rounded_border = 0;
	tree_window_theme.rounded_scrolling = 0;

	const auto compare_function = [](Ordered_Tree<Gui_Tree_Node> *tree, const String &string) {
		return tree->root_node ? tree->root_node->data.name == string : false;
	};

	gui::set_theme(&tree_window_theme);
	set_next_window_size(tree_theme.window_size.width, tree_theme.window_size.height);
	if (begin_child(name, WINDOW_SCROLL_BAR)) {
		Gui_Window *window = get_parent_window(get_window());
		Find_Result<Ordered_Tree<Gui_Tree_Node> *> find_result = find_in_array(window->trees, name, compare_function);
		Ordered_Tree<Gui_Tree_Node> *tree = find_result.data;
		if (!find_result.found) {
			tree = new Ordered_Tree<Gui_Tree_Node>();
			tree->create_root_node({ name, GUI_TREE_NODE_OPEN });
			window->trees.push(tree);
		}
		tree_state.window_context = get_context();
		tree_state.current_tree = tree;
		tree_state.parent_node = tree->root_node;
		tree_state.level_node_count_stack.push(0);
		tree_state.tree_level_depth = -1;
#ifdef _DEBUG
		tree_debug_counter++;
#endif
		return true;
	}
	gui::reset_window_theme();
	return false;
}

void Gui_Manager::end_tree()
{
#ifdef _DEBUG
	tree_debug_counter--;
	if (tree_debug_counter != 0) {
		ASSERT_MSG(tree_debug_counter > 0, "You forgot to use 'gui::begin_tree'.");
		ASSERT_MSG(tree_debug_counter < 0, "You forgot to use 'gui::end_tree'.");
	}
	if (tree_node_debug_counter != 0) {
		ASSERT_MSG(tree_node_debug_counter > 0, "You forgot to use 'gui::begin_node_tree'.");
		ASSERT_MSG(tree_node_debug_counter < 0, "You forgot to use 'gui::end_node_tree'.");
	}
#endif
	tree_state.window_context = NULL;
	tree_state.current_tree = NULL;
	tree_state.parent_node = NULL;
	tree_state.level_node_count_stack.clear();
	tree_state.tree_level_depth = -1;

	end_child();
	gui::reset_window_theme();
}

static void update_node_state(Gui_Tree_Node *node_info, void *args)
{
	node_info->state &= ~GUI_TREE_NODE_OPEN;
	node_info->state &= ~GUI_TREE_NODE_SELECTED;
}

bool Gui_Manager::begin_tree_node(const char *name, Gui_Tree_Style node_flags)
{
	if (!(tree_state.parent_node->data.state & GUI_TREE_NODE_OPEN)) {
		return false;
	}
	tree_state.tree_level_depth += 1;
	u32 level_node_counter = tree_state.level_node_count_stack.last();
	Ordered_Tree<Gui_Tree_Node>::Node *current_node = tree_state.current_tree->get_child_node(tree_state.parent_node, level_node_counter);
	if (!current_node) {
		current_node = tree_state.current_tree->create_child_node(tree_state.parent_node, { name , GUI_TREE_NODE_NO_STATE }, level_node_counter);
	}
	Context *context = tree_state.window_context;
	Gui_Window *window = get_window();
	Gui_Tree_Theme *theme = &tree_theme;

	Rect_s32 tree_node_rect = Rect_s32(0, 0, window->view_rect.width, theme->tree_node_height);
	context->place_rect(&tree_node_rect);
	bool mouse_over = detect_intersection(&tree_node_rect);

	Gui_Tree_Node_State node_state = current_node->data.state;
	//if ((node_state & GUI_TREE_NODE_SELECTED) && !Keys_State::is_key_down(KEY_CTRL) && was_click(KEY_LMOUSE)) {
	//	node_state &= ~GUI_TREE_NODE_SELECTED;
	//}
	if (active_window == window->gui_id) {
		if ((node_state & GUI_TREE_NODE_SELECTED) && !Keys_State::is_key_down(KEY_CTRL) && was_click(KEY_LMOUSE)) {
			node_state &= ~GUI_TREE_NODE_SELECTED;
		}
		if (mouse_over && was_click(KEY_LMOUSE)) {
			if (node_state & GUI_TREE_NODE_SELECTED) {
				node_state &= ~GUI_TREE_NODE_SELECTED;
			} else {
				node_state |= GUI_TREE_NODE_SELECTED;
			}
			if (node_state & GUI_TREE_NODE_OPEN) {
				node_state &= ~GUI_TREE_NODE_OPEN;
				tree_state.current_tree->walk_and_update_nodes(current_node, update_node_state, NULL);
			} else {
				node_state |= GUI_TREE_NODE_OPEN;
			}
		}
	}
	s32 node_offset = tree_state.tree_level_depth * theme->tree_node_depth_offset;
	s32 text_offset = (node_flags & GUI_TREE_NODE_FINAL) ? 5 : 20;
	if (must_rect_be_drawn(&window->clip_rect, &tree_node_rect)) {
		Rect_s32 down_image_rect = { 0, 0, 15, 15 };
		Rect_s32 node_text_rect = get_text_rect(name);
		place_in_middle_and_by_left(&tree_node_rect, &down_image_rect, node_offset);
		place_in_middle_and_by_left(&tree_node_rect, &node_text_rect, text_offset + node_offset);

		Render_Primitive_List *render_list = GET_RENDER_LIST();
		Rect_s32 clip_rect = calculate_clip_rect(&window->clip_rect, &tree_node_rect);
		render_list->push_clip_rect(&clip_rect);

		Color tree_node_color = (node_state & GUI_TREE_NODE_SELECTED) ? theme->picked_tree_node_color : mouse_over && (active_window == window->gui_id) ? theme->hover_tree_node_color : theme->tree_node_color;
		render_list->add_rect(&tree_node_rect, tree_node_color, 0);
		if (!(node_flags & GUI_TREE_NODE_FINAL)) {
			//Texture2D *expand = (node_state & GUI_TREE_NODE_OPEN) ? &expand_down_texture : &expand_right_texture;
			//render_list->add_texture(&down_image_rect, expand);
		}
		if (!(node_flags & GUI_TREE_NODE_NOT_DISPLAY_NAME)) {
			render_list->add_text(&node_text_rect, name);
		}
		render_list->pop_clip_rect();
	}
	last_drawn_rect = tree_node_rect;
	current_node->data.state = node_state;
	tree_state.level_node_count_stack.last() = level_node_counter + 1;
	tree_state.level_node_count_stack.push(0);
	tree_state.parent_node = current_node;

	Padding padding;
	padding.rects = 5;
	Rect_s32 tree_node_context_rect = tree_node_rect;
	tree_node_context_rect.offset_x(node_offset + text_offset);
	tree_state.context.setup(LAYOUT_HORIZONTALLY_CENTER, &tree_node_context_rect, &tree_node_context_rect, &padding);

	context_stack.push(&tree_state.context);

#ifdef _DEBUG
	tree_node_debug_counter++;
#endif
	return true;
}

void Gui_Manager::end_tree_node()
{
#ifdef _DEBUG
	tree_node_debug_counter--;
#endif
	tree_state.level_node_count_stack.pop();
	tree_state.parent_node = tree_state.parent_node->parent_node;
	tree_state.tree_level_depth -= 1;

	context_stack.pop();
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

inline bool is_integer(const char *string)
{
	assert(string);

	u32 string_len = (u32)strlen(string);
	if (string_len == 0) {
		return false;
	}
	if (string_len == 1) {
		return isdigit(string[0]);
	}
	if (string_len > 1) {
		char first_char = string[0];
		if (!isdigit(first_char) && (first_char != '-') && (first_char != '+')) {
			return false;
		}
		for (u32 i = 1; i < string_len; i++) {
			if (!isdigit(string[i])) {
				return false;
			}
		}
	}
	return true;
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

	s32 result = 0;
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
					if (is_integer(first_line_column_data->text) && is_integer(second_line_column_data->text)) {
						s32 first_integer = atoi(first_line_column_data->text);
						s32 second_integer = atoi(second_line_column_data->text);
						result = gui_manager->picked_column->state == GUI_LIST_COLUMN_STATE_SORTING_UP ? first_integer < second_integer : first_integer > second_integer;
					} else {
						result = compare_strings_priority(first_line_column_data->text, second_line_column_data->text);
						if (gui_manager->picked_column->state == GUI_LIST_COLUMN_STATE_SORTING_DOWN) {
							result *= -1;
						}
					}
					break;
				}
			}
			break;
		}
	}
	return result;
}

bool Gui_Manager::begin_list(const char *name, Gui_List_Column columns[], u32 columns_count)
{
	assert(name);
	assert(columns_count > 0);

	line_list.count = 0;
	list_column_rect_list.count = 0;

	Gui_Window_Theme window_theme;
	window_theme.background_color = list_theme.background_color;
	window_theme.rects_padding = 0;
	window_theme.horizontal_padding = 0;
	window_theme.vertical_padding = 0;
	window_theme.rounded_border = 0;
	window_theme.rounded_scrolling = 0;

	gui::set_theme(&window_theme);

	Size_s32 window_list_size = list_theme.window_size;
	set_next_window_size(window_list_size.width, window_list_size.height);

	Rect_s32 column_top_rect;
	if (list_theme.column_filter) {
		place_rect_on_window_top(list_theme.filter_rect_height, &column_top_rect);
	}

	if (begin_child(name, WINDOW_SCROLL_BAR)) {
		line_list.count = 0;
		Gui_Window *window = get_window();

		if (!list_theme.column_filter) {
			picked_column = &columns[0];
			s32 prev_offset_in_persents = 0;
			for (u32 i = 0; i < columns_count; i++) {
				Rect_s32 column_rect;
				column_rect.x = window->rect.x + calculate(window_list_size.width, prev_offset_in_persents);
				column_rect.y = window->rect.y;
				column_rect.width = calculate(window_list_size.width, (s32)columns[i].size_in_percents) - (s32)math::ceil(list_theme.split_line_thickness);
				column_rect.height = list_theme.filter_rect_height;
				list_column_rect_list.push(column_rect);

				prev_offset_in_persents += columns[i].size_in_percents;
			}
			return true;
		}

		u32 picked_column_index = UINT32_MAX;
		for (u32 i = 0; i < columns_count; i++) {
			Gui_List_Column_State sorting = columns[i].state;
			if (sorting != GUI_LIST_COLUMN_STATE_DEFAULT) {
				picked_column_index = i;
				picked_column = &columns[i];
				break;
			}
		}

		if (picked_column_index == UINT32_MAX) {
			picked_column_index = 0;
			columns[0].state = GUI_LIST_COLUMN_STATE_SORTING_DOWN;
			picked_column = &columns[0];
		}

		window->place_rect_over_window(&column_top_rect);

		Render_Primitive_List *render_list = GET_RENDER_LIST();
		Gui_Window *parent_window = get_parent_window(window);
		render_list->push_clip_rect(&parent_window->clip_rect);
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
			Rect_s32 parent_window_clip_rect = get_parent_window_clip_rect(window);
			column_text_clip_rect = calculate_clip_rect(&parent_window_clip_rect, &column_text_clip_rect);
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
					//render_list->add_texture(&button_rect, &up_texture);
				} else if (columns[i].state == GUI_LIST_COLUMN_STATE_SORTING_DOWN) {
					//render_list->add_texture(&button_rect, &down_texture);
				}
			}
			if ((offset_in_persents < 100) && (i < (columns_count - 1))) {
				Point_s32 first_point = { window->rect.x + calculate(window_list_size.width, offset_in_persents), window->rect.y + half_difference };
				Point_s32 second_point = { window->rect.x + calculate(window_list_size.width, offset_in_persents), window->rect.y + list_theme.filter_rect_height - half_difference };
				render_list->add_line(first_point, second_point, list_theme.split_lines_color, list_theme.split_line_thickness);
			}
			prev_offset_in_persents += columns[i].size_in_percents;
		}
		render_list->pop_clip_rect();
		return true;
	}
	gui::reset_window_theme();
	return false;
}

static const u32 TIME_BETWEEN_LIST_LINE_STEPS = 60; // time in milliseconds
static const u32 QUICK_GO_MODE_TIME = 300; // time in milliseconds

void Gui_Manager::end_list()
{
	Context *context = get_context();
	Gui_Window *window = get_window();
	Gui_ID list_gui_id = GET_LIST_GUI_ID();
	update_active_and_hot_state(window, list_gui_id, &window->view_rect);

	if (detect_intersection(&window->rect) && (was_click(KEY_LMOUSE) || was_click(KEY_RMOUSE))) {
		if (active_list != list_gui_id) {
			active_list = list_gui_id;
		}
	} else if (!detect_intersection(&window->rect) && (was_click(KEY_LMOUSE) || was_click(KEY_RMOUSE))) {
		if (active_list == list_gui_id) {
			active_list = 0;
		}
	}

	if (next_list_active) {
		active_list = list_gui_id;
		next_list_active = false;
	}

	if (active_list == list_gui_id) {
		Window_Context *window_context = static_cast<Window_Context *>(context);
		Window_Placing_State window_placing_state = window_context->get_placing_state();

		Array<Rect_s32> line_rects;
		for (u32 i = 0; i < line_list.count; i++) {
			Rect_s32 temp = { 0, 0, get_window_size().width, list_theme.line_height };
			context->place_rect(&temp);
			line_rects.push(temp);
		}
		static bool run_quick_mode = false;
		static s64 timer = 0;
		static bool check_timer = false;
		if (was_key_just_pressed(KEY_ARROW_DOWN) || was_key_just_pressed(KEY_ARROW_UP)) {
			check_timer = true;
			timer = milliseconds_counter();
		}

		if (was_key_just_released(KEY_ARROW_DOWN) || was_key_just_released(KEY_ARROW_UP)) {
			check_timer = false;
			timer = 0;
			run_quick_mode = false;
		}

		s64 delta_two = milliseconds_counter() - timer;
		if (!run_quick_mode && check_timer && delta_two > QUICK_GO_MODE_TIME) {
			run_quick_mode = true;
		}

		static s64 last_modifing_time = 0;
		s64 delta = milliseconds_counter() - last_modifing_time;
		bool result = delta > TIME_BETWEEN_LIST_LINE_STEPS;

		u32 selected_lines_count = 0;
		if ((was_click(KEY_ARROW_DOWN) || (run_quick_mode && Keys_State::is_key_down(KEY_ARROW_DOWN) && result)) || (was_click(KEY_ARROW_UP) || (run_quick_mode && Keys_State::is_key_down(KEY_ARROW_UP) && result))) {
			for (u32 i = 0; i < line_list.count; i++) {
				Gui_List_Line *line = &line_list[i];
				if (*line->state & GUI_LIST_LINE_SELECTED) {
					selected_lines_count++;
				}
			}
			if (selected_lines_count > 1) {
				u32 index = 0;
				for (index; index < line_list.count; index++) {
					Gui_List_Line *line = &line_list[index];
					if (*line->state & GUI_LIST_LINE_SELECTED) {
						break;
					}
				}
				index += 1;
				for (index; index < line_list.count; index++) {
					Gui_List_Line *line = &line_list[index];
					*line->state = 0;
				}
			}
		}

		// To prevent a line blicking the code should select a line in the next frame bacause 
		// after the call move_window_content a window content moves only at the next frame.
		static u32 reset_index = 0;
		static u32 select_index = 0;
		static bool select_line_in_next_frame = false;
		if (select_line_in_next_frame) {
			select_line_in_next_frame = false;
			*line_list[reset_index].state = 0;
			*line_list[select_index].state = GUI_LIST_LINE_SELECTED;
		}

		if (selected_lines_count == 1) {
			u32 line_index = 0;
			if (was_click(KEY_ARROW_DOWN) || (run_quick_mode && Keys_State::is_key_down(KEY_ARROW_DOWN) && result)) {
				for (; line_index < line_list.count; line_index++) {
					Gui_List_Line *line = &line_list[line_index];
					if ((*line->state & GUI_LIST_LINE_SELECTED) && (line_index != (line_list.count - 1))) {
						select_line_in_next_frame = true;
						reset_index = line_index;
						select_index = line_index + 1;

						if (line_rects[line_index + 1].bottom() > window->view_rect.bottom()) {
							move_window_content(window, line_rects[line_index + 1].bottom() - window->view_rect.bottom(), MOVE_UP);
						}
						last_modifing_time = milliseconds_counter();
						break;
					}
				}
			}
			if (was_click(KEY_ARROW_UP) || (run_quick_mode && Keys_State::is_key_down(KEY_ARROW_UP) && result)) {
				for (; line_index < line_list.count; line_index++) {
					Gui_List_Line *line = &line_list[line_index];
					if ((*line->state & GUI_LIST_LINE_SELECTED) && (line_index != 0)) {
						select_line_in_next_frame = true;
						reset_index = line_index;
						select_index = line_index - 1;

						if (line_rects[line_index - 1].y < window->view_rect.y) {
							move_window_content(window, window->view_rect.y - line_rects[line_index - 1].y, MOVE_DOWN);
						}
						last_modifing_time = milliseconds_counter();
						break;
					}
				}
			}
		}
		window_placing_state.content_rect.x = window->content_rect.x;
		window_placing_state.content_rect.y = window->content_rect.y;
		window_context->set_placing_state(&window_placing_state);
	}

	if (list_theme.column_filter) {
		qsort_s((void *)line_list.items, line_list.count, sizeof(Gui_List_Line), compare_list_lines, (void *)this);
	}

	Render_Primitive_List *render_list = GET_RENDER_LIST();
	render_list->push_clip_rect(&window->clip_rect);

	Rect_s32 line_rect = { 0, 0, get_window_size().width, list_theme.line_height };

	for (u32 line_index = 0; line_index < line_list.count; line_index++) {
		Gui_List_Line *gui_list_line = &line_list[line_index];
		if (*gui_list_line->state > (GUI_LIST_LINE_SELECTED | GUI_LIST_LINE_CLICKED_BY_ENTER_KEY)) {
			*gui_list_line->state = GUI_LIST_LINE_RESET_STATE;
		}
		if (*gui_list_line->state & GUI_LIST_LINE_CLICKED_BY_LEFT_MOUSE) {
			*gui_list_line->state &= ~GUI_LIST_LINE_CLICKED_BY_LEFT_MOUSE;
		}
		if (*gui_list_line->state & GUI_LIST_LINE_CLICKED_BY_RIGHT_MOUSE) {
			*gui_list_line->state &= ~GUI_LIST_LINE_CLICKED_BY_RIGHT_MOUSE;
		}
		if (*gui_list_line->state & GUI_LIST_LINE_CLICKED_BY_ENTER_KEY) {
			*gui_list_line->state &= ~GUI_LIST_LINE_CLICKED_BY_ENTER_KEY;
		}

		context->place_rect(&line_rect);
		if (must_rect_be_drawn(&window->view_rect, &line_rect)) {
			bool mouse_hover = hot_item == list_gui_id;
			if (mouse_hover && was_click(KEY_LMOUSE) && !Keys_State::is_key_down(KEY_CTRL)) {
				update_list_line_state(gui_list_line->state, &line_rect, GUI_LIST_LINE_CLICKED_BY_LEFT_MOUSE);

			} else if (mouse_hover && was_click(KEY_RMOUSE) && !Keys_State::is_key_down(KEY_CTRL)) {
				update_list_line_state(gui_list_line->state, &line_rect, GUI_LIST_LINE_CLICKED_BY_RIGHT_MOUSE);

			} else if (mouse_hover && detect_intersection(&line_rect) && (key_bindings.was_binding_triggered(KEY_CTRL, KEY_LMOUSE))) {
				*gui_list_line->state = GUI_LIST_LINE_RESET_STATE;
				*gui_list_line->state |= GUI_LIST_LINE_SELECTED;
				*gui_list_line->state |= GUI_LIST_LINE_CLICKED_BY_LEFT_MOUSE;

			} else if (was_click(KEY_ENTER) && (*gui_list_line->state && GUI_LIST_LINE_SELECTED)) {
				*gui_list_line->state = GUI_LIST_LINE_RESET_STATE;
				*gui_list_line->state |= GUI_LIST_LINE_SELECTED;
				*gui_list_line->state |= GUI_LIST_LINE_CLICKED_BY_ENTER_KEY;

			}

			Color line_color = list_theme.line_color;
			if (*gui_list_line->state & GUI_LIST_LINE_SELECTED) {
				line_color = list_theme.picked_line_color;
			} else if (mouse_hover && detect_intersection(&line_rect)) {
				line_color = list_theme.hover_line_color;
			}
			render_list->add_rect(&line_rect, line_color);

			for (u32 column_index = 0; column_index < gui_list_line->columns.count; column_index++) {
				Gui_Line_Column *column = &gui_list_line->columns[column_index];
				Rect_s32 *filter_column_rect = &list_column_rect_list[column_index];
				Rect_s32 line_column_rect = { filter_column_rect->x, line_rect.y, filter_column_rect->width, line_rect.height };

				Rect_s32 clip_rect = calculate_clip_rect(&window->clip_rect, &line_column_rect);
				render_list->push_clip_rect(&clip_rect);

				for (u32 i = 0; i < column->rendering_data_list.count; i++) {
					Column_Rendering_Data *rendering_data = &column->rendering_data_list[i];
					if (rendering_data->type == LIST_LINE_INFO_TYPE_TEXT) {
						Rect_s32 line_column_text_rect = get_text_rect(rendering_data->text);
						place_in_middle_and_by_left(&line_column_rect, &line_column_text_rect, list_theme.line_text_offset);
						render_list->add_text(&line_column_text_rect, rendering_data->text);

					} else if (rendering_data->type == LIST_LINE_INFO_TYPE_IMAGE) {
						Rect_s32 texture_rect = { 0, 0, 20, 20 };
						place_in_middle_and_by_left(&line_column_rect, &texture_rect, list_theme.line_text_offset);
						//render_list->add_texture(&texture_rect, rendering_data->texture);
						line_column_rect.x += texture_rect.width + 5;
					}
				}
				render_list->pop_clip_rect();
			}
		}
	}
	render_list->pop_clip_rect();
	end_child();
	gui::reset_window_theme();
	list_count++;
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

//void Gui_Manager::add_image(Texture2D *texture, Alignment alignment)
//{
//	Column_Rendering_Data data;
//	data.type = LIST_LINE_INFO_TYPE_IMAGE;
//	data.texture = texture;
//	current_list_column.rendering_data_list.push(data);
//}
//
//void Gui_Manager::add_image_button(Texture2D *texture, Alignment alignment)
//{
//}

bool Gui_Manager::element_clicked(Key key)
{
	if (detect_intersection(&last_drawn_rect) && was_click(key)) {
		return true;
	}
	return false;
}

bool Gui_Manager::window_active(Gui_Window *window)
{
	assert(window);
	if (window->gui_id == active_window) {
		return true;
	}
	bool result = false;
	for (u32 i = 0; i < window->child_windows.count; i++) {
		Gui_Window *child_window = &windows[window->child_windows[i]];
		if (window_active(child_window)) {
			result = true;
			break;
		}
	}
	return result;
}

bool Gui_Manager::handle_edit_field_shortcut_event(Gui_Window *window, Rect_s32 *edit_field_rect, Gui_ID edit_field_gui_id, Edit_Field_Type edit_field_type, u32 vector3_edit_field_index)
{
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

	if (next_ui_element_active && window_active(window) && (active_edit_field != edit_field_gui_id)) {
		next_ui_element_active = false;
		active_edit_field = edit_field_gui_id;
		edit_field_state = make_edit_field_state(&edit_field_instance->caret_rect, edit_field_instance->editing_value, edit_field_instance->max_chars_number, edit_field_instance->symbol_validation);
	}

	if (active_edit_field == edit_field_gui_id) {
		if (update_next_time_editing_value) {
			edit_field_state.data = edit_field_instance->editing_value;
			update_next_time_editing_value = false;
		}
		if (change_active_field) {
			edit_field_state = make_edit_field_state(&edit_field_instance->caret_rect, edit_field_instance->editing_value, edit_field_instance->max_chars_number, edit_field_instance->symbol_validation);
		}
		if ((strlen(edit_field_instance->editing_value) == 0) && !edit_field_state.data.is_empty()) {
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

				int point_index = edit_field_state.data.find(".");
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

void Gui_Manager::make_next_list_active()
{
	next_list_active = true;
}

void Gui_Manager::text(const char *some_text)
{
	Gui_Window *window = get_window();

	Rect_s32 text_rect = get_text_rect(some_text);
	text_rect.height = 20;
	Rect_s32 alignment_text_rect = get_text_rect(some_text);

	Context *context = get_context();
	context->place_rect(&text_rect);

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
	static u32 pre_active_tab_count = UINT32_MAX;

	Gui_Window *window = get_window();
	assert(window->style & WINDOW_TAB_BAR);
	assert(tab_theme.tab_bar_height >= tab_theme.tab_height);

	s32 diff = tab_theme.tab_bar_height - tab_theme.tab_height;

	Rect_s32 name_rect = get_text_rect(tab_name);
	Rect_s32 tab_rect = { window->tab_position.x, window->tab_position.y + diff, name_rect.width + tab_theme.additional_space_in_tab, tab_theme.tab_height };

	bool draw_tab = true;
	bool current_tab_active = false;

	Gui_ID tab_gui_id = GET_TAB_GUI_ID();
	if (tab_gui_id == active_tab) {
		pre_active_tab_count = tab_count;
		current_tab_active = true;
	}

	Gui_Window_Tab *window_tab = window->find_tab(tab_gui_id);
	if (window_tab) {
		if (!window_tab->draw) {
			return false;
		}
		update_active_and_hot_state(window, tab_gui_id, &tab_rect);
		if (tab_gui_id == active_item) {
			if (active_tab != active_item) {
				Gui_Window_Tab *prev_active_window_tab = window->find_tab(active_tab);
				if (prev_active_window_tab) {
					prev_active_window_tab->placing_info.scroll = window->scroll;
					prev_active_window_tab->placing_info.content_rect = window->content_rect;

					window->scroll = window_tab->placing_info.scroll;
					window->content_rect = window_tab->placing_info.content_rect;
				}
			}
			if (tab_count < pre_active_tab_count) {
				current_tab_active = true;
			}
			pre_active_tab_count = tab_count;
			active_tab = active_item;
		}
	} else {
		s32 offset = 0;
		offset += window->style & WINDOW_HEADER ? window_theme.header_height : 0;
		offset += window->style & WINDOW_TAB_BAR ? tab_theme.tab_bar_height : 0;

		Gui_Window_Tab tab;
		tab.draw = true;
		tab.gui_id = tab_gui_id;
		tab.placing_info.content_rect = { window->rect.x, window->rect.y + offset, 0, 0 };
		tab.placing_info.scroll = Point_s32(window->rect.x, window->rect.y + offset);

		if ((active_tab == 0) && window->tabs.is_empty()) {
			active_tab = tab_gui_id;
			pre_active_tab_count = tab_count;
			current_tab_active = true;
		}
		window->tabs.push(tab);
	}

	if (must_rect_be_drawn(&window->clip_rect, &tab_rect) && draw_tab) {
		Render_Primitive_List *render_list = GET_RENDER_LIST();

		Rect_s32 clip_rect = calculate_clip_rect(&window->tab_bar_clip_rect, &tab_rect);
		render_list->push_clip_rect(&clip_rect);

		if (active_tab != tab_gui_id) {
			render_list->add_rect(&tab_rect, tab_theme.tab_color, 10, ROUND_TOP_RECT);
		} else {
			render_list->add_rect(&tab_rect, tab_theme.active_tab_color, 10, ROUND_TOP_RECT);
		}
		place_in_middle(&tab_rect, &name_rect, BOTH_AXIS);
		render_list->add_text(&name_rect, tab_name);

		render_list->pop_clip_rect();
	}
	tab_count++;
	window->tab_position.x += tab_rect.width;

	return current_tab_active;
}

//void Gui_Manager::image(Texture2D *texture, s32 width, s32 height)
//{
//	assert(texture);
//
//	Rect_s32 image_rect = { 0, 0, width, height };
//
//	Context *context = get_context();
//	context->place_rect(&image_rect);
//
//	Gui_Window *window = get_window();
//	if (must_rect_be_drawn(&window->clip_rect, &image_rect)) {
//
//		Render_Primitive_List *render_list = GET_RENDER_LIST();
//		render_list->push_clip_rect(&window->clip_rect);
//		render_list->add_texture(image_rect.x, image_rect.y, image_rect.width, image_rect.height, texture);
//		render_list->pop_clip_rect();
//	}
//}

static bool find_window_by_name(const Gui_Window &window, const String &name)
{
	return true;
}

void Gui_Manager::list_box(Array<String> *array, u32 *item_index)
{
	assert(array->count > 0);

	if (*item_index >= array->count) {
		*item_index = 0;
	}
	Rect_s32 list_box_rect = list_box_theme.default_rect;
	Rect_s32 expand_down_texture_rect = list_box_theme.expand_down_texture_rect;

	Context *context = get_context();
	context->place_rect(&list_box_rect);

	Gui_Window *window = get_window();
	if (must_rect_be_drawn(&window->clip_rect, &list_box_rect)) {
		Rect_s32 drop_window_rect;
		drop_window_rect.set(list_box_rect.x, list_box_rect.bottom() + 5);
		drop_window_rect.set_size(list_box_rect.width, array->count * button_theme.rect.height);

		Gui_ID list_box_gui_id = GET_LIST_BOX_GUI_ID();
		update_active_and_hot_state(window, list_box_gui_id, &list_box_rect);

		if (was_click(KEY_LMOUSE)) {
			if (hot_item == list_box_gui_id) {
				if (active_list_box != active_item) {
					active_list_box = active_item;
				} else {
					active_list_box = 0;
					setup_active_window(window);
				}
			} else {
				if ((active_list_box == list_box_gui_id) && !detect_intersection(&drop_window_rect)) {
					active_list_box = 0;
					setup_active_window(window);
				}
			}
		}
		Gui_Window *parent_window = window;

		u32 text_layout = LAYOUT_LEFT;
		if (active_list_box == list_box_gui_id) {
			Gui_Window_Theme win_theme;
			win_theme.horizontal_padding = 0;
			win_theme.vertical_padding = 0;
			win_theme.outlines_width = 1.0f;
			win_theme.rects_padding = 0;

			Gui_Text_Button_Theme button_theme;
			button_theme.rect.width = list_box_rect.width;
			button_theme.color = window_theme.background_color;
			button_theme.text_layout |= text_layout;

			gui::set_theme(&win_theme);
			gui::set_theme(&button_theme);

			set_next_window_pos(drop_window_rect.x, drop_window_rect.y);
			set_next_window_size(drop_window_rect.width, drop_window_rect.height);

			String window_name = "list_box_" + String((s32)list_box_gui_id);
			begin_window(window_name, WINDOW_OUTLINES);
			move_window_on_rendering_order_top(get_window());

			for (u32 i = 0; i < array->count; i++) {
				if (button(array->get(i))) {
					*item_index = i;
					active_list_box = 0;
					setup_active_window(parent_window);
				}
			}
			end_window();

			gui::reset_button_theme();
			gui::reset_window_theme();
		}

		window = get_window();

		Rect_s32 name_rect = get_text_rect(array->get(*item_index));

		if (text_layout & LAYOUT_RIGHT) {
			place_in_middle_and_by_right(&list_box_rect, &name_rect, list_box_theme.shift_from_size);
		} else if (text_layout & LAYOUT_LEFT) {
			place_in_middle_and_by_left(&list_box_rect, &name_rect, list_box_theme.shift_from_size);
		} else {
			place_in_middle(&list_box_rect, &name_rect, BOTH_AXIS);
		}
		place_in_middle_and_by_right(&list_box_rect, &expand_down_texture_rect, list_box_theme.expand_down_texture_shift);

		Render_Primitive_List *render_list = GET_RENDER_LIST();
		render_list->push_clip_rect(&window->clip_rect);
		render_list->add_rect(&list_box_rect, list_box_theme.background_color, list_box_theme.rounded_border);
		render_list->add_text(&name_rect, array->get(*item_index));
		//render_list->add_texture(&expand_down_texture_rect, &expand_down_texture);
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
	Context *context = get_context();
	Gui_Window *window = get_window();

	Rect_s32 button_rect = button_theme.rect;
	context->place_rect(&button_rect);

	bool mouse_hover = false;
	if (must_rect_be_drawn(&window->clip_rect, &button_rect)) {

		u32 button_gui_id = GET_BUTTON_GUI_ID();
		update_active_and_hot_state(window, button_gui_id, &button_rect);

		mouse_hover = (hot_item == button_gui_id);

		Rect_s32 name_rect = get_text_rect(name);

		if (button_theme.text_layout & LAYOUT_RIGHT) {
			place_in_middle_and_by_right(&button_rect, &name_rect, button_theme.shift_from_size);
		} else if (button_theme.text_layout & LAYOUT_LEFT) {
			place_in_middle_and_by_left(&button_rect, &name_rect, button_theme.shift_from_size);
		} else {
			place_in_middle(&button_rect, &name_rect, BOTH_AXIS);
		}
		Color button_color = mouse_hover ? button_theme.hover_color : button_theme.color;
		Render_Primitive_List *render_list = GET_RENDER_LIST();
		Rect_s32 clip_rect = calculate_clip_rect(&window->clip_rect, &button_rect);
		render_list->push_clip_rect(&clip_rect);
		render_list->add_rect(&button_rect, button_color, button_theme.rounded_border);
		render_list->add_text(&name_rect, name);
		render_list->pop_clip_rect();
	}
	*left_mouse_click = false;
	if (was_click(KEY_LMOUSE) && mouse_hover) {
		*left_mouse_click = true;
	}
	*right_mouse_click = false;
	if (was_click(KEY_RMOUSE) && mouse_hover) {
		*right_mouse_click = true;
	}
	button_count++;
}

void Gui_Manager::init(Engine *engine)
{
	//render_2d = &engine->render_sys.render_2d;

	key_bindings.bind(KEY_CTRL, KEY_BACKSPACE);
	key_bindings.bind(KEY_CTRL, KEY_ARROW_UP);
	key_bindings.bind(KEY_CTRL, KEY_ARROW_DOWN);
	key_bindings.bind(KEY_CTRL, KEY_ARROW_LEFT);
	key_bindings.bind(KEY_CTRL, KEY_ARROW_RIGHT);
	key_bindings.bind(KEY_CTRL, KEY_LMOUSE);

	s32 font_size = 12;
	String font_name = "consola";

	Variable_Service *gui = engine->var_service.find_namespace("gui");
	ATTACH(gui, font_size);
	ATTACH(gui, font_name);

	set_font(font_name, (u32)font_size);
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
		int string_len = 0;
		save_file.read((void *)&string_len, sizeof(int));

		char *string = new char[string_len + 1];
		save_file.read((void *)string, string_len);
		string[string_len] = '\0';

		Rect_s32 rect;
		save_file.read((void *)&rect, sizeof(Rect_s32));
		saved_windows_rects.push({ string, rect });

		free_string(string);
	}
}

struct Name_Texture {
	const char *name = NULL;
	//Texture2D *texture = NULL;
};

void Gui_Manager::load_and_init_textures(Gpu_Device *gpu_device)
{
	assert(gpu_device);

	//cross_icon_image.init_from_file("cross-small.png", "editor");

	//const u32 TEXTURE_COUNT = 5;
	//Name_Texture pairs[TEXTURE_COUNT] = { { "expand_down1.png", &expand_down_texture }, { "check2.png", &check_texture }, { "up.png", &up_texture }, { "down.png", &down_texture }, { "expand_right.png", &expand_right_texture } };

	//Texture2D_Desc texture_desc;
	//texture_desc.width = 64;
	//texture_desc.height = 64;
	//texture_desc.mip_levels = 1;

	//gpu_device->create_texture_2d(&texture_desc, &default_texture);
	//gpu_device->create_shader_resource_view(&texture_desc, &default_texture);

	//Color transparent_value = Color(255, 255, 255, 0);
	//fill_texture((void *)&transparent_value, &default_texture);

	//u8 *image_data = NULL;
	//u32 width = 0;
	//u32 height = 0;
	//String full_path;
	//for (u32 i = 0; i < TEXTURE_COUNT; i++) {
	//	build_full_path_to_editor_file(pairs[i].name, full_path);
	//	if (!create_texture2d_from_file(full_path, *pairs[i].texture)) {
	//		*pairs[i].texture = default_texture;
	//	}
	//}
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

	Render_Font *new_render_font = render_2d->get_render_font(new_font);
	if (!new_render_font) {
		print(" Gui_Manager::set_font: Failed to set new font '{}' wit size {}. Render font was not found.", font_name, font_size);
	}
	render_font = new_render_font;
}

void Gui_Manager::shutdown()
{
	String path_to_save_file;
	build_full_path_to_gui_file("new_gui_data.gui", path_to_save_file);

	File save_file;
	if (save_file.open(path_to_save_file, FILE_MODE_WRITE, FILE_CREATE_ALWAYS)) {
		u32 window_parent_count = 0;
		Gui_Window *window = NULL;
		For(windows, window)
		{
			if (window->type == WINDOW_TYPE_PARENT) {
				window_parent_count++;
			}
		}
		save_file.write((void *)&window_parent_count, sizeof(u32));

		For(windows, window)
		{
			if (window->type == WINDOW_TYPE_PARENT) {
				save_file.write((void *)&window->name.len, sizeof(int));
				save_file.write((void *)&window->name.data[0], window->name.len);
				save_file.write((void *)&window->rect, sizeof(Rect_s32));
			}
		}
	} else {
		print("Gui_Manager::shutdown: Hades gui data can not be save in file by path {}.", path_to_save_file);
	}
}

void Gui_Manager::new_frame()
{
	pre_setup.window_rect = { 0, 0, 0, 0 };
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
	menu_item_count = 0;
	edit_field_count = 0;
	became_just_actived = 0;
	hover_window = 0;

	s32 max_windows_order_index = -1;
	Gui_Window *_hover_window = NULL;
	for (u32 i = 0; i < windows_order.count; i++) {
		Gui_Window *window = get_window_by_index(&windows_order, i);
		Rect_s32 window_rect = (window->style & WINDOW_RESIZABLE) ? Rect_s32(window->rect.x - 5, window->rect.y - 5, window->rect.width + 10, window->rect.height + 10) : window->rect;
		if (detect_intersection(&window_rect)) {
			if (window->index_in_windows_order > max_windows_order_index) {
				max_windows_order_index = window->index_in_windows_order;
				hover_window = window->gui_id;
				_hover_window = window;
			}
		}
	}

	if (was_key_just_pressed(KEY_LMOUSE) || was_key_just_pressed(KEY_RMOUSE)) {
		if ((resizing_window == 0) && (hover_window != 0) && (hover_window != active_window)) {
			windows_order.remove(_hover_window->index_in_windows_order);
			for (u32 i = 0; i < windows_order.count; i++) {
				Gui_Window *window = get_window_by_index(&windows_order, i);
				window->index_in_windows_order = i;
			}
			_hover_window->index_in_windows_order = windows_order.count;
			windows_order.push(_hover_window->index_in_windows_array);
			setup_active_window(_hover_window);
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

	for (u32 i = 0; i < windows_order.count; i++) {
		Gui_Window *window = get_window_by_index(&windows_order, i);
		render_2d->add_render_primitive_list(&window->render_list);
		for (u32 i = 0; i < window->child_windows.count; i++) {
			Gui_Window *child_window = get_window_by_index(&window->child_windows, i);
			render_2d->add_render_primitive_list(&child_window->render_list);
		}
	}
}

bool Gui_Manager::begin_window(const char *name, Window_Style window_style, bool window_open)
{
	Gui_Window *window = find_window(name);
	if (!window) {
		window = create_window(name, WINDOW_TYPE_PARENT, window_style, window_open);
		setup_active_window(window);
	}

	if (!window->open) {
		reset_window_params = 0;
		return false;
	}

	Rect_s32 prev_view_rect = window->view_rect;
	Rect_s32 prev_content_rect = window->content_rect;

	window_stack.push(window->index_in_windows_array);
	window->new_frame(window_style);

	if (next_ui_element_active) {
		next_ui_element_active = false;
		setup_active_window(window);
	} else {
		//@Note: Can I get rid of it ?
		update_active_and_hot_state(window, window->gui_id, &window->rect);
	}
	Rect_s32 *rect = &window->rect;

	bool mouse_was_moved = (mouse_x_delta != 0) || (mouse_y_delta != 0);
	if ((window_style & WINDOW_MOVE) && mouse_was_moved && !active_scrolling && (resizing_window == 0) && (active_window == window->gui_id) && Keys_State::is_key_down(KEY_LMOUSE)) {
	Size_u32 window_size = Engine::get_render_system()->get_window_size();
		s32 min_window_position = 0;
		if (window->style & WINDOW_OUTLINES) {
			min_window_position = (s32)window_theme.outlines_width;
		}
		//s32 x = math::clamp(rect->x + mouse_x_delta, min_window_position, (s32)Render_System::screen_width - rect->width);
		//s32 y = math::clamp(rect->y + mouse_y_delta, min_window_position, (s32)Render_System::screen_height - rect->height);
		//window->set_position(x, y);
	}

	if ((window_style & WINDOW_AUTO_WIDTH) && !((reset_window_params & SET_WINDOW_SIZE) && (pre_setup.window_rect.width > 0))) {
		rect->width = prev_content_rect.width;
	}

	if ((window_style & WINDOW_AUTO_HEIGHT) && !((reset_window_params & SET_WINDOW_SIZE) && (pre_setup.window_rect.height > 0))) {
		rect->height = prev_content_rect.height;
	}

	static Rect_Side rect_side;
	static Cursor_Type cursor_type = CURSOR_TYPE_ARROW;
	if ((WINDOW_RESIZABLE & window_style) && !active_scrolling && (resizing_window == 0) && detect_window_borders_intersection(rect, &rect_side)) {
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
			//if ((rect->right() + mouse_x_delta) < (s32)Render_System::screen_width) {
			//	rect->width = math::max(rect->width + mouse_x_delta, MIN_WINDOW_WIDTH);
			//}
		}
		if ((rect_side == RECT_SIDE_BOTTOM) || (rect_side == RECT_SIDE_RIGHT_BOTTOM) || (rect_side == RECT_SIDE_LEFT_BOTTOM)) {
			//if ((rect->bottom() + mouse_y_delta) < (s32)Render_System::screen_height) {
			//	rect->height = math::max(rect->height + mouse_y_delta, MIN_WINDOW_HEIGHT);
			//}
			//if (window->display_vertical_scrollbar) {
			//	if (prev_view_rect.bottom() == prev_content_rect.bottom()) {
			//		s32 delta = rect->bottom() - prev_content_rect.bottom();
			//		window->content_rect.y += delta;
			//	}
			//}
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
		window->set_position(pre_setup.window_rect.x, pre_setup.window_rect.y);
		reset_window_params &= ~SET_WINDOW_POSITION;
	}

	if (reset_window_params & SET_WINDOW_SIZE) {
		rect->width = pre_setup.window_rect.width > 0 ? pre_setup.window_rect.width : rect->width;
		rect->height = pre_setup.window_rect.height > 0 ? pre_setup.window_rect.height : rect->height;
		reset_window_params &= ~SET_WINDOW_SIZE;
	}

	if (reset_window_params & SET_WINDOW_THEME) {
		//backup_window_theme = window_theme;
		//window_theme = future_window_theme;
	}

	window->content_rect.width += window_theme.horizontal_padding;
	window->content_rect.height += window_theme.vertical_padding - window_theme.rects_padding;

	render_window(window);
	window->clip_rect = window->view_rect;

	if (window->open) {
		Padding padding = { window_theme.rects_padding, window_theme.horizontal_padding, window_theme.vertical_padding };
		window->context->setup(current_layout, window, &padding);
		context_stack.push(window->context);
	}
	return window->open;
}

void Gui_Manager::end_window()
{
	Gui_Window *window = get_window();

	window->content_rect.width += window_theme.horizontal_padding;
	window->content_rect.height += window_theme.vertical_padding;

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

	if (reset_window_params & SET_WINDOW_THEME) {
		reset_window_params &= ~SET_WINDOW_THEME;
		window_theme = Gui_Window_Theme();
	}
	window_stack.pop();
	context_stack.pop();
}

void Gui_Manager::render_window(Gui_Window *window)
{
	Rect_s32 *rect = &window->rect;

	Render_Primitive_List *render_list = GET_RENDER_LIST();
	render_list->add_rect(&window->rect, window_theme.background_color, window_theme.rounded_border);
#if OUTLINE_ACTIVE_WINDOW
	if (active_window == window->gui_id) {
		render_list->add_outlines(rect->x, rect->y, rect->width, rect->height, Color::Red, 3.0f, window_theme.rounded_border);
	}
#endif
	if (window->style & WINDOW_OUTLINES) {
		render_list->add_outlines(rect->x, rect->y, rect->width, rect->height, window_theme.outlines_color, window_theme.outlines_width, window_theme.rounded_border);
	}
	if (window->style & WINDOW_HEADER) {
		Rect_s32 header_rect = { rect->x, rect->y, rect->width, window_theme.header_height };
		window->place_rect_over_window(&header_rect);
		render_list->add_rect(&header_rect, window_theme.header_color, window_theme.rounded_border, ROUND_TOP_RECT);

		if (window->type == WINDOW_TYPE_CHILD) {
			Rect_s32 parent_window_clip_rect = get_parent_window_clip_rect(window);
			Rect_s32 temp = calculate_clip_rect(&parent_window_clip_rect, &header_rect);
			render_list->push_clip_rect(&temp);
		} else {
			render_list->push_clip_rect(&header_rect);
		}
		Rect_s32 header_text_rect = get_text_rect(window->name);
		place_in_middle(&header_rect, &header_text_rect, BOTH_AXIS);

		const char *header_text = window_theme.header_text ? window_theme.header_text : window->name.c_str();
		render_list->add_text(&header_text_rect, header_text);

		if (window->style & WINDOW_CLOSE_BUTTON) {
			Rect_s32 cross_icon_image_rect = { 0, 0, header_rect.height, header_rect.height };
			place_in_middle_and_by_right(&header_rect, &cross_icon_image_rect, 5);
			if (base_image_button(&cross_icon_image_rect, &cross_icon_image)) {
				window->open = false;
			}
		}
		render_list->pop_clip_rect();
	}

	if (window->style & WINDOW_TAB_BAR) {
		window->tab_position = Point_s32(window->view_rect.x, window->view_rect.y);
		Rect_s32 tab_bar_rect = { window->view_rect.x, window->view_rect.y, window->view_rect.width, tab_theme.tab_bar_height };
		window->tab_bar_clip_rect = calculate_clip_rect(&window->clip_rect, &tab_bar_rect);
		window->place_rect_over_window(&tab_bar_rect);
		render_list->add_rect(&tab_bar_rect, tab_theme.tab_bar_color);
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

void Gui_Manager::open_window(const char *name)
{
	Gui_Window *window = find_window(name);
	if (window && !window->open) {
		window->open = true;
		setup_active_window(window);
		move_window_on_rendering_order_top(window);
	} else {
		pre_setup.windows_params.push({ name, true });
	}
}

void Gui_Manager::close_window(const char *name)
{
	Gui_Window *window = find_window(name);
	if (window && window->open) {
		window->open = false;
		remove_window_from_rendering_order(window);
	} else {
		pre_setup.windows_params.push({ name, false });
	}
}

bool Gui_Manager::begin_child(const char *name, Window_Style window_style)
{
	assert(!window_stack.is_empty());

	Gui_Window *parent_window = get_window();
	Gui_Window *child_window = find_window(name);
	if (!child_window) {
		child_window = create_window(name, WINDOW_TYPE_CHILD, window_style, true);
		//Windows array could be resized.
		parent_window = get_window();
		parent_window->child_windows.push(child_window->index_in_windows_array);
		child_window->parent_window_idx = parent_window->index_in_windows_array;
	}

	Rect_s32 child_rect = { 0, 0, child_window->rect.width, child_window->rect.height };

	Gui_Window_Theme theme;
	if (!backup_window_themes.is_empty()) {
		theme = backup_window_themes.last();
	}
	parent_window->context->place_rect(&child_rect);
	if (must_rect_be_drawn(&parent_window->view_rect, &child_rect)) {

		child_window->set_position(child_rect.x, child_rect.y);
		child_window->new_frame(window_style);

		if ((was_key_just_pressed(KEY_LMOUSE) || was_key_just_pressed(KEY_RMOUSE)) && (parent_window->gui_id == hover_window) && detect_intersection(&child_window->rect)) {
			setup_active_window(child_window);
		}

		if (reset_window_params & SET_WINDOW_POSITION) {
			child_window->set_position(pre_setup.window_rect.x, pre_setup.window_rect.y);
			reset_window_params &= ~SET_WINDOW_POSITION;
		}

		if (reset_window_params & SET_WINDOW_SIZE) {
			child_window->rect.set_size(pre_setup.window_rect.width, pre_setup.window_rect.height);
			child_window->view_rect = child_window->rect;
			reset_window_params &= ~SET_WINDOW_SIZE;
		}

		if (reset_window_params & SET_WINDOW_THEME) {
			//backup_window_theme = window_theme;
			//window_theme = future_window_theme;
			//reset_window_params &= ~SET_WINDOW_THEME;
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
		window_stack.push(child_window->index_in_windows_array);

		Render_Primitive_List *render_list = &child_window->render_list;
		render_list->push_clip_rect(&parent_window->clip_rect);
		render_window(child_window);
		render_list->pop_clip_rect();

		child_window->clip_rect = calculate_clip_rect(&parent_window->clip_rect, &child_window->view_rect);
		Padding padding = { window_theme.rects_padding, window_theme.horizontal_padding, window_theme.vertical_padding };
		child_window->context->setup(current_layout, child_window, &padding);
		context_stack.push(child_window->context);

		return true;
	}
	return false;
}

void Gui_Manager::end_child()
{
	Gui_Window *window = get_window();

	window->content_rect.width += window_theme.horizontal_padding;
	window->content_rect.height += window_theme.vertical_padding;

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
		//window_theme = backup_window_theme;
		//reset_window_params &= ~SET_WINDOW_THEME;
	}

#if DRAW_CHILD_WINDOW_DEBUG_RECTS
	draw_debug_rect(window, &window->view_rect, 0.1f);
	draw_debug_rect(window, &window->content_rect, 0.2f, Color::Green);
#endif

	window->content_rect.set_size(0, 0);
	window_stack.pop();
	context_stack.pop();
}

void Gui_Manager::same_line()
{
	Context *context = get_context();
	Gui_Layout prev_layout = context->layout;
	context->layout = 0;
	context->layout |= LAYOUT_HORIZONTALLY;
	context->layout |= HORIZONTAL_LAYOUT_JUST_SET;
}

void Gui_Manager::next_line()
{
	Context *context = get_context();
	Gui_Layout prev_layout = context->layout;
	context->layout = 0;
	context->layout |= LAYOUT_VERTICALLY;
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
		Rect_s32 clip_rect;
		if (window->type == WINDOW_TYPE_CHILD) {
			Rect_s32 parent_window_clip_rect = get_parent_window_clip_rect(window);
			clip_rect = calculate_clip_rect(&parent_window_clip_rect, &window->rect);
		} else {
			clip_rect = window->rect;
		}
		render_list->push_clip_rect(&clip_rect);
		render_list->add_rect(&scroll_rect, window_theme.scroll_color, window_theme.rounded_scrolling);
		render_list->pop_clip_rect();

		window->scroll[axis] = scroll_rect[axis];
	}
}

void Gui_Manager::move_window_content(Gui_Window *window, u32 distance, Moving_Direction moving_direction)
{
	if (window->content_rect.height > window->view_rect.height) {
		u32 difference = (u32)(window->content_rect.height - window->view_rect.height);
		s32 content_offset = (s32)math::min(difference, distance);
		s32 scroll_offset = (s32)(((float)window->view_rect.height / (float)window->content_rect.height) * (float)content_offset);

		if (moving_direction == MOVE_UP) {
			window->content_rect.y -= content_offset;
			window->scroll.y += scroll_offset;
		} else if (moving_direction == MOVE_DOWN) {
			window->content_rect.y += content_offset;
			window->scroll.y -= scroll_offset;
		} else {
			assert(false);
		}
	}
}

bool Gui_Manager::detect_window_borders_intersection(Rect_s32 *rect, Rect_Side *rect_side)
{
	static s32 offset_from_border = 5;
	static s32 tri_size = 10;

	Point_s32 mouse = { mouse_x, mouse_y };
	Triangle<s32> left_triangle = Triangle<s32>(Point_s32(rect->x, rect->bottom()), Point_s32(rect->x, rect->bottom() - tri_size), Point_s32(rect->x + tri_size, rect->bottom()));
	Triangle<s32> right_triangle = Triangle<s32>(Point_s32(rect->right() - tri_size, rect->bottom()), Point_s32(rect->right(), rect->bottom() - tri_size), Point_s32(rect->right(), rect->bottom()));

	if (detect_intersection(left_triangle, mouse)) {
		*rect_side = RECT_SIDE_LEFT_BOTTOM;
		return true;
	} else if (detect_intersection(right_triangle, mouse)) {
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

void gui::init_gui(Engine *engine)
{
	gui_manager.init(engine);
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

//void gui::image(Texture2D *texture, s32 width, s32 height)
//{
//	gui_manager.image(texture, width, height);
//}

void gui::list_box(Array<String> *array, u32 *item_index)
{
	gui_manager.list_box(array, item_index);
}

bool gui::radio_button(const char *name, bool *state)
{
	return gui_manager.radio_button(name, state);
}

//bool gui::image_button(u32 width, u32 height, Texture2D *texture)
//{
//	Gui_Window *window = gui_manager.get_window();
//	Rect_s32 rect = { -1, -1, (s32)width, (s32)height };
//	return gui_manager.image_button(&rect, texture, &window->view_rect);
//}
bool gui::image_button(Image *image)
{
	return gui_manager.image_button(image);
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

bool gui::begin_menu(const char *name)
{
	return gui_manager.begin_menu(name);
}

bool gui::menu_item(const char *text, const char *shortcut, bool submenu)
{
	return gui_manager.menu_item(NULL, text, shortcut, submenu);
}

bool gui::menu_item(Image *image, const char *text, const char *shortcut, bool submenu)
{
	return gui_manager.menu_item(image, text, shortcut, submenu);
}

void gui::end_menu()
{
	gui_manager.end_menu();
}

void gui::segment()
{
	gui_manager.segment();
}

void gui::open_menu(const char *name)
{
	gui_manager.open_window(MENU_WINDOW_PREFIX + name);
}

void gui::close_menu(const char *name)
{
	gui_manager.close_window(MENU_WINDOW_PREFIX + name);
}

bool gui::begin_tree(const char *name)
{
	return gui_manager.begin_tree(name);
}

void gui::end_tree()
{
	gui_manager.end_tree();
}

bool gui::begin_tree_node(const char *name_label, Gui_Tree_Style node_flags)
{
	return gui_manager.begin_tree_node(name_label, node_flags);
}

void gui::end_tree_node()
{
	gui_manager.end_tree_node();
}

bool gui::tree_node_selected()
{
	if (gui_manager.tree_state.current_tree && gui_manager.tree_state.parent_node) {
		return gui_manager.tree_state.parent_node->data.state & GUI_TREE_NODE_SELECTED;
	}
	return false;
}

bool gui::element_clicked(Key key)
{
	return gui_manager.element_clicked(key);
}

bool gui::element_double_clicked(Key key)
{
	if (detect_intersection(&gui_manager.last_drawn_rect) && was_double_click(key)) {
		return true;
	}
	return false;
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
	return list_line_state & GUI_LIST_LINE_CLICKED_BY_LEFT_MOUSE;
}

bool gui::right_mouse_click(Gui_List_Line_State list_line_state)
{
	return list_line_state & GUI_LIST_LINE_CLICKED_BY_RIGHT_MOUSE;
}

bool gui::enter_key_click(Gui_List_Line_State list_line_state)
{
	return list_line_state & GUI_LIST_LINE_CLICKED_BY_ENTER_KEY;
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
//
//void gui::add_image(Texture2D *texture, Alignment alignment)
//{
//	gui_manager.add_image(texture, alignment);
//}
//
//void gui::add_image_button(Texture2D *texture, Alignment alignment)
//{
//	gui_manager.add_image_button(texture, alignment);
//}

bool gui::mouse_over_element()
{
	return detect_intersection(&gui_manager.last_drawn_rect);
}

Size_s32 gui::get_window_size()
{
	Size_s32 size = gui_manager.get_window()->view_rect.get_size();
	s32 temp = gui_manager.window_theme.horizontal_padding * 2;
	if (size.width > temp) {
		size.width -= temp;
	}
	temp = gui_manager.window_theme.vertical_padding * 2;
	if (size.height > temp) {
		size.height -= temp;
	}
	return size;
}

Gui_ID gui::get_last_tab_gui_id()
{
	return gui_manager.get_last_tab_gui_id();
}

static Gui_Layout temp_layout = 0;

void gui::set_layout()
{
	Context *context = gui_manager.get_context();
	temp_layout = context->layout;
	context->layout = LAYOUT_HORIZONTALLY_CENTER;
}

void gui::reset_layout()
{
	Context *context = gui_manager.get_context();
	context->layout = temp_layout;
}

Render_Primitive_List *gui::get_render_primitive_list()
{
	return &gui_manager.get_window()->render_list;
}

void gui::make_tab_active(Gui_ID tab_gui_id)
{
	return gui_manager.make_tab_active(tab_gui_id);
}

void gui::make_next_list_active()
{
	return gui_manager.make_next_list_active();
}

void gui::make_next_ui_element_active()
{
	gui_manager.next_ui_element_active = true;
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

void gui::open_window(const char *name)
{
	gui_manager.open_window(name);
}

void gui::close_window(const char *name)
{
	gui_manager.close_window(name);
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
	assert(false);
	gui_manager.set_next_window_theme(gui_window_theme);
}

void gui::set_theme(Gui_Window_Theme *gui_window_theme)
{
	gui_manager.backup_window_themes.push(gui_manager.window_theme);
	gui_manager.window_theme = *gui_window_theme;
}

void gui::set_theme(Gui_Text_Button_Theme *gui_button_theme)
{
	gui_manager.button_theme = *gui_button_theme;
}

void gui::set_theme(Gui_Image_Button_Theme *gui_button_theme)
{
	gui_manager.image_button_theme = *gui_button_theme;
}

void gui::set_theme(Gui_Edit_Field_Theme *gui_edit_field_theme)
{
	gui_manager.backup_edit_field_theme = gui_manager.edit_field_theme;
	gui_manager.edit_field_theme = *gui_edit_field_theme;
}

void gui::set_window_padding(s32 horizontal, s32 vertical, s32 rects)
{
	Gui_Window_Theme *theme = &gui_manager.window_theme;
	s32 rect_padding = rects < 0 ? theme->rects_padding : rects;
	s32 vertical_padding = vertical < 0 ? theme->vertical_padding : vertical;
	s32 horizontal_padding = horizontal < 0 ? theme->horizontal_padding : horizontal;
	Context *context = gui_manager.get_context();
	context->padding = Padding(rect_padding, horizontal_padding, vertical_padding);
}

void gui::set_theme(Gui_Tab_Theme *gui_tab_theme)
{
	gui_manager.backup_tab_theme = gui_manager.tab_theme;
	gui_manager.tab_theme = *gui_tab_theme;
}

void gui::set_theme(Gui_Tree_Theme *gui_tree_theme)
{
	gui_manager.backup_tree_themes.push(gui_manager.tree_theme);
	gui_manager.tree_theme = *gui_tree_theme;
}

void gui::set_theme(Gui_List_Theme *gui_list_theme)
{
	gui_manager.backup_list_theme = gui_manager.list_theme;
	gui_manager.list_theme = *gui_list_theme;
}

void gui::reset_window_theme()
{
	assert(gui_manager.backup_window_themes.count > 0);

	gui_manager.window_theme = gui_manager.backup_window_themes.last();
	gui_manager.backup_window_themes.pop();
}

void gui::reset_button_theme()
{
	gui_manager.button_theme = Gui_Text_Button_Theme();
}

void gui::reset_image_button_theme()
{
	gui_manager.image_button_theme = Gui_Image_Button_Theme();
}

void gui::reset_edit_field_theme()
{
	gui_manager.edit_field_theme = gui_manager.backup_edit_field_theme;
}

void gui::reset_tab_theme()
{
	gui_manager.tab_theme = gui_manager.backup_tab_theme;
}

void gui::reset_tree_theme()
{
	gui_manager.tree_theme = gui_manager.backup_tree_themes.pop();
}

void gui::reset_list_theme()
{
	gui_manager.list_theme = gui_manager.backup_list_theme;
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
		Gui_Window *window = gui_manager.get_window_by_index(&gui_manager.windows_order, i);
		if (detect_intersection(&window->rect)) {
			return true;
		}
	}
	return false;
}
