#include "gui.h"

#include "../win32/win_local.h"
#include "../sys/sys_local.h"

#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/os/input.h"
#include "../libs/os/event.h"
#include "../libs/math/common.h"
#include "../libs/ds/stack.h"
#include "../libs/ds/hash_table.h"
#include "../libs/ds/linked_list.h"

#include "../render/font.h"
#include "../render/render_system.h"

typedef u32 Gui_ID;

Render_2D *render_2d = get_render_2d();

Color header_color = Color(74, 82, 90);
Color Window_color = Color(36, 39, 43);

const s32 MIN_WINDOW_WIDTH = 20;
const s32 MIN_WINDOW_HEIGHT = 40;
const Size_s32 MIN_WINDOW_SIZE = { 20 , 20 };

static const Rect_s32 default_window_rect = { 50, 50, 300, 300 };

#define GET_RENDER_LIST() (&window->render_list)


inline void place_in_middle_by_y(Rect_s32 *source, Rect_s32 *dest)
{
	assert(dest->height > 2);
	dest->y = source->y + (source->height / 2) - (dest->height / 2);
}

struct Gui_Text_Button_Theme {
	s32 border_about_text = 10;
	s32 rounded_border = 5;
	Color stroke_color = Color::Black;
	Color color = Color(0, 75, 168);
	Color hover_color = Color(0, 60, 168);
};

struct Gui_Window_Theme {
	s32 header_height = 20;
	s32 rounded_border = 6;
	s32 place_between_elements = 10;
	s32 shift_element_from_window_side = 20;
	s32 scroll_bar_width = 15;
	Color header_color = Color::White;
	Color background_color = Color(36, 39, 43);
};

Gui_Window_Theme window_theme;
Gui_Text_Button_Theme button_theme;

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

inline u32 place_in_middle(Rect_s32 *dest, Rect_s32 *source)
{
	return (dest->y + dest->height / 2) - (source->height / 2);
}

enum Alignment {
	LEFT_ALIGNMENT,
	RIGHT_ALIGNMENT
};

enum Place {
	PLACE_HORIZONTALLY,
	PLACE_VERTICALLY,
	PLACE_HORIZONTALLY_AND_IN_MIDDLE,
};

enum Axis {
	X_AXIS = 0,
	Y_AXIS = 1,
};

#define UPDATE_VIEW_RECT(filed, value) (window->view_rect.filed += value)


typedef u32 Element_Alignment;

const Element_Alignment ALIGNMENT_HORIZONTALLY = 0x01;
const Element_Alignment ALIGNMENT_VERTICALLY = 0x02;


struct Gui_Window {
	Gui_ID gui_id;

	Element_Alignment alignment;
	
	Rect_s32 rect;
	Rect_s32 view_rect;
	Rect_s32 content_rect;
	Point_s32 next_place;
	Point_s32 scroll;

	String name;
	
	Array<Gui_Window *> child_windows;
	Render_Primitive_List render_list;

	Point_s32 get_place(Rect_s32 *rect);
	Rect_s32 get_scrollbar_rect(Axis axis);
};

Point_s32 Gui_Window::get_place(Rect_s32 *element_rect)
{
	assert(!((alignment & ALIGNMENT_VERTICALLY) && (alignment & ALIGNMENT_HORIZONTALLY)));

	Point_s32 position = { 0, 0 };
	static bool reset_x_position = false;
	static s32 prev_rect_height = 0;
	static u32 offset = window_theme.place_between_elements;

	if (alignment & ALIGNMENT_VERTICALLY) {
		if (reset_x_position) {
			next_place.x = content_rect.x;
			reset_x_position = false;
			next_place.y += prev_rect_height + offset;
			content_rect.height += prev_rect_height + offset;
		}
		position.y = next_place.y + offset;
		position.x = next_place.x + offset;
		next_place.y += element_rect->height + offset;
		content_rect.height += element_rect->height + offset;
	}

	if (alignment & ALIGNMENT_HORIZONTALLY) {
		reset_x_position = true;
		prev_rect_height = element_rect->height;
		position.x = next_place.x + offset;
		position.y = next_place.y + offset;
		next_place.x += element_rect->width + offset;
		content_rect.width += element_rect->width + offset;
	}
	return position;
}

Rect_s32 Gui_Window::get_scrollbar_rect(Axis axis)
{
	const static s32 scroll_bar_width = 8;
	if (axis == Y_AXIS) {
		return Rect_s32(rect.right() - scroll_bar_width, rect.y, scroll_bar_width, view_rect.height);
	} 
	return Rect_s32(rect.x, rect.bottom() - scroll_bar_width, view_rect.width, scroll_bar_width);
}

struct Gui_Manager {
	s32 mouse_x;
	s32 mouse_y;
	s32 last_mouse_x;
	s32 last_mouse_y;
	s32 mouse_x_delta;
	s32 mouse_y_delta;

	u32 list_box_count;

	Gui_ID active_list_box;
	Gui_ID hot_item;
	Gui_ID active_item;
	Gui_ID resizing_window;
	Gui_ID probably_resizing_window;

	Cursor_Type cursor_type;
	Rect_s32 window_rect;
	
	Stack<Gui_Window *> window_stack;
	Array<Gui_Window *> windows_order;
	Array<Gui_Window> windows;

	Gui_Window_Theme window_theme;

	void init();
	void shutdown();
	
	void new_frame();
	void end_frame();

	void begin_window(const char *name);
	void end_window();
	void same_line();
	void next_line();
	void set_next_window_pos(s32 x, s32 y);
	void set_next_window_size(s32 width, s32 height);

	void list_box(const char *strings[], u32 len, u32 *item_index);
	void scroll_bar(Gui_Window *window, Axis axis, Rect_s32 *scroll_bar);
	bool button(const char *name);
	
	bool check_item(Gui_ID id, Rect_s32 *rect);
	bool detect_collision_window_borders(Rect_s32 *rect, Rect_Side *rect_side);
	bool check_button_state(const char *name, Rect_s32 *rect, bool (*click_callback)() = NULL);
	
	Rect_s32 get_win32_rect();
	
	Gui_Window *get_window();
	Gui_Window *find_window(const char *name);
	Gui_Window *find_window_in_order(const char *name, int *window_index);
	
	Gui_Window *create_window(const char *name);
	Gui_Window *create_window(const char *name, Rect_s32 *rect);
};

void Gui_Manager::set_next_window_pos(s32 x, s32 y)
{
	window_rect.set(x, y);
}

void Gui_Manager::set_next_window_size(s32 width, s32 height)
{
	window_rect.set_size(width, height);
}

Rect_s32 Gui_Manager::get_win32_rect()
{
	return Rect_s32(0, 0, win32.window_width, win32.window_height);
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

Gui_Window *Gui_Manager::create_window(const char *name)
{
	Gui_ID window_id = fast_hash(name);
	
	Gui_Window new_window;
	new_window.gui_id = window_id;
	new_window.rect = window_rect;
	new_window.view_rect = new_window.rect;
	new_window.alignment = 0;
	new_window.alignment |= ALIGNMENT_VERTICALLY;
	new_window.content_rect.set(new_window.rect.x, new_window.rect.y);

	new_window.scroll.x = new_window.rect.x;
	new_window.scroll.y = new_window.rect.y;

	new_window.name = name;
	new_window.render_list.render_2d = render_2d;

	windows.push(new_window);
	window_rect.x += new_window.rect.width + 40;

	return &windows.last_item();
}

Gui_Window *Gui_Manager::create_window(const char *name, Rect_s32 *rect)
{
	Gui_ID window_id = fast_hash(name);

	Gui_Window new_window;
	new_window.gui_id = window_id;
	new_window.rect = *rect;
	new_window.view_rect = new_window.rect;
	new_window.alignment = 0;
	new_window.alignment |= ALIGNMENT_VERTICALLY;
	new_window.content_rect.set(new_window.rect.x, new_window.rect.y);

	new_window.scroll.x = new_window.rect.x;
	new_window.scroll.y = new_window.rect.y;

	new_window.name = name;
	new_window.render_list.render_2d = render_2d;

	windows.push(new_window);
	windows_order.push(&windows.last_item());

	return &windows.last_item();
}

bool Gui_Manager::check_button_state(const char *name, Rect_s32 *rect, bool (*click_callback)())
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

inline void place_in_center(Rect_s32 *in_element_place, Rect_s32 *placed_element)
{
	placed_element->x = ((in_element_place->width / 2) - (placed_element->width / 2)) + in_element_place->x;
	placed_element->y = ((in_element_place->height / 2) - (placed_element->height / 2)) + in_element_place->y;
}

inline Rect_s32 get_text_rect(const char *text)
{
	Rect_s32 text_rect;
	Size_u32 size = font.get_text_size(text);
	text_rect.width = (s32)size.width;
	text_rect.height = (s32)size.height;
	return text_rect;
}

void Gui_Manager::list_box(const char * strings[], u32 len, u32 *item_index)
{
	assert(len > 0);

	if (*item_index >= len) {
		*item_index = 0;
	}

	Rect_s32 list_box_rect = { 0, 0, 120, 20 };
	
	Gui_Window *window = get_window();
	Point_s32 pos = window->get_place(&list_box_rect);
	
	list_box_rect.set(pos.x, pos.y);
	
	String list_box_name = window->name + String("_list_box_") + String((int)list_box_count);
	if (check_button_state(list_box_name, &list_box_rect)) {
		if (active_item == active_list_box) {
			active_list_box = 0;
		} else {
			active_list_box = active_item;
		}
	}

	Gui_ID gui_id = fast_hash(list_box_name);
	if (gui_id == active_list_box) {
		set_next_window_pos(list_box_rect.x, list_box_rect.bottom());
		set_next_window_size(list_box_rect.width, 200);
		begin_window(list_box_name);
		
		for (u32 i = 0; i < len; i++) {
			if (button(strings[i])) {
				*item_index = i;
			}
		}
		end_window();
	}

	if (must_item_be_drawn(&window->rect, &list_box_rect)) {
		Rect_s32 clip_rect = calcualte_clip_rect(&window->rect, &list_box_rect);
		Rect_s32 text_rect = get_text_rect(strings[*item_index]);
		place_in_center(&list_box_rect, &text_rect);
		
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
	Rect_s32 button_rect{ 0, 0, 123, 20 };
	Gui_Window *window = get_window();
	Point_s32 pos = window->get_place(&button_rect);

	button_rect.x = pos.x;
	button_rect.y = pos.y;

	bool button_down = check_button_state(NULL, &button_rect);

	Color button_color = detect_collision(&button_rect) ? button_theme.hover_color : button_theme.color;

	if (must_item_be_drawn(&window->rect, &button_rect)) {
		Rect_s32 clip_rect = calcualte_clip_rect(&window->rect, &button_rect);
		Rect_s32 text_rect = get_text_rect(name);
		place_in_center(&button_rect, &text_rect);

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

void Gui_Manager::init()
{
	String path_to_save_file;
	os_path.build_full_path_to_gui_file("new_gui_data.gui", path_to_save_file);

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

		print("Gui Window");
		print("name ", string);
		print("rect ", &rect);

		create_window(string, &rect);
		free_string(string);
	}
}

void Gui_Manager::shutdown()
{
	String path_to_save_file;
	os_path.build_full_path_to_gui_file("new_gui_data.gui", path_to_save_file);

	File save_file;
	if (!save_file.open(path_to_save_file, FILE_MODE_WRITE, FILE_CREATE_ALWAYS)) {
		print("Gui_Manager::shutdown: Hades gui data can not be save in file from path {}.", path_to_save_file);
		return;
	}

	save_file.write((void *)&windows.count, sizeof(int));

	Gui_Window *window = NULL;
	For(windows, window) {

		save_file.write((void *)&window->name.len, sizeof(int));
		save_file.write((void *)&window->name.data[0], window->name.len);
		save_file.write((void *)&window->rect, sizeof(Rect_s32));
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

void Gui_Manager::begin_window(const char *name)
{
	int window_index = 0;
	
	Gui_Window *window = NULL;
	window = find_window_in_order(name, &window_index);
	
	if (!window) {
		window = create_window(name);
		windows_order.push(window);
	}
	
	window_stack.push(window);

	s32 old_win_y = window->rect.y;
	s32 old_win_x = window->rect.x;
	
	window->next_place.x = window->content_rect.x;
	window->next_place.y = window->content_rect.y;
	window->alignment = 0;
	window->alignment |= ALIGNMENT_VERTICALLY;


	bool became_just_focused = false;
	Rect_s32  *rect = &window->rect;
	if (check_item(window->gui_id, rect) && was_left_mouse_button_just_pressed()) {
		active_item = window->gui_id;
		became_just_focused = true;
	}


	if (became_just_focused) {
		windows_order.remove(window_index);
		windows_order.push(window);
	}

	if (active_item == window->gui_id && is_left_mouse_button_down()) {
		rect->x = math::clamp(rect->x + mouse_x_delta, 0, (s32)win32.window_width - rect->width);
		rect->y = math::clamp(rect->y + mouse_y_delta, 0, (s32)win32.window_height - rect->height);

		window->view_rect.x = rect->x;
		window->view_rect.y = rect->y;

		window->content_rect.x += rect->x - old_win_x;
		window->content_rect.y += rect->y - old_win_y;

		window->next_place.x = window->content_rect.x;
		window->next_place.y = window->content_rect.y;

		window->scroll.x += rect->x - old_win_x;
		window->scroll.y += rect->y - old_win_y;
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
				if ((rect->right() + mouse_x_delta) < win32.window_width) {
					rect->width = math::max(rect->width + mouse_x_delta, MIN_WINDOW_WIDTH);
				}
			}
			if ((rect_side == RECT_SIDE_BOTTOM) || (rect_side == RECT_SIDE_RIGHT_BOTTOM) || (rect_side == RECT_SIDE_LEFT_BOTTOM)) {
				if ((rect->bottom() + mouse_y_delta) < win32.window_height) {
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
	window->view_rect = *rect;


	Render_Primitive_List *render_list = GET_RENDER_LIST();
	render_list->add_outlines(rect->x, rect->y, rect->width, rect->height, Color(92, 100, 107), 2.0f, window_theme.rounded_border);
	render_list->add_rect(&window->rect, window_theme.background_color, window_theme.rounded_border);
}

void Gui_Manager::end_window()
{
	Gui_Window *window = get_window();
	
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

	if ((active_item == scroller_id) || check_button_state(scroller, &scroll_rect, is_left_mouse_button_down)) {

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

String write_filed(const char *field_name, const char *string)
{
	String result;
	result.append(field_name);
	result.append(" ");
	result.append(string);
	
	free_string(string);
	return result;
}

struct Window_Save_Data {
	String name;
	Rect_s32 rect;
};

void gui::init_gui()
{
	gui_manager.init();
}

void gui::shutdown()
{
	gui_manager.shutdown();
}

void list_box(const char *strings[], u32 len, u32 *item_index)
{
	gui_manager.list_box(strings, len, item_index);
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
	gui_manager.begin_window(name);
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

void gui::draw_test_gui()
{

	begin_frame();

	//begin_window("Window1");
	//button("Window1");
	//end_window();

	//begin_window("Window2");
	//button("Window2");
	//end_window();
	
	begin_window("Window3");
	button("Window3");
	end_window();

	
	if (begin_window("Test")) {

		const char *str[] = { "first", "second", "third" };
		static u32 item_index = 123124;
		list_box(str, 3, &item_index);
		if (button("Click")) {
			print("Was click by bottom");
		}
		end_window();
	}

	//	const char *str2[] = { "first2", "second2", "third2" };
	//	static u32 item_index2 = 0;
	//	list_box(str2, 3, &item_index2);
		//same_line();
		//button("same line");
		//button("same line1");
		//button("same line2");
		//button("same line3");
		//button("same line4");
		//next_line();
		//button("next line1");
		//button("next line2");
		//button("next line3");
		//button("next line4");
		//button("next line5");
		//button("next line6");
		//button("next line7");
		//button("next line8");
		//button("next line9");
		//button("next line10");
		//button("next line11");
		//button("next line12");
		//button("next line13");
		//button("next line14");
		//button("next line15");
		//button("next line16");
		//button("next line17");
		//button("next line18");
		//button("next line19");

	//}
	//end_window();

	//set_next_window_pos(500, 300);
	//set_next_window_size(1000, 700);
	
	//begine_window("temp");
	//	if (button("temp buttom")) {
	//		print("temp button was pressed");
	//	}
	//	begine_window("temp2");
	//	if (button("temp buttom2")) {
	//		print("temp button was pressed2");
	//	}
	//	end_window();
	//end_window();

	//begine_window("temp1");
	//end_window();

	end_frame();
}
