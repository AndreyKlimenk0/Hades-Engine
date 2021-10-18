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

#include "../game/entity.h"
#include "../game/world.h"

Editor editor;

static Window_Theme window_theme;
static Button_Theme button_theme;
static List_Box_Theme list_box_theme;
static Edit_Field_Theme edit_field_theme;


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

struct Function_Callback_With_Arg : Callback {
	Function_Callback_With_Arg(void(*callback)(Args *args)) : callback(callback) {}
	void(*callback)(Args *args);
	//Args *args;

	void call() {}
	void call(Args *args) { (*callback)(args); }
};

void editor_make_entity(Args *args)
{
	Entity_Type entity_type;
	args->get("entity_type", (int *)&entity_type);

	Vector3 vec3;
	args->get("position", &vec3);
	
	world.entity_manager.make_entity(entity_type, vec3);
}

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
	static s64 show_time = blink_time;
	static s64 hidding_time = blink_time;
	static s64 show_time_accumulator = 0;
	static s64 hidding_time_accumulator = 0;

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

void Caret::set_position(int _x, int _y)
{
	fx = _x;
	fy = _y;
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

void Button::set_position(int _x, int _y)
{
	x = _x;
	y = _y;
	text_position = Point(x + (theme.border_about_text / 2) + theme.text_shift, y + (theme.border_about_text / 2));
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

List_Box::List_Box(const char *_label, int _x, int _y)
{
	type = ELEMENT_TYPE_LIST_BOX;
	theme = list_box_theme;
	x = _x + direct_write.get_text_width(_label) + theme.list_box_shift_from_text;
	y = _y;
	width = direct_write.get_text_width(_label) + theme.list_box_shift_from_text + theme.header_width;;
	height = theme.header_height;

	header_width = theme.header_width;

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
	
	drop_button_image_width = (size.width * cross_scale_factor);

	drop_button = new Button(x + width - drop_button_image_width, y, down_image, cross_scale_factor);
	drop_button->callback = new Member_Callback<List_Box>(this, &List_Box::on_drop_button_click);

	text_y = place_in_middle(this, direct_write.glyph_height);
	text_x = x + 2;
}


List_Box::~List_Box()
{
	DELETE_PTR(drop_button);
	
	Button *button = NULL;
	For(item_list, button) {
		DELETE_PTR(button);
	}
}

void List_Box::draw()
{
	label.draw();
	
	direct2d.draw_rounded_rect(x, y, header_width, height, theme.rounded_border, theme.rounded_border, theme.color);
	
	if (current_chosen_item_text) {
		direct2d.draw_text(text_x, text_y, current_chosen_item_text->to_str());
	} else {
		direct2d.draw_text(text_x, text_y, "There is no added items");
	}

	drop_button->draw();

	if (list_state == LIST_BOX_IS_DROPPED) {
		direct2d.draw_rounded_rect(x, y + 2 + height, header_width, list_box_size, theme.rounded_border, theme.rounded_border, theme.color);
		for (int i = 0; i < item_list.count; i++) {
			item_list[i]->draw();
		}
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

void List_Box::on_drop_button_click()
{
	if (list_state == LIST_BOX_IS_PICKED_UP) {
		list_state = LIST_BOX_IS_DROPPED;
	} else if (list_state == LIST_BOX_IS_DROPPED) {
		list_state = LIST_BOX_IS_PICKED_UP;
	}
}

void List_Box::handle_event(Event *event)
{
	drop_button->handle_event(event);

	if (list_state == LIST_BOX_IS_DROPPED) {
		Button *b = NULL;
		For(item_list, b) {
			button = b;
			b->handle_event(event);
		}
	}
}

void List_Box::add_item(const char *item_text)
{
	Button *button = new Button(item_text, x, (y + 2 + height) + (direct_write.glyph_height * item_list.count), header_width, direct_write.glyph_height, &button_theme);
	button->callback = new Member_Callback<List_Box>(this, &List_Box::on_list_item_click);
	item_list.push(button);

	list_box_size += direct_write.glyph_height;

	if (item_list.count == 1) {
		current_chosen_item_text = &item_list[0]->text;
	}
}

void List_Box::add_item(const char * string, int enum_value)
{
	string_enum_pairs.set(string, enum_value);
	add_item(string);
}

int List_Box::get_current_chosen_enum_value()
{
	return string_enum_pairs[*current_chosen_item_text];
}

String *List_Box::get_current_chosen_item_string()
{
	return current_chosen_item_text;
}


void List_Box::set_position(int _x, int _y)
{
	x = _x + direct_write.get_text_width(label.text) + theme.list_box_shift_from_text;
	y = _y;

	drop_button->set_position(x + header_width - drop_button_image_width, _y);

	label.set_position(_x, place_in_middle(this, direct_write.glyph_height));

	text_y = place_in_middle(this, direct_write.glyph_height);
	text_x = x + 2;

	for (int i = 0; i < item_list.count; i++) {
		item_list[i]->set_position(x, (y + 2 + height) + (direct_write.glyph_height * i));
	}
}

Picked_Panel_List_Box::Picked_Panel_List_Box(int _x, int _y, const char *_label) : List_Box(_label, _x, _y) 
{
	type = ELEMENT_TYPE_PICKED_PANEL_LIST_BOX;
}

void Picked_Panel_List_Box::on_list_item_click()
{
	List_Box::on_list_item_click();

	Picked_Panel *current_picked_panel = string_picked_panel_pairs[current_chosen_item_text];
	if (current_picked_panel == last_picked_panel) {
		return;
	}
	current_picked_panel->draw_panel = true;
	last_picked_panel->draw_panel = false;
	last_picked_panel = current_picked_panel;
}

void Picked_Panel_List_Box::add_item(const char *string, int enum_value, Picked_Panel *picked_panel)
{
	assert(picked_panel);

	if (item_list.is_empty()) {
		last_picked_panel = picked_panel;
		last_picked_panel->draw_panel = true;
	}
	
	List_Box::add_item(string, enum_value);
	
	Button *button = item_list.last_item();
	DELETE_PTR(button->callback);
	button->callback = new Member_Callback<Picked_Panel_List_Box>(this, &Picked_Panel_List_Box::on_list_item_click);

	string_picked_panel_pairs.set(string, picked_panel);
}

Picked_Panel::~Picked_Panel()
{
	Input_Field *input_field = NULL;
	For(input_fields, input_field) {
		DELETE_PTR(input_field);
	}
}

Picked_Panel *Picked_Panel_List_Box::get_picked_panel()
{
	return string_picked_panel_pairs[current_chosen_item_text];
}

void Picked_Panel::draw()
{
	if (!draw_panel) {
		return;
	}

	Input_Field *input_field = NULL;
	For(input_fields, input_field) {
		if (input_field->type == ELEMENT_TYPE_LIST_BOX) {
			continue;
		}
		input_field->draw();
	}

	List_Box *list_box = NULL;
	For(list_boxies, list_box) {
		list_box->draw();
	}
}

void Picked_Panel::handle_event(Event * event)
{
	if (!draw_panel) {
		return;
	}

	Input_Field *input_field = NULL;
	For(input_fields, input_field) {
		input_field->handle_event(event);
	}
}

void Picked_Panel::add_field(Input_Field *input_field)
{
	if (input_field->type == ELEMENT_TYPE_LIST_BOX) {
		list_boxies.push(static_cast<List_Box *>(input_field));
	}
	input_fields.push(input_field);
}

void Picked_Panel::set_position(int _x, int _y)
{
	assert(false);
}

Edit_Field::Edit_Field(const char *_label_text, Edit_Data_Type _edit_data_type, int _x, int _y)
{
	int caret_height = (int)(edit_field_theme.height * edit_field_theme.caret_height_in_percents / 100.0f);
	
	type = ELEMENT_TYPE_EDIT_FIELD;
	
	theme = edit_field_theme;

	x = _x + direct_write.get_text_width(_label_text) + theme.field_shift_from_text;
	y = _y;
	width = theme.width + direct_write.get_text_width(_label_text) + theme.field_shift_from_text;;
	height = theme.height;

	field_width = theme.width;

	edit_itself_data = true;

	max_text_width = (field_width - theme.shift_caret_from_left * 3);

	edit_data_type = _edit_data_type;

	label = Label(_x, place_in_middle(this, direct_write.glyph_height), _label_text);

	caret = Caret(x + theme.shift_caret_from_left, place_in_middle(this, caret_height), caret_height);


	if (edit_data_type == EDIT_DATA_INT) {
		text.append('0');;

		Direct_Character character = direct_write.characters['0'];
		caret.fx += character.width;

		caret_index_in_text += 1;
		text_width = character.width;

		edit_data.itself_data.int_value = 0;

	} else if (edit_data_type == EDIT_DATA_FLOAT) {
		text.append("0.0");

		D2D1_SIZE_F size = direct_write.get_text_size_in_pixels("0.0");
		caret.fx += size.width;

		caret_index_in_text += 3;
		text_width = size.height;

		edit_data.itself_data.float_value = 0.0f;
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

				text_width = direct_write.get_text_width(text);
				caret.fx = x + theme.shift_caret_from_left + text_width + caret.fwidth;

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

				text_width = direct_write.get_text_width(text);
				caret.fx = x + theme.shift_caret_from_left + text_width + caret.fwidth;

				if (edit_itself_data) {
					if (edit_data_type == EDIT_DATA_INT) {
						
						edit_data.itself_data.int_value = atoi(text);

					} else if (edit_data_type == EDIT_DATA_FLOAT) {
						
						edit_data.itself_data.float_value = atof(text);
					}
				} else {
					if (edit_data_type == EDIT_DATA_INT) {

						*edit_data.not_itself_data.int_value = atoi(text);

					} else if (edit_data_type == EDIT_DATA_FLOAT) {
						
						*edit_data.not_itself_data.float_value = atof(text);
					}
				}
			}
		}
	}
}

void Edit_Field::draw()
{
	label.draw();
	direct2d.draw_rounded_rect(x, y, field_width, height, theme.rounded_border, theme.rounded_border, theme.color);

	if (flags & ELEMENT_FOCUSED) {
		caret.draw();
	}

	if (!text.is_empty()) {
		direct2d.draw_text(x + theme.shift_caret_from_left, place_in_middle(this, direct_write.glyph_height), text);
	}
}

void Edit_Field::set_position(int _x, int _y)
{
	int caret_height = (int)(theme.height * theme.caret_height_in_percents / 100.0f);

	x = _x + direct_write.get_text_width(label.text) + theme.field_shift_from_text;
	y = _y;

	label.set_position(_x, place_in_middle(this, direct_write.glyph_height));
	caret.set_position(x + theme.shift_caret_from_left, place_in_middle(this, caret_height));

	int text_width = direct_write.get_text_width(text);
	caret.fx = x + theme.shift_caret_from_left + text_width + caret.fwidth;
}

Vector3_Edit_Field::Vector3_Edit_Field(int _x, int _y)
{
	type = ELEMENT_TYPE_VECTOR3_EDIT_FIELD;

	label = Label(0, 0, "Position");

	x = Edit_Field("x", EDIT_DATA_FLOAT);
	y = Edit_Field("y", EDIT_DATA_FLOAT);
	z = Edit_Field("z", EDIT_DATA_FLOAT);
}

void Vector3_Edit_Field::draw()
{
	x.draw();
	y.draw();
	z.draw();
}

void Vector3_Edit_Field::handle_event(Event * event)
{
	x.handle_event(event);
	y.handle_event(event);
	z.handle_event(event);
}

Form::Form()
{
	type = ELEMENT_TYPE_FORM;
	submit = new Button("Submit");
}

//Form::~Form()
//{
//	Input_Field *input_field = NULL;
//	FOR(input_fields, input_field) {
//		delete input_field;
//	}
//}

void Form::draw()
{
	submit->draw();

	//Input_Field *input_field = NULL;
	//FOR(input_fields, input_field) {
	//	input_field->draw();

	//}
}

void Form::add_field(Input_Field *input_field)
{
	input_fields.push(input_field);
		
	int x = editor.current_window->x + editor.current_window->next_place.x;
	int y = editor.current_window->y + editor.current_window->next_place.y;

	submit->set_position(50, 300);
}

void Form::on_submit()
{
	Args args;

	Input_Field *input_field = NULL;
	For(input_fields, input_field) {
		String str = input_field->label.text;
		str.to_lower();
		str.replace(' ', '_');

		if (input_field->type == ELEMENT_TYPE_LIST_BOX) {
		
			List_Box *list_box = static_cast<List_Box *>(input_field);
			args.set(str, list_box->get_current_chosen_enum_value());
		
		} else if (input_field->type == ELEMENT_TYPE_VECTOR3_EDIT_FIELD) {
			
			Vector3_Edit_Field *vec3_edit_field = static_cast<Vector3_Edit_Field *>(input_field);
			Vector3 vec3 = Vector3(vec3_edit_field->x.edit_data.itself_data.float_value, vec3_edit_field->y.edit_data.itself_data.float_value, vec3_edit_field->z.edit_data.itself_data.float_value);
			args.set(str, &vec3);
		}
	}

	if (callback) {
		Function_Callback_With_Arg *args_callback = static_cast<Function_Callback_With_Arg *>(callback);
		args_callback->call(&args);
	}
}

void Form::handle_event(Event * event)
{
	submit->handle_event(event);

	//Input_Field *input_field = NULL;
	//FOR(input_fields, input_field) {
	//	input_field->handle_event(event);
	//}
}

Window::Window()
{
	type = ELEMENT_TYPE_WINDOW;
	next_place.x = window_theme.shift_element_from_left;
	next_place.y = window_theme.header_height + 20;
	theme = window_theme;
}

Window::Window(int x, int y, int width, int height) : Element(x, y, width, height + window_theme.header_height)
{
	type = ELEMENT_TYPE_WINDOW;
	make_header();
}

Window::~Window()
{
	Element *element = NULL;
	For(elements, element) {
		delete element;
	}

	List_Box *list_box = NULL;
	For(list_boxies, list_box) {
		delete list_box;
	}

	DELETE_PTR(close_button);
}

void Window::add_element(Element *element)
{
	set_element_position(element);

	if ((element->type == ELEMENT_TYPE_LIST_BOX) || (element->type == ELEMENT_TYPE_EDIT_FIELD)) {
		Input_Field *input_field = static_cast<Input_Field *>(element);
		input_fields.push(input_field);
	}

	if (element->type == ELEMENT_TYPE_VECTOR3_EDIT_FIELD) {
		Vector3_Edit_Field *vec3 = static_cast<Vector3_Edit_Field *>(element);
		input_fields.push(&vec3->x);
		input_fields.push(&vec3->y);
		input_fields.push(&vec3->z);
	}

	if ((element->type == ELEMENT_TYPE_LIST_BOX) || (element->type == ELEMENT_TYPE_PICKED_PANEL_LIST_BOX)) {
		List_Box *list_box = static_cast<List_Box *>(element);
		list_boxies.push(list_box);

		if (list_boxies.count > 1) {
			qsort(list_boxies.items, list_boxies.count, sizeof(list_boxies[0]), compare_list_boxies);
		}
		return;
	}

	if (element->type == ELEMENT_TYPE_FORM) {
		Form *form = static_cast<Form *>(element);
		forms.push(form);
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

#define CALCULATE_PLACE_BY_X(element) (x + width - element->width - theme.shift_element_from_left_side)

void Window::set_element_position(Element *element)
{
	int _x = CALCULATE_PLACE_BY_X(element);
	
	if (element->type == ELEMENT_TYPE_PICKED_PANEL) {
		int temp_storage = next_place.y;
		
		Picked_Panel *draw_panel = static_cast<Picked_Panel *>(element);

		Input_Field *input_field = NULL;
		For(draw_panel->input_fields, input_field) {
			set_element_position(input_field);
		}
		
		next_place.y = temp_storage;
	} else {
		element->set_position(_x, next_place.y);
		go_next_element_place(element);
	}
}

void Window::aligning_input_fields()
{
	static int max_text_width = 0;
	static int shift_from_left_side = 20;

	Input_Field *input_field = NULL;
	For(input_fields, input_field) {
		if (max_text_width < input_field->x) {
			max_text_width = input_field->x;
		}
	}

	For(input_fields, input_field) {
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
	if (detect_collision(&event->mouse_info, this)) {
		flags |= ELEMENT_HOVER;
	} else {
		flags &= ~ELEMENT_HOVER;
	}
	
	Element *element = NULL;
	For(elements, element) {
		element->handle_event(event);
	}

	List_Box *list_box = NULL;
	For(list_boxies, list_box) {
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
	For(elements, element) {
		element->draw();
	}

	List_Box *list_box = NULL;
	For(list_boxies, list_box) {
		list_box->draw();
	}
}

List_Box *Window::find_list_box(const char *label)
{
	List_Box *list_box = NULL;
	For(list_boxies, list_box) {
		if (list_box->label.text == label) {
			return list_box;
		}
	}
	return NULL;
}

Form *Window::find_form(const char *label)
{
	Form *form = NULL;
	For(forms, form) {
		if (form->label.is_empty()) {
			continue;
		}

		if (form->label == label) {
			return form;
		}
	}
	return NULL;
}

void Editor::add_window(Window *window)
{
	windows.push(window);
}

void Editor::handle_event(Event * event)
{
	bool window_hover = false;

	Window *window = NULL;
	For(windows, window) {
		window->handle_event(event);
		if (window->flags & ELEMENT_HOVER) {
			window_hover = true;
		}
	}

	if (!window_hover) {
		free_camera.handle_event(event);
	}
}

void Editor::update()
{
	Window *window = NULL;
	For(windows, window) {
		window->update();
	}
}

void Editor::draw()
{
	Window *window = NULL;
	For(windows, window) {
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
	
	current_button = new Button(text);
	current_button->callback = callback;

	current_window->add_element(current_button);
}

void Editor::make_list_box(const char *label)
{
	assert(current_window != NULL);
	
	current_list_box = new List_Box(label);


	if (current_picked_panel) {
		current_picked_panel->add_field(current_list_box);
	} 
	
	if (current_form) {
		current_form->add_field(current_list_box);
	}
	
	if ((!current_picked_panel) && (!current_form)) {
		current_window->add_element(current_list_box);
	}
}

void Editor::make_picked_list_box(const char * label)
{
	assert(current_window != NULL);

	int x = current_window->get_element_x_place();
	int y = current_window->get_element_y_place();

	current_picked_panel_list_box = new Picked_Panel_List_Box(x, y, label);

	if (current_form) {
		current_form->add_field(current_picked_panel_list_box);
	}

	current_window->add_element(current_picked_panel_list_box);
}

void Editor::make_picked_panel()
{
	assert(current_window != NULL);

	current_picked_panel = new Picked_Panel();
}

void Editor::end_picked_panel()
{
	assert(current_window != NULL);
	assert(current_picked_panel != NULL);

	current_window->add_element(current_picked_panel);
	current_picked_panel = NULL;
}

void Editor::make_vector3_edit_field()
{
	assert(current_window != NULL);

	int x = current_window->get_element_x_place();
	int y = current_window->get_element_y_place();

	Vector3_Edit_Field *vec3 = new Vector3_Edit_Field(x, y);

	current_window->add_element(vec3);
	
	if (current_form) {
		current_form->add_field(vec3);
	}
}

void Editor::make_edit_field(const char *label, Edit_Data_Type edit_data_type)
{
	assert(current_window != NULL);

	current_edit_field = new Edit_Field(label, edit_data_type);

	if (current_picked_panel) {
		current_picked_panel->add_field(current_edit_field);
	}
	
	if (current_form) {
		current_form->add_field(current_edit_field);
	}

	if ((!current_picked_panel) && (!current_form)) {
		current_window->add_element(current_edit_field);
	}
}

void Editor::make_edit_field(const char * label, int value)
{
}

void Editor::make_edit_field(const char * label, float value)
{
}

void Editor::make_form()
{
	current_form = new Form();
	current_form->submit->callback = new Member_Callback<Form>(current_form, &Form::on_submit);
	current_form->callback = new Function_Callback_With_Arg(&editor_make_entity);
	
	current_window->add_element(current_form);
}

void Editor::end_form()
{
	current_form = NULL;
}

void Editor::not_draw_form()
{
	current_form->draw_form = false;
}

void Editor::set_form_label(const char * label)
{
	assert(current_form != NULL);

	current_form->label = label;
}

void Editor::add_item(const char * item_text, int enum_value)
{
	assert(current_list_box != NULL);

	current_list_box->add_item(item_text, enum_value);
}

void Editor::add_picked_panel(const char * item_text, int enum_value)
{
	current_picked_panel_list_box->add_item(item_text, enum_value, current_picked_panel);
}