#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "editor.h"
#include "../libs/color.h"
#include "../libs/os/input.h"
#include "../sys/sys_local.h"
#include "../win32/win_time.h"
#include "../win32/win_local.h"
#include "../render/font.h"

Editor editor;

static Window_Theme window_theme;
static Button_Theme button_theme;
static List_Box_Theme list_box_theme;
static Input_Filed_Theme edit_field_theme;

struct Callback {
	virtual void call() = 0;
};

template< typename T>
struct Member_Callback : Callback {
	Member_Callback(T *object, void (T::*callback)()) : object(object), member(callback) {}

	T *object;
	void (T::*member)();

	void call() { (object->*member)(); }
};

struct Function_Callback : Callback {

	Function_Callback(void(*callback)()) : callback(callback) {}
	void(*callback)();

	void call() { (*callback)(); }
};

template <typename T>
inline Callback *make_callback(T *object, void (T::*callback)())
{
	return new Member_Callback(object, callback);
}

static int compare_list_boxies(const void *first, const void *second)
{
	assert(first);
	assert(second);

	const List_Box *first_list_box = static_cast<const List_Box *>(first);
	const List_Box *second_list_box = static_cast<const List_Box *>(second);

	return first_list_box->y > second_list_box->y;
}

static inline bool detect_collision(Mouse_Info *mouse, Element *element)
{
	if (mouse->x > element->x && mouse->x < (element->x + element->width) &&
		mouse->y > element->y && mouse->y < (element->y + element->height)) {
		return true;
	}
	return false;
}

static inline float calculate_scale_based_on_percent_from_element_height(int header_height, int image_height, float percents)
{
	float needed_height = header_height * percents / 100.0f;
	float ratios = needed_height / image_height;
	return ratios;
}

static inline u32 get_width_from_bitmap(const ID2D1Bitmap *bitmap)
{
	D2D1_SIZE_U size = bitmap->GetPixelSize();
	return size.width;
}

static inline u32 get_height_from_bitmap(const ID2D1Bitmap *bitmap)
{
	D2D1_SIZE_U size = bitmap->GetPixelSize();
	return size.height;
}

static inline int place_in_middle(Element *dest, int height)
{
	return (dest->y + dest->height / 2) - (height / 2);
}

static inline int place_in_middle(Element *source, Element *dest)
{
	return (dest->y + dest->height / 2) - (source->height / 2);
}

Label::Label(int _x, int _y, const char *_text) : Element(_x, _y)
{
	text = _text;
}

void Label::draw()
{
	direct2d.draw_text(x, y, text);
}

Caret::Caret(float x, float y, float height)
{
	use_float_rect = true;

	fx = x;
	fy = y;
	fwidth = 1;
	fheight = height;
}

void Caret::draw()
{
	static bool show = true;
	static s32 show_time = blink_time;
	static s32 hidding_time = blink_time;
	static s32 show_time_accumulator = 0;
	static s32 hidding_time_accumulator = 0;

	static s64 current_time = 0;
	static s64 last_time = 0;

	current_time = milliseconds_counter();

	s64 elapsed_time = current_time - last_time;

	if (show) {
		show_time_accumulator += elapsed_time;
		if (show_time_accumulator < show_time) {
			if (use_float_rect) {
				direct2d.fill_rect(fx, fy, fwidth, fheight, Color::White);
			} else {
				direct2d.fill_rect(x, y, width, height, Color::White);
			}
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
}

Button::Button(const char *_text, int _x, int _y, int _width, int _height, Button_Theme *_button_theme)
{
	type = ELEMENT_TYPE_BUTTON;
	x = _x;
	y = _y;
	text = _text;

	if (_width == 0) {
		_width = direct_write.get_text_width(_text);
	}
	width = _width;

	if (_height == 0) {
		_height = direct_write.glyph_height;
	}
	height = _height;

	if (_button_theme) {
		theme = *_button_theme;
	} else {
		theme = button_theme;
	}

	text_position = Point(x + (theme.border_about_text / 2) + theme.text_shift, y + (theme.border_about_text / 2));
	
	width += theme.border_about_text;
	height += theme.border_about_text;
}

Button::Button(int _x, int _y, ID2D1Bitmap *_image, float _scale) : Element(_x, _y)
{
	assert(_image);

	D2D1_SIZE_U size = _image->GetPixelSize();
	width = size.width * _scale;
	height = size.height * _scale;
	image = _image;
	scale = _scale;
	type = ELEMENT_TYPE_BUTTON;
	theme = button_theme;
}

Button::~Button()
{
	RELEASE_COM(image);
	DELETE_PTR(callback);
}

void Button::handle_event(Event *event)
{
	if (event->type == EVENT_TYPE_MOUSE) {
		if (detect_collision(&event->mouse_info, this)) {
			flags |= ELEMENT_HOVER;
		} else {
			flags &= ~ELEMENT_HOVER;
		}
	} else if (event->type == EVENT_TYPE_KEY) {
		if ((flags & ELEMENT_HOVER) && was_click_by_left_mouse_button()) {
			if (callback) {
				callback->call();
			}
		}
	}
}

void Button::draw()
{
	if (image) {
		direct2d.draw_bitmap(x, y, width, height, image, scale);
		return;
	}

	if (flags & ELEMENT_HOVER) {
		direct2d.draw_rounded_rect(x, y, width, height, theme.rounded_border, theme.rounded_border, theme.hover_color);
	} else {
		direct2d.draw_rounded_rect(x, y, width, height, theme.rounded_border, theme.rounded_border, theme.color);
	}

	if (!text.is_empty()) {
		direct2d.draw_text(text_position.x, text_position.y, text);
	}
}

List_Box::List_Box(int _x, int _y, const char *_label) : Input_Field(_x, _y, list_box_theme.header_width, list_box_theme.header_height)
{
	type = ELEMENT_TYPE_LIST_BOX;
	theme = list_box_theme;
	x = _x + direct_write.get_text_width(_label) + theme.list_box_shift_from_text;
	y = _y;
	width = theme.header_width;
	height = theme.header_height;

	label = Label(_x, place_in_middle(this, direct_write.glyph_height), _label);

	button_theme.rounded_border = 2.0f;
	button_theme.border_about_text = 0;
	button_theme.color = Color(74, 82, 90);
	button_theme.text_shift = 2;

	list_box_size = 0;

	ID2D1Bitmap *down_image = NULL;

	load_bitmap_from_file("D:\\dev\\Hades-Engine\\data\\editor\\down.png", 1, 1, &down_image);
	float cross_scale_factor = calculate_scale_based_on_percent_from_element_height(height, get_height_from_bitmap(down_image), 100);
	D2D1_SIZE_U size = down_image->GetPixelSize();

	drop_button = new Button(x + width - (size.width * cross_scale_factor), y, down_image, cross_scale_factor);
	drop_button->callback = new Member_Callback<List_Box>(this, &List_Box::on_drop_button_click);

	text_y = place_in_middle(this, direct_write.glyph_height);
	text_x = x + 2;
}

List_Box::List_Box(int x, int y, Array<String> *items, const char *l) : Input_Field(x, y, list_box_theme.header_width, list_box_theme.header_height)
{
	assert(items);
	assert(items->count > 1);

	theme = list_box_theme;

	type = ELEMENT_TYPE_LIST_BOX;

	button_theme.rounded_border = 2.0f;
	button_theme.border_about_text = 0;
	button_theme.color = Color(74, 82, 90);
	button_theme.text_shift = 2;

	for (int i = 0; i < items->count; i++) {
		Button *button = new Button(items->at(i), x, (y + 2 + height) + (direct_write.glyph_height * i), width, direct_write.glyph_height, &button_theme);
		button->callback = new Member_Callback<List_Box>(this, &List_Box::on_list_item_click);
		list_items.push(button);
	}

	current_chosen_item_text = &list_items[0]->text;

	list_box_size = direct_write.glyph_height * items->count;

	ID2D1Bitmap *down_image = NULL;

	load_bitmap_from_file("D:\\dev\\Hades-Engine\\data\\editor\\down.png", 1, 1, &down_image);
	float cross_scale_factor = calculate_scale_based_on_percent_from_element_height(height, get_height_from_bitmap(down_image), 100);
	D2D1_SIZE_U size = down_image->GetPixelSize();

	drop_button = new Button(x + width - (size.width * cross_scale_factor), y, down_image, cross_scale_factor);
	drop_button->callback = new Member_Callback<List_Box>(this, &List_Box::on_drop_button_click);

	text_y = place_in_middle(this, direct_write.glyph_height);
	text_x = x + 2;

}

List_Box::~List_Box()
{
	DELETE_PTR(drop_button);
	
	Button *button = NULL;
	FOR(list_items, button) {
		delete button;
	}
}

void List_Box::push_item(const char *item_text)
{
	Button *button = new Button(item_text, x, (y + 2 + height) + (direct_write.glyph_height * list_items.count), width, direct_write.glyph_height, &button_theme);
	button->callback = new Member_Callback<List_Box>(this, &List_Box::on_list_item_click);
	list_items.push(button);

	list_box_size += direct_write.glyph_height;

	if (list_items.count == 1) {
		current_chosen_item_text = &list_items[0]->text;
	}
}

void List_Box::handle_event(Event *event)
{
	drop_button->handle_event(event);

	if (list_state == LIST_BOX_IS_DROPPED) {
		Button *b = NULL;
		FOR(list_items, b) {
			button = b;
			b->handle_event(event);
		}
	}
}

void List_Box::on_drop_button_click()
{
	if (list_state == LIST_BOX_IS_PICKED_UP) {
		list_state = LIST_BOX_IS_DROPPED;
	} else if (list_state == LIST_BOX_IS_DROPPED) {
		list_state = LIST_BOX_IS_PICKED_UP;
	}
}

void List_Box::on_list_item_click()
{
	if (button) {
		button->flags &= ~ELEMENT_HOVER;
		list_state = LIST_BOX_IS_PICKED_UP;
		current_chosen_item_text = &button->text;
	}
}

void List_Box::draw()
{
	label.draw();
	direct2d.draw_rounded_rect(x, y, width, height, theme.rounded_border, theme.rounded_border, theme.color);
	
	if (current_chosen_item_text) {
		direct2d.draw_text(text_x, text_y, current_chosen_item_text->to_str());
	} else {
		direct2d.draw_text(text_x, text_y, "There is no added items");
	}
	
	drop_button->draw();

	if (list_state == LIST_BOX_IS_DROPPED) {
		direct2d.draw_rounded_rect(x, y + 2 + height, width, list_box_size, theme.rounded_border, theme.rounded_border, theme.color);
		for (int i = 0; i < list_items.count; i++) {
			list_items[i]->draw();
		}
	}
}

Edit_Field::Edit_Field(int _x, int _y, const char *_label_text, Edit_Data_Type _edit_data_type)
{
	int caret_height = (int)(edit_field_theme.height * edit_field_theme.caret_height_in_percents / 100.0f);
	
	type = ELEMENT_TYPE_EDIT_FIELD;

	x = _x + direct_write.get_text_width(_label_text) + edit_field_theme.field_shift_from_text;
	y = _y;
	width = edit_field_theme.width;
	height = edit_field_theme.height;

	max_text_width = (width - edit_field_theme.shift_caret_from_left * 3);

	edit_data_type = _edit_data_type;

	label = Label(_x, place_in_middle(this, direct_write.glyph_height), _label_text);

	theme = edit_field_theme;


	caret = Caret(x + theme.shift_caret_from_left, place_in_middle(this, caret_height), caret_height);


	if (edit_data_type == EDIT_DATA_INT) {
		text.append('0');;

		Direct_Character character = direct_write.characters['0'];
		caret.fx += character.width;

		caret_index_in_text += 1;
		text_width = character.width;
	} else if (edit_data_type == EDIT_DATA_FLOAT) {
		text.append('0');
		text.append('.');
		text.append('0');

		D2D1_SIZE_F size = direct_write.get_text_size_in_pixels("0.0");
		caret.fx += size.width;

		caret_index_in_text += 3;
		text_width = size.height;

	}
}

void Edit_Field::handle_event(Event *event)
{
	if (event->type == EVENT_TYPE_MOUSE) {
		if (detect_collision(&event->mouse_info, this)) {
			flags |= ELEMENT_HOVER;
		} else {
			flags &= ~ELEMENT_HOVER;
		}
	} else if (event->type == EVENT_TYPE_KEY) {
		if (flags & ELEMENT_HOVER) {
			if (was_click_by_left_mouse_button()) {
				flags |= ELEMENT_FOCUSED;
			}
		} else {
			if (was_click_by_left_mouse_button()) {
				flags &= ~ELEMENT_FOCUSED;
			}
		}
		if (flags & ELEMENT_FOCUSED) {
			if (event->is_key_down(VK_BACK)) {

				D2D1_SIZE_F s = direct_write.get_text_size_in_pixels(text);
				int w = s.width;
				text.pop_char();

				D2D1_SIZE_F size = direct_write.get_text_size_in_pixels(text);
				text_width = size.width;
				caret.fx = x + theme.shift_caret_from_left + size.width + caret.fwidth;

			} else if (event->is_key_down(VK_LEFT)) {
				if (caret_index_in_text > -1) {
					char c = text.data[caret_index_in_text];
					Direct_Character character = direct_write.characters[c];

					caret.fx -= character.width;
					caret_index_in_text -= 1;
				}
			} else if (event->is_key_down(VK_RIGHT)) {
				if (caret_index_in_text < text.len) {
					caret_index_in_text += 1;

					char c = text.data[caret_index_in_text];
					Direct_Character character = direct_write.characters[c];

					caret.fx += character.width;
				}
			}
		}
	} else if (event->type == EVENT_TYPE_CHAR) {
		if (flags & ELEMENT_FOCUSED) {
			if ((max_text_width > text_width) && (isalnum(event->char_key) || isspace(event->char_key) || (event->char_key == '.'))) {
				text.append(event->char_key);
				caret_index_in_text += 1;

				D2D1_SIZE_F size = direct_write.get_text_size_in_pixels(text);
				text_width = size.width;
				caret.fx = x + theme.shift_caret_from_left + size.width + caret.fwidth;
			}
		}
	}
}

void Edit_Field::draw()
{
	label.draw();
	direct2d.draw_rounded_rect(x, y, width, height, theme.rounded_border, theme.rounded_border, theme.color);

	if (flags & ELEMENT_FOCUSED) {
		caret.draw();
	}

	if (!text.is_empty()) {
		direct2d.draw_text(x + theme.shift_caret_from_left, place_in_middle(this, direct_write.glyph_height), text);
	}
}

Window::Window()
{
	type = ELEMENT_TYPE_WINDOW;
	next_place.x = window_theme.shift_element_from_left;
	next_place.y = window_theme.header_height + 20;
}

Window::Window(int x, int y, int width, int height) : Element(x, y, width, height + window_theme.header_height)
{
	type = ELEMENT_TYPE_WINDOW;
	make_header();
}

Window::~Window()
{
	Element *element = NULL;
	FOR(elements, element) {
		delete element;
	}

	List_Box *list_box = NULL;
	FOR(list_boxies, list_box) {
		delete list_box;
	}

	DELETE_PTR(close_button);
}

void Window::add_element(Element *element)
{
	go_next_element_place(element);

	if ((element->type == ELEMENT_TYPE_LIST_BOX) || (element->type == ELEMENT_TYPE_EDIT_FIELD)) {
		Input_Field *input_field = static_cast<Input_Field *>(element);
		input_fields.push(input_field);
	}

	if (element->type == ELEMENT_TYPE_LIST_BOX) {
		List_Box *list_box = static_cast<List_Box *>(element);
		list_boxies.push(list_box);

		if (list_boxies.count > 1) {
			qsort(list_boxies.items, list_boxies.count, sizeof(list_boxies[0]), compare_list_boxies);
		}
		return;
	}

	elements.push(element);
}

void Window::make_header()
{
	ID2D1Bitmap *cross_image = NULL;
	load_bitmap_from_file("D:\\dev\\Hades-Engine\\data\\editor\\cross.png", 1, 1, &cross_image);

	float cross_scale_factor = calculate_scale_based_on_percent_from_element_height(window_theme.header_height, get_height_from_bitmap(cross_image), 70);

	D2D1_SIZE_U size = cross_image->GetPixelSize();

	int button_x = x + width - (size.width * cross_scale_factor) - window_theme.shift_cross_button_from_left_on;
	int button_y = (y + window_theme.header_height / 2) - ((size.height * cross_scale_factor) / 2);

	close_button = new Button(button_x, button_y, cross_image, cross_scale_factor);
	close_button->callback = new Member_Callback<Window>(this, &Window::close);
}

void Window::go_next_element_place(Element *element)
{
	next_place.y += element->height + window_theme.place_between_elements;
}

void Window::aligning_input_fields()
{
	static int max_text_width = 0;

	Input_Field *input_field = NULL;
	FOR(input_fields, input_field) {
		if (max_text_width < input_field->x) {
			max_text_width = input_field->x;
		}
	}

	FOR(input_fields, input_field) {
		int d = max_text_width - input_field->x;
		input_field->x = max_text_width;
		input_field->label.x += d;

		if (input_field->type == ELEMENT_TYPE_EDIT_FIELD) {
			Edit_Field *edit_field = static_cast<Edit_Field *>(input_field);
			edit_field->caret.fx += d;
		}
	}
}

void Window::handle_event(Event *event)
{
	Element *element = NULL;
	FOR(elements, element) {
		element->handle_event(event);
	}

	List_Box *list_box = NULL;
	FOR(list_boxies, list_box) {
		list_box->handle_event(event);
	}
}

void Window::update()
{
}

void Window::draw()
{
	if (close_window) {
		return;
	}

	float factor = window_theme.window_rounded_factor + 2;
	direct2d.draw_rounded_rect(x, y, width, height, factor, factor, window_theme.header_color);
	direct2d.draw_rounded_rect(x, y + window_theme.header_height, width, height - window_theme.header_height, factor, factor, window_theme.color);
	direct2d.fill_rect(x, y + window_theme.header_height, width, 10, window_theme.color);

	close_button->draw();


	Element *element = NULL;
	FOR(elements, element)
	{
		element->draw();
	}

	List_Box *list_box = NULL;
	FOR(list_boxies, list_box)
	{
		list_box->draw();
	}
}

void Editor::add_window(Window *window)
{
	windows.push(window);
}

void Editor::init()
{
	make_window(WINDOW_LEFT | WINDOW_FULL_HEIGHT | WINDOW_AUTO_WIDTH);

	make_list_box("Entity Type");
	add_item("Text 1");
	add_item("Text 2");
	add_item("Text 3");
	add_item("Text 4");
	add_item("Text 5");
	add_item("Text 6");

	make_list_box("Entity Type");
	add_item("Text 01");
	add_item("Text 02");
	add_item("Text 03");
	add_item("Text 04");
	add_item("Text 05");
	add_item("Text 06");

	make_edit_field("Position");
	make_edit_field("X");
	make_edit_field("Y");
	make_edit_field("Z");

	make_button("Button 1");
	make_button("Button 2");
	make_button("Button 3");
	
	current_window->aligning_input_fields();
	
	make_window(WINDOW_RIGHT | WINDOW_FULL_HEIGHT | WINDOW_AUTO_WIDTH);

	make_list_box("Position");
	add_item("Text 11");
	add_item("Text 22");
	add_item("Text 33");
	add_item("Text 44");
	add_item("Text 55");
	add_item("Text 66");

	make_list_box("Position");
	add_item("Text 011");
	add_item("Text 022");
	add_item("Text 033");
	add_item("Text 044");
	add_item("Text 055");
	add_item("Text 066");

	make_edit_field("Position");
	make_edit_field("X");
	make_edit_field("Y");
	make_edit_field("Z");
	
	make_button("Button 11");
	make_button("Button 12");
	make_button("Button 13");

	current_window->aligning_input_fields();
}

void Editor::handle_event(Event * event)
{
	free_camera.handle_event(event);

	Window *window = NULL;
	FOR(windows, window) {
		window->handle_event(event);
	}
}

void Editor::update()
{
	Window *window = NULL;
	FOR(windows, window) {
		window->update();
	}
}

void Editor::draw()
{
	Window *window = NULL;
	FOR(windows, window) {
		window->draw();
	}
}

void Editor::make_window(int flags)
{
	current_window = new Window();
	
	int window_width_in_persents = 20;
	int auto_window_width = win32.window_width * window_width_in_persents / 100;

	int window_width;
	int window_height;

	if (WINDOW_AUTO_WIDTH & flags) {
		window_width = auto_window_width;
	}

	if (WINDOW_FULL_HEIGHT & flags) {
		window_height = win32.window_height;
	}

	if (WINDOW_FULL_WIDTH & flags) {
		window_width = win32.window_width;
	}
	
	if (WINDOW_LEFT & flags) {
		current_window->x = 0;
		current_window->y = 0;
	}

	if (WINDOW_RIGHT & flags) {
		current_window->x = win32.window_width - window_width;
		current_window->y = 0;
	}

	current_window->height = window_height;
	current_window->width = window_width;
	current_window->make_header();

	add_window(current_window);
}

void Editor::make_button(const char * text, Callback * callback)
{
	assert(current_window != NULL);
	
	int x = current_window->x + current_window->next_place.x;
	int y = current_window->y + current_window->next_place.y;
	
	current_button = new Button(text, x, y);
	current_button->callback = callback;

	current_window->add_element(current_button);
}

void Editor::make_list_box(const char *text)
{
	assert(current_window != NULL);

	int x = current_window->x + current_window->next_place.x;
	int y = current_window->y + current_window->next_place.y;
	
	current_list_box = new List_Box(x, y, text);

	current_window->add_element(current_list_box);
}

void Editor::make_edit_field(const char *label)
{
	assert(current_window != NULL);

	int x = current_window->x + current_window->next_place.x;
	int y = current_window->y + current_window->next_place.y;

	current_edit_field = new Edit_Field(x, y, label, EDIT_DATA_FLOAT);

	current_window->add_element(current_edit_field);
}

void Editor::add_item(const char * item_text)
{
	assert(current_list_box != NULL);

	current_list_box->push_item(item_text);
}