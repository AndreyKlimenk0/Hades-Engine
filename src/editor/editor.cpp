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
#include "../render/render_system.h"

#include "../game/world.h"

Editor editor;

static Window_Theme window_theme;
static Button_Theme button_theme;
static List_Box_Theme list_box_theme;
static Edit_Field_Theme edit_field_theme;

void editor_make_entity(Args *args)
{
	Entity_Type entity_type;
	args->get("entity_type", (int *)&entity_type);

	if (entity_type == ENTITY_TYPE_SPHERE) {
		Vector3 vec3;
		args->get("position", &vec3);

		float radius;
		args->get("radius", &radius);

		int stack_slice;
		args->get("stack_slice", &stack_slice);

		int stack_count;
		args->get("stack_count", &stack_count);

		world.entity_manager.make_sphere(vec3, radius, stack_slice, stack_count);
	} else if (entity_type == ENTITY_TYPE_LIGHT) {
		Light_Type light_type;
		args->get("light_type", (int *)&light_type);

		if (light_type == POINT_LIGHT_TYPE) {
			Vector3 position;
			args->get("position", &position);

			Vector3 color;
			args->get("color", &color);

			float range;
			args->get("range", &range);

			world.entity_manager.make_point_light(position, color, range);
		} else if (light_type == DIRECTIONAL_LIGHT_TYPE) {
			Vector3 direction;
			args->get("direction", &direction);

			Vector3 color;
			args->get("color", &color);

			world.entity_manager.make_direction_light(direction, color);
		}
	}

	//world.entity_manager.make_entity(entity_type, vec3);
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

static inline bool detect_collision(Mouse_Info *mouse, int x, int y, int width, int height)
{
	if (mouse->x > x && mouse->x < (x + width) && mouse->y > y && mouse->y < (y + height)) {
		return true;
	}
	return false;
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

static inline void place_in_middle(Element *element, int width, int height)
{
	assert(element->width > 2);
	assert(element->height > 2);

	element->x = (width / 2) - (element->width / 2);
	element->y = (height / 2) - (element->height / 2);
}

static inline void place_in_center(int *x, int *y, int placed_element_width, int placed_element_height, int width, int height)
{
	*x = (width / 2) - (placed_element_width / 2);
	*y = (height / 2) - (placed_element_height / 2);
}

Label::Label(int _x, int _y, const char *_text) : Element(_x, _y)
{
	text = _text;
	width = direct_write.get_text_width(text);
	height = direct_write.glyph_height;
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

Button::~Button()
{
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

void Button::init_button(int _x, int _y, int _width, int _height, Button_Theme *_button_theme)
{
	type = ELEMENT_TYPE_BUTTON;
	x = _x;
	y = _y;
	width = _width;
	height = _height;

	if (_button_theme) {
		theme = *_button_theme;
	} else {
		theme = button_theme;
	}
}

void Button::init_text_button(const char * _text, int _x, int _y, int _width, int _height, Button_Theme * _button_theme)
{
	if (_width == -1) {
		_width = direct_write.get_text_width(_text);
	}

	if (_height == -1) {
		_height = direct_write.glyph_height;
	}

	text_position = Point(x + (theme.border_about_text / 2) + theme.text_shift, y + (theme.border_about_text / 2));

	init_button(_x, _y, _width, _height, _button_theme);

	width += theme.border_about_text;
	height += theme.border_about_text;
}

void Button::init_texture_button(const char *texture_name, int _x, int _y, int _width, int _height, Button_Theme * _button_theme)
{
	texture = texture_manager.get_texture(texture_name);

	if (_width == -1) {
		_width = texture->width;
	}

	if (_height == -1) {
		_height = texture->height;
	}

	init_button(_x, _y, _width, _height, _button_theme);
}

void Button::draw()
{
	if (texture) {
		draw_texture_on_screen(x, y, texture, width, height);
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

	//ID2D1Bitmap *down_image = NULL;

	//load_bitmap_from_file("D:\\dev\\Hades-Engine\\data\\editor\\down.png", 1, 1, &down_image);
	//float cross_scale_factor = calculate_scale_based_on_percent_from_element_height(height, get_height_from_bitmap(down_image), 100);
	//D2D1_SIZE_U size = down_image->GetPixelSize();

	//drop_button_image_width = (size.width * cross_scale_factor);

	//drop_button = new Button(x + width - drop_button_image_width, y, down_image, cross_scale_factor);
	//drop_button->callback = new Member_Callback<List_Box>(this, &List_Box::on_drop_button_click);
	drop_button.init_texture_button("cross.png", x + width - 50, y);
	drop_button.callback = new Member_Callback<List_Box>(this, &List_Box::on_drop_button_click);

	text_y = place_in_middle(this, direct_write.glyph_height);
	text_x = x + 2;
}


List_Box::~List_Box()
{
	Button *button = NULL;
	For(item_list, button)
	{
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

	drop_button.draw();

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
	drop_button.handle_event(event);

	if (list_state == LIST_BOX_IS_DROPPED) {
		Button *b = NULL;
		For(item_list, b)
		{
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

int List_Box::get_chosen_enum_value()
{
	return string_enum_pairs[*current_chosen_item_text];
}

String *List_Box::get_chosen_item_string()
{
	return current_chosen_item_text;
}


void List_Box::set_position(int _x, int _y)
{
	x = _x + direct_write.get_text_width(label.text) + theme.list_box_shift_from_text;
	y = _y;

	drop_button.set_position(x + header_width - 50, _y);

	label.set_position(_x, place_in_middle(this, direct_write.glyph_height));

	text_y = place_in_middle(this, direct_write.glyph_height);
	text_x = x + 2;

	for (int i = 0; i < item_list.count; i++) {
		item_list[i]->set_position(x, (y + 2 + height) + (direct_write.glyph_height * i));
	}
}

Panel_List_Box::Panel_List_Box(int _x, int _y, const char *_label) : List_Box(_label, _x, _y)
{
	type = ELEMENT_TYPE_PANEL_LIST_BOX;
}

void Panel_List_Box::on_list_item_click()
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

void Panel_List_Box::add_item(const char *string, int enum_value, Picked_Panel *picked_panel)
{
	assert(picked_panel);

	if (item_list.is_empty()) {
		last_picked_panel = picked_panel;
		last_picked_panel->draw_panel = true;
	}

	List_Box::add_item(string, enum_value);

	Button *button = item_list.last_item();
	DELETE_PTR(button->callback);
	button->callback = new Member_Callback<Panel_List_Box>(this, &Panel_List_Box::on_list_item_click);

	string_picked_panel_pairs.set(string, picked_panel);
}

Picked_Panel::~Picked_Panel()
{
	Input_Field *input_field = NULL;
	For(input_fields, input_field)
	{
		DELETE_PTR(input_field);
	}
}

Picked_Panel *Panel_List_Box::get_picked_panel()
{
	return string_picked_panel_pairs[current_chosen_item_text];
}

void Picked_Panel::draw()
{
	if (!draw_panel) {
		return;
	}

	Input_Field *input_field = NULL;
	For(input_fields, input_field)
	{
		if (input_field->type == ELEMENT_TYPE_LIST_BOX) {
			continue;
		}
		input_field->draw();
	}

	List_Box *list_box = NULL;
	For(list_boxies, list_box)
	{
		list_box->draw();
	}
}

void Picked_Panel::handle_event(Event * event)
{
	if (!draw_panel) {
		return;
	}

	Input_Field *input_field = NULL;
	For(input_fields, input_field)
	{
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

Edit_Field::Edit_Field(const char *_label_text, Edit_Data_Type _edit_data_type, Edit_Field_Theme *edit_theme, int _x, int _y)
{
	if (edit_theme) {
		theme = *edit_theme;
	} else {
		theme = edit_field_theme;
	}

	int caret_height = (int)(theme.height * theme.caret_height_in_percents / 100.0f);

	type = ELEMENT_TYPE_EDIT_FIELD;

	x = _x + direct_write.get_text_width(_label_text) + theme.field_shift_from_text;
	y = _y;
	width = theme.width + direct_write.get_text_width(_label_text) + theme.field_shift_from_text;;
	height = theme.height;

	field_width = theme.width;

	edit_itself_data = true;

	max_text_width = (field_width - theme.shift_caret_from_left * 3 - 10);

	edit_data_type = _edit_data_type;

	label = Label(_x, place_in_middle(this, direct_write.glyph_height), _label_text);

	caret = Caret(x + theme.shift_caret_from_left, place_in_middle(this, caret_height), caret_height);


	if (edit_data_type == EDIT_DATA_INT) {
		text.append('0');;

		Direct_Character character = direct_write.characters['0'];
		caret.fx += character.width;

		caret_index_in_text = 0;
		text_width = character.width;

		edit_data.itself_data.int_value = 0;

	} else if (edit_data_type == EDIT_DATA_FLOAT) {
		text.append("0.0");

		D2D1_SIZE_F size = direct_write.get_text_size_in_pixels("0.0");
		caret.fx += size.width;

		caret_index_for_inserting = 3;
		caret_index_in_text = 2;
		text_width = size.width;

		edit_data.itself_data.float_value = 0.0f;
	}
}

Edit_Field::Edit_Field(const char * _label_text, float *float_value, Edit_Field_Theme * edit_theme, int _x, int _y)
{
	if (edit_theme) {
		theme = *edit_theme;
	} else {
		theme = edit_field_theme;
	}

	int caret_height = (int)(theme.height * theme.caret_height_in_percents / 100.0f);

	type = ELEMENT_TYPE_EDIT_FIELD;

	x = _x + direct_write.get_text_width(_label_text) + theme.field_shift_from_text;
	y = _y;
	width = theme.width + direct_write.get_text_width(_label_text) + theme.field_shift_from_text;;
	height = theme.height;

	field_width = theme.width;

	edit_itself_data = false;

	max_text_width = (field_width - theme.shift_caret_from_left * 3);

	edit_data_type = EDIT_DATA_FLOAT;

	label = Label(_x, place_in_middle(this, direct_write.glyph_height), _label_text);

	caret = Caret(x + theme.shift_caret_from_left, place_in_middle(this, caret_height), caret_height);

	edit_data.not_itself_data.float_value = float_value;

	char *float_str = to_string(*float_value);

	set_text(float_str);

	free_string(float_str);
}

void Edit_Field::handle_event(Event *event)
{

	if (!edit_itself_data) {
		char *str_value = NULL;

		if (edit_data_type == EDIT_DATA_INT) {
			str_value = to_string(*edit_data.not_itself_data.int_value);
		} else if (edit_data_type == EDIT_DATA_FLOAT) {
			str_value = to_string(*edit_data.not_itself_data.float_value);
		}

		if (text != str_value) {
			set_text(str_value);
		}
		free_string(str_value);
	}

	static int last_mouse_x;
	static int last_mouse_y;

	if (event->type == EVENT_TYPE_MOUSE) {
		if (detect_collision(&event->mouse_info, this)) {
			flags |= ELEMENT_HOVER;

			last_mouse_x = event->mouse_info.x;
			last_mouse_y = event->mouse_info.y;

		} else {
			flags &= ~ELEMENT_HOVER;
		}
	} else if (event->type == EVENT_TYPE_KEY) {
		if (flags & ELEMENT_HOVER) {
			if (was_click_by_left_mouse_button()) {
				set_caret_position_on_mouse_click(last_mouse_x, last_mouse_y);
				flags |= ELEMENT_FOCUSED;
			}
		} else {
			if (was_click_by_left_mouse_button()) {
				flags &= ~ELEMENT_FOCUSED;
			}
		}
		if (flags & ELEMENT_FOCUSED) {
			if (event->is_key_down(VK_BACK)) {
				if (caret_index_in_text > -1) {

					char c = text.data[caret_index_in_text];
					text.remove(caret_index_in_text);
					Direct_Character character = direct_write.characters[c];

					caret.fx -= character.width;
					text_width -= character.width;

					caret_index_in_text -= 1;
					caret_index_for_inserting -= 1;

					update_edit_data(text);
				}

			} else if (event->is_key_down(VK_LEFT)) {
				if (caret_index_in_text > -1) {

					char c = text.data[caret_index_in_text];
					Direct_Character character = direct_write.characters[c];
					caret.fx -= character.width;
					text_width -= character.width;

					caret_index_in_text -= 1;
					caret_index_for_inserting -= 1;
				}
			} else if (event->is_key_down(VK_RIGHT)) {
				if (caret_index_in_text < text.len) {

					caret_index_in_text += 1;
					caret_index_for_inserting += 1;

					char c = text.data[caret_index_in_text];
					Direct_Character character = direct_write.characters[c];
					caret.fx += character.width;
					text_width += character.width;
				}
			}
		}
	} else if (event->type == EVENT_TYPE_CHAR) {
		if (flags & ELEMENT_FOCUSED) {
			if ((max_text_width > text_width) && (isalnum(event->char_key) || isspace(event->char_key) || (event->char_key == '.') || (event->char_key == '-'))) {

				if (caret_index_in_text == (text.len - 1)) {
					text.append(event->char_key);
				} else {
					text.insert(caret_index_for_inserting, event->char_key);
				}
				caret_index_in_text += 1;
				caret_index_for_inserting += 1;

				Direct_Character character = direct_write.characters[event->char_key];
				caret.fx += character.width;
				text_width += character.width;

				update_edit_data(text);

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
		direct2d.draw_text(x + theme.shift_caret_from_left, place_in_middle(this, direct_write.glyph_height) + 1, text);
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

void Edit_Field::set_caret_position_on_mouse_click(int mouse_x, int mouse_y)
{
	int text_width = direct_write.get_text_width(text);
	int mouse_x_relative_text = mouse_x - x - theme.shift_caret_from_left;

	if (mouse_x_relative_text > text_width) {
		caret.fx = x + theme.shift_caret_from_left + text_width + caret.fwidth;
		return;
	}

	float caret_temp_x = caret.fx;
	float characters_width = 0.0f;

	for (int i = 0; i < text.len; i++) {
		char c = text.data[i];
		Direct_Character character = direct_write.characters[c];
		float mouse_x_relative_character = mouse_x_relative_text - characters_width;

		float pad = 0.5f;
		if ((mouse_x_relative_character >= 0.0f) && ((mouse_x_relative_character + pad) <= character.width)) {

			float characters_width = 0.0f;
			for (int j = 0; j < i; j++) {
				char _char = text.data[j];
				Direct_Character character = direct_write.characters[_char];
				characters_width += character.width;
			}

			caret.fx = x + theme.shift_caret_from_left + characters_width;
			caret_index_for_inserting = i;
			caret_index_in_text = i - 1;
			break;
		}
		characters_width += character.width;
		caret.fx = caret_temp_x;
	}
}

void Edit_Field::set_text(const char * _text)
{
	if (!text.is_empty()) {
		int width_current_text = direct_write.get_text_width(text);
		caret.fx -= width_current_text;
	}

	text = String(_text, 0, 7);

	int _width = direct_write.get_text_width(text);
	caret.fx += _width;

	caret_index_for_inserting = text.len;
	caret_index_in_text = text.len - 1;
	text_width = _width;
}

void Edit_Field::update_edit_data(const char *text)
{
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

int Edit_Field::get_int_value()
{
	if (edit_itself_data) {
		return edit_data.itself_data.int_value;
	}
	return *edit_data.not_itself_data.int_value;
}

float Edit_Field::get_float_value()
{
	if (edit_itself_data) {
		return edit_data.itself_data.float_value;
	}
	return *edit_data.not_itself_data.float_value;
}

Vector3_Edit_Field::Vector3_Edit_Field(const char *_label, int _x, int _y)
{
	type = ELEMENT_TYPE_VECTOR3_EDIT_FIELD;

	label = Label(0, 0, _label);

	Edit_Field_Theme theme;
	theme.width = 60;

	x = Edit_Field("x", EDIT_DATA_FLOAT, &theme);
	y = Edit_Field("y", EDIT_DATA_FLOAT, &theme);
	z = Edit_Field("z", EDIT_DATA_FLOAT, &theme);

	width = x.width; // made for Window::calculate_place_by_x
	height = x.height; // made for Window::go_to_next_element_place
}

Vector3_Edit_Field::Vector3_Edit_Field(const char * _label, Vector3 * vec3, int _x, int _y)
{
	type = ELEMENT_TYPE_VECTOR3_EDIT_FIELD;

	label = Label(0, 0, _label);

	Edit_Field_Theme theme;
	theme.width = 50;

	x = Edit_Field("x", &vec3->x, &theme);
	y = Edit_Field("y", &vec3->y, &theme);
	z = Edit_Field("z", &vec3->z, &theme);

	width = x.width; // made for Window::calculate_place_by_x
	height = x.height; // made for Window::go_to_next_element_place
}

void Vector3_Edit_Field::draw()
{
	label.draw();
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

void Vector3_Edit_Field::set_position(int _x, int _y)
{
	int edit_width = x.theme.width;

	x.set_position(_x - (edit_width * 2) - 40, _y);
	y.set_position(_x - edit_width - 20, _y);
	z.set_position(_x, _y);

	// Label x position works only for current window size
	label.set_position(_x - (edit_width * 2) - 50 - label.width, x.label.y);
}

void Vector3_Edit_Field::get_vector(Vector3 * vector)
{
	vector->x = x.get_float_value();
	vector->y = y.get_float_value();
	vector->z = z.get_float_value();
}

Form::Form()
{
	type = ELEMENT_TYPE_FORM;
	submit = new Button("Submit");
}

void Form::draw()
{
	submit->draw();
}

void Form::add_field(Input_Field *input_field)
{
	input_fields.push(input_field);
}

void Form::set_position(int _x, int _y)
{
	submit->set_position(_x, _y);
}

void Form::on_submit()
{
	Args args;
	fill_args(&args, &input_fields);


	if (callback) {
		Function_Callback_With_Arg *args_callback = static_cast<Function_Callback_With_Arg *>(callback);
		args_callback->call(&args);
	}
}

void Form::fill_args(Args * args, Array<Input_Field *> *fields)
{
	Input_Field *input_field = NULL;
	For((*fields), input_field)
	{
		String str = input_field->label.text;
		str.to_lower();
		str.replace(' ', '_');

		if (input_field->type == ELEMENT_TYPE_LIST_BOX) {
			List_Box *list_box = static_cast<List_Box *>(input_field);
			args->set(str, list_box->get_chosen_enum_value());

		} else if (input_field->type == ELEMENT_TYPE_EDIT_FIELD) {
			Edit_Field *edit_field = static_cast<Edit_Field *>(input_field);

			if (edit_field->edit_data_type == EDIT_DATA_INT) {
				args->set(str, edit_field->get_int_value());
			} else if (edit_field->edit_data_type == EDIT_DATA_FLOAT) {
				args->set(str, edit_field->get_float_value());
			} else {
				assert(false);
			}

		} else if (input_field->type == ELEMENT_TYPE_PANEL_LIST_BOX) {
			Panel_List_Box *panel_list_box = static_cast<Panel_List_Box *>(input_field);
			Picked_Panel *picked_panel = panel_list_box->get_picked_panel();

			args->set(str, panel_list_box->get_chosen_enum_value());
			fill_args(args, &picked_panel->input_fields);

		} else if (input_field->type == ELEMENT_TYPE_VECTOR3_EDIT_FIELD) {
			Vector3 vec3;
			Vector3_Edit_Field *vec3_edit_field = static_cast<Vector3_Edit_Field *>(input_field);
			vec3_edit_field->get_vector(&vec3);

			args->set(str, &vec3);
		}
	}
}

void Form::handle_event(Event * event)
{
	submit->handle_event(event);
}

Window::Window()
{
	type = ELEMENT_TYPE_WINDOW;
	next_place.x = window_theme.shift_element_from_left;
	next_place.y = window_theme.header_height + 20;
	theme = window_theme;
}

Window::Window(int _width, int _height, int _flags)
{
	make_window(-1, -1, _width, _height, _flags, NULL);
}

Window::Window(int _x, int _y, int _width, int _height, int _flags)
{
	make_window(_x, _y, _width, _height, _flags, NULL);
}

Window::~Window()
{
	Element *element = NULL;
	For(elements, element)
	{
		delete element;
	}

	List_Box *list_box = NULL;
	For(list_boxies, list_box)
	{
		delete list_box;
	}

	DELETE_PTR(close_button);
}

void Window::make_window(int _x, int _y, int _width, int _height, int _flags, Window_Theme *_theme)
{
	next_place.x = 0;
	next_place.y = 0;
	width = _width;
	height = _height;
	x = _x > -1 ? _x : 0;
	y = _y > -1 ? _y : 0;

	type = ELEMENT_TYPE_WINDOW;
	theme = _theme ? *_theme : window_theme;

	if (_x == -1 || _y == -1) {
		place_in_middle(this, win32.window_width, win32.window_height);
	}

	if (_flags & WINDOW_CENTER) {
		place_in_middle(this, win32.window_width, win32.window_height);
	}

	if (_flags & WINDOW_WITH_HEADER) {
		flags |= WINDOW_WITH_HEADER;
		next_place.y += window_theme.header_height;

	}
	next_place.x += x + window_theme.shift_element_from_window_side;
	next_place.y += y + window_theme.offset_from_window_top;
}
void Window::add_header_text(const char *)
{
	if (!(flags & WINDOW_WITH_HEADER)) {
		print("Window doesn't have a header.Text can not be drawn");
	}
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

	if ((element->type == ELEMENT_TYPE_LIST_BOX) || (element->type == ELEMENT_TYPE_PANEL_LIST_BOX)) {
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

	//close_button = new Button(button_x, button_y, cross_image, cross_scale_factor);
	//close_button->callback = new Member_Callback<Window>(this, &Window::close);
}

void Window::calculate_current_place(Element * element)
{
	if (place == PLACE_VERTICALLY) {
		if (aligment == RIGHT_ALIGNMENT) {
			next_place.x = x + width - element->width - theme.shift_element_from_window_side;
		} else {
			next_place.x = x + theme.shift_element_from_window_side;
		}

	} else if (place == PLACE_HORIZONTALLY) {
		if (aligment == RIGHT_ALIGNMENT) {
			next_place.x -= element->width + window_theme.place_between_elements;
			if (next_place.x < x) {
				next_place.x = ((x + width) - theme.shift_element_from_window_side) - (element->width + window_theme.place_between_elements);
				next_place.y += element->height + window_theme.place_between_elements;
			}
		} else {
			if ((next_place.x + element->width) > (x + width)) {
				next_place.x = x + theme.shift_element_from_window_side;
				next_place.y += element->height + window_theme.place_between_elements;
			}
		}
	} else if (place == PLACE_HORIZONTALLY_AND_IN_MIDDLE) {
		next_place.y = place_in_middle(element, this);
	}
}

void Window::calculate_next_place(Element * element)
{
	if (place == PLACE_VERTICALLY) {
		next_place.y += element->height + window_theme.place_between_elements;
	} else {
		if (aligment == RIGHT_ALIGNMENT) {
			//next_place.x -= element->width + window_theme.place_between_elements;
		} else {
			next_place.x += element->width + window_theme.place_between_elements;
		}
	}
}

void Window::window_callback()
{
	if (window_active) {
		window_active = false;
	} else {
		window_active = true;
		Window *window = NULL;
		For(windows_will_be_disabled, window)
		{
			window->window_active = false;
		}
	}
}

void Window::move(int x_delta, int y_delta)
{
	x += x_delta;
	y += y_delta;

	header_text_position.x += x_delta;
	header_text_position.y += y_delta;

	Element *element = NULL;
	For(elements, element) {
		element->set_position(element->x + x_delta, element->y + y_delta);
	}

	//List_Box *list_box = NULL;
	//For(list_boxies, list_box) {
	//	list_box->set_position(list_box->x + x_delta, list_box->y + y_delta);
	//}
}

void Window::set_name(const char *_name)
{
	int _x, _y;
	D2D1_SIZE_F size = direct_write.get_text_size_in_pixels(_name);
	
	name = _name;
	
	place_in_center(&_x, &_y, size.width, size.height, width, theme.header_height);
	header_text_position.x = _x + x;
	header_text_position.y = _y + y;
}

void Window::set_element_position(Element *element)
{
	if (element->type == ELEMENT_TYPE_PICKED_PANEL) {
		return;
	}

	if (element->type == ELEMENT_TYPE_FORM) {
		Form *form = static_cast<Form *>(element);
		element = form->submit;
	}

	calculate_current_place(element);

	if (element->type == ELEMENT_TYPE_PANEL_LIST_BOX) {
		Panel_List_Box *panel_list_box = static_cast<Panel_List_Box *>(element);
		panel_list_box->set_position(next_place.x, next_place.y);
		calculate_next_place(element);

		int next_place_max_y = 0;

		for (int i = 0; i < panel_list_box->string_picked_panel_pairs.count; i++) {
			Picked_Panel *panel = panel_list_box->string_picked_panel_pairs[i];

			int temp_storage = next_place.y;

			Input_Field *input_field = NULL;
			For(panel->input_fields, input_field)
			{
				set_element_position(input_field);
			}

			if (next_place.y > next_place_max_y) {
				next_place_max_y = next_place.y;
			}

			next_place.y = temp_storage;
		}

		next_place.y = next_place_max_y;

	} else {
		element->set_position(next_place.x, next_place.y);
		calculate_next_place(element);
	}
}

void Window::set_element_place(Place _place)
{
	place = _place;
}

void Window::set_alignment(Alignment _alignment)
{
	if (_alignment == RIGHT_ALIGNMENT) {
		aligment = RIGHT_ALIGNMENT;
		next_place.x = (x + width) - theme.shift_element_from_window_side;
	} else {
		aligment = LEFT_ALIGNMENT;
		next_place.x = x + theme.shift_element_from_window_side;
	}
}

void Window::handle_event(Event *event)
{
	if (event->type == EVENT_TYPE_MOUSE) {
		if (detect_collision(&event->mouse_info, x, y, width, height)) {
			flags |= ELEMENT_HOVER;
		} else {
			flags &= ~ELEMENT_HOVER;
		}
	}

	if (flags & WINDOW_WITH_HEADER) {

		if (event->type == EVENT_TYPE_MOUSE) {

			if (detect_collision(&event->mouse_info, x, y, width, window_theme.header_height)) {
				flags |= WINDOW_HEADER_HOVER;
			} else {
				flags &= ~WINDOW_HEADER_HOVER;
			}

			int x_delta = event->mouse_info.x - last_mouse_position.x;
			int y_delta = event->mouse_info.y - last_mouse_position.y;

			if (can_move) {
				move(x_delta, y_delta);
			}

			last_mouse_position.x = event->mouse_info.x;
			last_mouse_position.y = event->mouse_info.y;
		} else if (event->type == EVENT_TYPE_KEY) {
			if (is_left_mouse_button_down() && (flags & WINDOW_HEADER_HOVER)) {
				can_move = true;
			} else {
				can_move = false;
			}
		}
	}

	Element *element = NULL;
	For(elements, element)
	{
		element->handle_event(event);
	}

	List_Box *list_box = NULL;
	For(list_boxies, list_box)
	{
		list_box->handle_event(event);
	}
}

void Window::update()
{
}

void Window::draw()
{
	//if (close_window) {
	//	return;
	//}

	//float factor = window_theme.window_rounded_factor + 2;
	//direct2d.draw_rounded_rect(x, y, width, height, factor, factor, window_theme.header_color);
	//direct2d.draw_rounded_rect(x, y + window_theme.header_height, width, height - window_theme.header_height, factor, factor, window_theme.color);
	//direct2d.fill_rect(x, y + window_theme.header_height, width, 10, window_theme.color);
	direct2d.fill_rect(x, y, width, height, window_theme.color);
	direct2d.draw_rect(x, y, width, height, window_theme.header_color, 3.0f);

	if (flags & WINDOW_WITH_HEADER) {

		if (flags & ELEMENT_FOCUSED) {
			direct2d.fill_rect(x, y, width, window_theme.header_height, Color(26, 26, 26));
		} else {
			direct2d.fill_rect(x, y, width, window_theme.header_height, window_theme.header_color);
		}

		if (theme.draw_window_name_in_header && !name.is_empty()) {
			direct2d.draw_text(header_text_position.x, header_text_position.y, name);
		}
	}

	//close_button->draw();


	Element *element = NULL;
	For(elements, element)
	{
		element->draw();
	}

	List_Box *list_box = NULL;
	For(list_boxies, list_box)
	{
		list_box->draw();
	}
}

void Editor::add_window(Window *window)
{
	drawn_windows.append_front(window);
	windows.push(window);
}

void Editor::handle_event(Event * event)
{
	bool window_hover = false;

	Node<Window *> *next_node = NULL;
	for (Node<Window *> *window_node = drawn_windows.first_node; window_node != NULL; window_node = next_node) {
		next_node = window_node->next;
		Window *window = window_node->item;


		if (event->type == EVENT_TYPE_KEY) {
			if (window->flags & ELEMENT_HOVER) {
				if (is_left_mouse_button_down()) {
					window_hover = true;

					if (focused_window) {
						focused_window->flags &= ~ELEMENT_FOCUSED;
					}
					window->flags |= ELEMENT_FOCUSED;
					focused_window = window;


					drawn_windows.remove(window_node);
					drawn_windows.append_front(window_node);

					focused_window->handle_event(event);
					break;
				}
			}
		}

		if (!window->window_active) {
			continue;
		}

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
	For(windows, window)
	{
		//window->update();
	}
}

void Editor::draw()
{
	for (Node<Window *> *window_node = drawn_windows.first_node; window_node != NULL; window_node = window_node->next) {
		Window *window = window_node->item;

		if (!window->window_active || (window == focused_window)) {
			continue;
		}
		window->draw();
	}

	if (focused_window) {
		focused_window->draw();
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

void Editor::make_window(int width, int height, int flags)
{
	current_window = new Window(width, height, flags);
	add_window(current_window);
}

void Editor::make_window(int x, int y, int width, int height, int flags)
{
	current_window = new Window(x, y, width, height, flags);
	add_window(current_window);
}

void Editor::make_window_button(const char *name, Window *window)
{
	make_button(name);
	current_button->callback = make_member_callback(window, &Window::window_callback);
}

void Editor::bind_window(const char *window_will_be_drawn, const char *window_will_be_disabled)
{
	Window *w1 = find_window(window_will_be_drawn);
	Window *w2 = find_window(window_will_be_disabled);
	if (!w1) {
		print("Editor:bind_window Window which will be drawn with name {} was not found");
		return;
	}

	if (!w2) {
		print("Editor:bind_window Window which will be disabled with name {} was not found");
		return;
	}

	w1->windows_will_be_disabled.push(w2);
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

	//if (current_picked_panel) {
	//	current_picked_panel->add_field(current_list_box);
	//	return;
	//} else if (current_form) {
	//	current_form->add_field(current_list_box);
	//}
	//current_window->add_element(current_list_box);
		
	if (current_picked_panel) {
		current_picked_panel->add_field(current_list_box);
	} else if (current_form) {
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

	current_panel_list_box = new Panel_List_Box(x, y, label);

	if (current_form) {
		current_form->add_field(current_panel_list_box);
	}
}

void Editor::make_end_picked_list_box()
{
	assert(current_window != NULL);
	assert(current_panel_list_box != NULL);

	current_window->add_element(current_panel_list_box);
	current_panel_list_box = NULL;
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

void Editor::make_vector3_edit_field(const char *label)
{
	assert(current_window != NULL);

	Vector3_Edit_Field *vec3 = new Vector3_Edit_Field(label);

	// @TODO
	if (current_picked_panel) {
		current_picked_panel->add_field(vec3);
		return;
	} else if (current_form) {
		current_form->add_field(vec3);
	}
	current_window->add_element(vec3);
}

void Editor::make_vector3_edit_field(const char * label, Vector3 *data)
{
	assert(current_window != NULL);

	Vector3_Edit_Field *vec3 = new Vector3_Edit_Field(label, data);

	// @TODO
	if (current_picked_panel) {
		current_picked_panel->add_field(vec3);
		return;
	} else if (current_form) {
		current_form->add_field(vec3);
	}
	current_window->add_element(vec3);
}

void Editor::make_edit_field(const char *label, Edit_Data_Type edit_data_type)
{
	assert(current_window != NULL);

	current_edit_field = new Edit_Field(label, edit_data_type);

	if (current_picked_panel) {
		current_picked_panel->add_field(current_edit_field);
	} else if (current_form) {
		current_form->add_field(current_edit_field);
	}

	if ((!current_picked_panel) && (!current_form)) {
		current_window->add_element(current_edit_field);
	}
}

void Editor::make_edit_field(const char * label, int value)
{
}

void Editor::make_edit_field(const char * label, float *value)
{
	assert(current_window != NULL);

	current_edit_field = new Edit_Field(label, value);

	if (current_picked_panel) {
		current_picked_panel->add_field(current_edit_field);
	} else if (current_form) {
		current_form->add_field(current_edit_field);
	}

	if ((!current_picked_panel) && (!current_form)) {
		current_window->add_element(current_edit_field);
	}
}

void Editor::make_form()
{
	current_form = new Form();
	current_form->submit->callback = new Member_Callback<Form>(current_form, &Form::on_submit);
	current_form->callback = new Function_Callback_With_Arg(&editor_make_entity);
}

void Editor::end_form()
{
	current_window->add_element(current_form);
	current_form = NULL;
}

void Editor::set_form_label(const char * label)
{
	assert(current_form != NULL);

	current_form->label = label;
}

void Editor::set_window_name(const char *name)
{
	current_window->set_name(name);
}

void Editor::add_item(const char * item_text, int enum_value)
{
	assert(current_list_box != NULL);

	current_list_box->add_item(item_text, enum_value);
}

void Editor::add_picked_panel(const char * item_text, int enum_value)
{
	current_panel_list_box->add_item(item_text, enum_value, current_picked_panel);
}

Window *Editor::find_window(const char *name)
{
	Window *window = NULL;
	For(windows, window)
	{
		if (name == window->name) {
			return window;
		}
	}
	return NULL;
}