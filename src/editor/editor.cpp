#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "editor.h"
#include "../libs/color.h"
#include "../libs/os/input.h"
#include "../libs/math/common.h"
#include "../sys/sys_local.h"
#include "../win32/win_time.h"
#include "../win32/win_local.h"
#include "../render/render_system.h"

#include "../game/world.h"

Editor editor;

static Window_Theme window_theme;
static Text_Button_Theme button_theme;
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

	return first_list_box->rect.y > second_list_box->rect.y;
}

inline bool detect_collision(Mouse_Info *mouse, int x, int y, int width, int height)
{
	if (mouse->x > x && mouse->x < (x + width) && mouse->y > y && mouse->y < (y + height)) {
		return true;
	}
	return false;
}

inline bool detect_collision(Mouse_Info *mouse, Rect_u32 *rect)
{
	if (mouse->x > rect->x && mouse->x < (rect->x + rect->width) && 
		mouse->y > rect->y && mouse->y < (rect->y + rect->height)) {
		return true;
	}
	return false;
}

inline float calculate_scale_based_on_percent_from_element_height(int header_height, int image_height, float percents)
{
	float needed_height = header_height * percents / 100.0f;
	float ratios = needed_height / image_height;
	return ratios;
}


inline u32 place_in_middle(Rect_u32 *dest, int height)
{
	return (dest->y + dest->height / 2) - (height / 2);
}

inline u32 place_in_middle(Rect_u32 *dest, Rect_u32 *source)
{
	return (dest->y + dest->height / 2) - (source->height / 2);
}

inline void place_in_middle(Rect_u32 *element, int width, int height)
{
	assert(element->width > 2);
	assert(element->height > 2);

	element->x = (width / 2) - (element->width / 2);
	element->y = (height / 2) - (element->height / 2);
}

inline void place_in_center(int *x, int *y, int placed_element_width, int placed_element_height, int width, int height)
{
	*x = (width / 2) - (placed_element_width / 2);
	*y = (height / 2) - (placed_element_height / 2);
}

inline void place_in_center(Rect_u32 *in_element_place, Rect_u32 *placed_element)
{
	placed_element->x = ((in_element_place->width / 2) - (placed_element->width / 2)) + in_element_place->x;
	placed_element->y = ((in_element_place->height / 2) - (placed_element->height / 2)) + in_element_place->y;
}

inline void place_in_middle_and_by_left(Rect_u32 *placing_element, Rect_u32 *placed_element, int offset_from_left = 0)
{
	placed_element->x = placing_element->x + offset_from_left;
	placed_element->y = place_in_middle(placing_element, placed_element);
}

inline void place_in_middle_and_by_right(Rect_u32 *placing_element, Rect_u32 *placed_element, int offset_from_right = 0)
{
	placed_element->x = placing_element->x + placing_element->width - placed_element->width - offset_from_right;
	placed_element->y = place_in_middle(placing_element, placed_element);
}

void inline place_by_left(Rect_u32 *element, Rect_u32 *point, int offset_from_left = 0)
{
	point->x = element->x + offset_from_left;
	point->y = element->y;
}

static void draw_debug_rect(Rect_u32 *rect)
{
	Render_2D *render_2d = get_render_2d();
	//render_2d->draw_rect(rect, Color::Red);
}

Text::Text(const char *text)
{
	Size_u32 size = font.get_text_size(text);
	string = text;
	rect.set(size);
}

void Text::operator=(const char *text)
{
	Size_u32 size = font.get_text_size(text);
	string = text;
	rect.set(size);
}

void Text::draw()
{
	Render_2D *render_2d = get_render_2d();
	//render_2d->draw_text(rect.x, rect.y, string);
}

void Text::set_position(u32 x, u32 y)
{
	rect.set(x, y);
}

void Text::update_size()
{
	Size_u32 size = font.get_text_size(string);
	rect.set(size);
}

Caret::Caret(u32 x, u32 y, u32 height)
{
	rect.set(x, y);
	rect.set_size(1, height);
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

	Render_2D *render_2d = get_render_2d();

	current_time = milliseconds_counter();

	s64 elapsed_time = current_time - last_time;

	if (show) {
		show_time_accumulator += elapsed_time;
		if (show_time_accumulator < show_time) {
			//render_2d->draw_rect(&rect, Color::White);
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

void Caret::set_position(u32 x, u32 y)
{
	rect.set(x, y);
}

Button::~Button()
{
	DELETE_PTR(callback);
}

void Button::handle_event(Event *event)
{
	if (event->type == EVENT_TYPE_MOUSE) {
		if (detect_collision(&event->mouse_info, &rect)) {
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

Button::Button(const Button &other)
{
	rect = other.rect;
	theme = other.theme;
	callback = other.callback->copy();
}

void Button::operator=(const Button &other)
{
	rect = other.rect;
	theme = other.theme;
	
	if (other.callback) {
		callback = other.callback->copy();
	}
}

List_Box::List_Box(const char *_label, u32 x, u32 y)
{
	u32 total_width = theme.field_width + font.get_text_width(_label) + theme.list_box_shift_from_text;
	
	type = ELEMENT_TYPE_LIST_BOX;
	theme = list_box_theme;
	list_box_size = 0;
	label = Text(_label);
	rect = Rect_u32(x, y, total_width, theme.field_height);
	field_rect = Rect_u32(x + font.get_text_width(_label) + theme.list_box_shift_from_text, y, theme.field_width, rect.height);

	button_theme.border_about_text = 0;
	button_theme.rounded_border = 0.0f;
	button_theme.color = theme.color;

	Texture *down_texture = texture_manager.get_texture("down.png");
	float down_scale_factor = calculate_scale_based_on_percent_from_element_height(rect.height, down_texture->height, 70);

	drop_button = Texture_Button(down_texture, down_scale_factor);
	drop_button.callback = new Member_Callback<List_Box>(this, &List_Box::on_drop_button_click);

	current_chosen_item_text = "There is no added items.";
}

void List_Box::draw()
{
	label.draw();

	Render_2D *render_2d = get_render_2d();
	//render_2d->draw_rect(&field_rect, theme.color, theme.rounded_border);

	drop_button.draw();
	current_chosen_item_text.draw();

	if (list_state == LIST_BOX_IS_DROPPED) {
		//render_2d->draw_rect(&list_box_rect, theme.color, theme.rounded_border);
		
		for (int i = 0; i < item_list.count; i++) {
			item_list[i].draw();
		}
	}
}

void List_Box::on_list_item_click()
{
	if (button) {
		button->flags &= ~ELEMENT_HOVER;
		list_state = LIST_BOX_IS_PICKED_UP;
		current_chosen_item_text = button->text.string;
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
		Text_Button *b = NULL;
		For(item_list, b) {
			button = b;
			b->handle_event(event);
		}
	}
}

void List_Box::add_item(const char *item_text)
{
	Text_Button button = Text_Button(item_text, field_rect.width, field_rect.height);
	button.set_position(field_rect.x, field_rect.bottom() + (field_rect.height * item_list.count) + theme.list_box_top_bottom_border);
	button.set_theme(&button_theme);
	button.callback = new Member_Callback<List_Box>(this, &List_Box::on_list_item_click);
	
	item_list.push(button);

	list_box_size += button.get_height();

	list_box_rect.set_size(field_rect.width, list_box_size + theme.list_box_top_bottom_border);

	if (item_list.count == 1) {
		current_chosen_item_text = item_list[0].text.string;
	}
}

void List_Box::add_item(const char * string, int enum_value)
{
	string_enum_pairs.set(string, enum_value);
	add_item(string);
}

int List_Box::get_chosen_enum_value()
{
	return string_enum_pairs[current_chosen_item_text.string];
}

String *List_Box::get_chosen_item_string()
{
	return &current_chosen_item_text.string;
}

void List_Box::set_position(u32 x, u32 y)
{
	rect.set(x, y);
	place_by_left(&rect, &field_rect, label.get_width() + theme.list_box_shift_from_text);
	place_in_middle_and_by_left(&field_rect, current_chosen_item_text, theme.field_text_offset_from_left);
	place_in_middle_and_by_left(&rect, label);
	place_in_middle_and_by_right(&rect, drop_button, theme.drop_button_offset_from_right);

	list_box_rect.set(field_rect.x, field_rect.bottom() + theme.list_box_offset);

	for (u32 index = 0; index < item_list.count; index++) {
		item_list[index].set_position(field_rect.x, field_rect.bottom() + (field_rect.height * index) + theme.list_box_top_bottom_border);
	}
}

Panel_List_Box::Panel_List_Box(int _x, int _y, const char *_label) : List_Box(_label, _x, _y)
{
	type = ELEMENT_TYPE_PANEL_LIST_BOX;
}

void Panel_List_Box::on_list_item_click()
{
	List_Box::on_list_item_click();

	Picked_Panel *current_picked_panel = string_picked_panel_pairs[current_chosen_item_text.string];
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

	Text_Button &button = item_list.last_item();
	DELETE_PTR(button.callback);
	button.callback = new Member_Callback<Panel_List_Box>(this, &Panel_List_Box::on_list_item_click);

	string_picked_panel_pairs.set(string, picked_panel);
}

Picked_Panel::~Picked_Panel()
{
	Input_Field *input_field = NULL;
	For(input_fields, input_field) {
		DELETE_PTR(input_field);
	}
}

Picked_Panel *Panel_List_Box::get_picked_panel()
{
	return string_picked_panel_pairs[current_chosen_item_text.string];
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

void Picked_Panel::set_position(u32 x, u32 y)
{
	assert(false);
}

Edit_Field::Edit_Field(const char *_label_text, Edit_Data_Type _edit_data_type, Edit_Field_Theme *edit_theme, u32 x, u32 y)
{
	if (edit_theme) {
		theme = *edit_theme;
	} else {
		theme = edit_field_theme;
	}

	int caret_height = (int)(theme.field_height * theme.caret_height_in_percents / 100.0f);

	type = ELEMENT_TYPE_EDIT_FIELD;

	u32 total_width = theme.field_width + font.get_text_width(_label_text) + theme.field_shift_from_text;
	rect = Rect_u32(x, y, total_width, theme.field_height);

	edit_itself_data = true;

	max_symbol_number = (theme.field_width - theme.shift_caret_from_left * 3 - 10);

	edit_data_type = _edit_data_type;

	label = Text(_label_text);

	caret = Caret(x + theme.shift_caret_from_left, place_in_middle(*this, caret_height), caret_height);

	field_rect.set_size(theme.field_width, theme.field_height);


	if (edit_data_type == EDIT_DATA_INT) {
		text = "0";
		caret.rect.x += text.get_width();

		caret_index_in_text = 0;
		text_width = text.get_width();

		edit_data.itself_data.int_value = 0;

	} else if (edit_data_type == EDIT_DATA_FLOAT) {
		text = "0.0";
		caret.rect.x += text.get_width();

		caret_index_for_inserting = 3;
		caret_index_in_text = 2;
		text_width = text.get_width();

		edit_data.itself_data.float_value = 0.0f;
	}
}

Edit_Field::Edit_Field(const char * _label_text, float *float_value, Edit_Field_Theme *edit_theme, u32 x, u32 y)
{
	if (edit_theme) {
		theme = *edit_theme;
	} else {
		theme = edit_field_theme;
	}

	int caret_height = (int)(theme.field_height * theme.caret_height_in_percents / 100.0f);

	type = ELEMENT_TYPE_EDIT_FIELD;

	u32 total_width = theme.field_width + font.get_text_width(_label_text) + theme.field_shift_from_text;
	rect = Rect_u32(x, y, total_width, theme.field_height);


	edit_itself_data = false;

	max_symbol_number = (theme.field_width - theme.shift_caret_from_left * 3);

	edit_data_type = EDIT_DATA_FLOAT;

	label = Text(_label_text);

	caret = Caret(x + theme.shift_caret_from_left, place_in_middle(*this, caret_height), caret_height);

	edit_data.not_itself_data.float_value = float_value;

	char *float_str = to_string(*float_value);

	field_rect.set_size(theme.field_width, theme.field_height);

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

		if (text.string != str_value) {
			set_text(str_value);
		}
		free_string(str_value);
	}

	static int last_mouse_x;
	static int last_mouse_y;

	if (event->type == EVENT_TYPE_MOUSE) {
		if (detect_collision(&event->mouse_info, &rect)) {
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

					char c = text.string[caret_index_in_text];
					text.string.remove(caret_index_in_text);
					text.update_size();
					
					u32 char_width = font.get_char_width(c);
					caret.rect.x -= (float)char_width;
					text_width -= char_width;

					caret_index_in_text -= 1;
					caret_index_for_inserting -= 1;

					update_edit_data(text.string);
				}

			} else if (event->is_key_down(VK_LEFT)) {
				if (caret_index_in_text > -1) {

					char c = text.string[caret_index_in_text];
					u32 char_width = font.get_char_width(c);
					caret.rect.x -= (float)char_width;
					text_width -= char_width;

					caret_index_in_text -= 1;
					caret_index_for_inserting -= 1;
				}
			} else if (event->is_key_down(VK_RIGHT)) {
				if (caret_index_in_text < text.string.len) {

					caret_index_in_text += 1;
					caret_index_for_inserting += 1;

					char c = text.string[caret_index_in_text];
					u32 char_width = font.get_char_width(c);
					caret.rect.x += (float)char_width;
					text_width += char_width;
				}
			}
		}
	} else if (event->type == EVENT_TYPE_CHAR) {
		if (flags & ELEMENT_FOCUSED) {
			if ((max_symbol_number > text_width) && (isalnum(event->char_key) || isspace(event->char_key) || (event->char_key == '.') || (event->char_key == '-'))) {

				if (caret_index_in_text == (text.string.len - 1)) {
					text.string.append(event->char_key);
				} else {
					text.string.insert(caret_index_for_inserting, event->char_key);
				}
				text.update_size();

				caret_index_in_text += 1;
				caret_index_for_inserting += 1;

				u32 char_width = font.get_char_width(event->char_key);
				caret.rect.x += (float)char_width;
				text_width += char_width;

				update_edit_data(text.string);

			}
		}
	}
}

void Edit_Field::draw()
{
	Render_2D *render_2d = get_render_2d();
	
	//render_2d->draw_rect(&field_rect, theme.color, theme.rounded_border);

	label.draw();

	text.draw();
	
	if (flags & ELEMENT_FOCUSED) {
		caret.draw();
	}
}

void Edit_Field::set_position(u32 x, u32 y)
{
	u32 caret_height = (u32)(theme.field_height * theme.caret_height_in_percents / 100.0f);
	
	rect.set(x, y);
	place_by_left(&rect, &field_rect, label.get_width() + theme.field_shift_from_text);
	place_in_middle_and_by_left(&field_rect, text, theme.text_offset_from_left);

	label.set_position(x, place_in_middle(&rect, label.get_height()));
	caret.set_position(x + theme.shift_caret_from_left, place_in_middle(&rect, caret_height));
	place_in_middle_and_by_left(&field_rect, caret, theme.shift_caret_from_left);

	u32 text_width = text.get_width();
	caret.rect.x = field_rect.x + theme.shift_caret_from_left + text_width;
}

void Edit_Field::set_caret_position_on_mouse_click(int mouse_x, int mouse_y)
{
	int text_width = text.get_width();
	int mouse_x_relative_text = mouse_x - rect.x - theme.shift_caret_from_left;

	if (mouse_x_relative_text > text_width) {
		caret.rect.x = field_rect.x + theme.shift_caret_from_left + text_width;
		return;
	}

	float caret_temp_x = caret.rect.x;
	float characters_width = 0.0f;

	for (int i = 0; i < text.string.len; i++) {
		char c = text.string[i];
		u32 char_width = font.get_char_width(c);
		float mouse_x_relative_character = mouse_x_relative_text - characters_width;

		float pad = 0.5f;
		if ((mouse_x_relative_character >= 0.0f) && ((mouse_x_relative_character + pad) <= char_width)) {

			float characters_width = 0.0f;
			for (int j = 0; j < i; j++) {
				char _char = text.string.data[j];
				u32 char_width = font.get_char_width(c);
				characters_width += char_width;
			}

			caret.rect.x = rect.x + theme.shift_caret_from_left + characters_width;
			caret_index_for_inserting = i;
			caret_index_in_text = i - 1;
			break;
		}

		characters_width += char_width;
		caret.rect.x = caret_temp_x;
	}
}

void Edit_Field::set_text(const char * _text)
{
	if (!text.string.is_empty()) {
		u32 width_current_text = text.get_width();
		caret.rect.x -= width_current_text;
	}

	text = String(_text, 0, 7);

	int _width = text.get_width();
	caret.rect.x += _width;

	caret_index_for_inserting = text.string.len;
	caret_index_in_text = text.string.len - 1;
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

Vector3_Edit_Field::Vector3_Edit_Field(const char *_label, u32 x, u32 y)
{
	//@Note: The Constructor doeesn't support setting of positon.
	assert(x == 0);

	type = ELEMENT_TYPE_VECTOR3_EDIT_FIELD;

	label = Text(_label);

	Edit_Field_Theme theme;
	theme.field_width = 60;

	x_field = Edit_Field("X", EDIT_DATA_FLOAT, &theme);
	y_field = Edit_Field("Y", EDIT_DATA_FLOAT, &theme);
	z_field = Edit_Field("Z", EDIT_DATA_FLOAT, &theme);

	Size_u32 label_size = font.get_text_size(_label);
	
	u32 fields_count = 3;
	u32 total_width = label_size.width + x_field.get_width() + y_field.get_width() + z_field.get_width() + (place_between_fields * fields_count);
	rect = Rect_u32(x, y, total_width, theme.field_height);
}

Vector3_Edit_Field::Vector3_Edit_Field(const char * _label, Vector3 *vec3, u32 x, u32 y)
{
	//@Note: The Constructor doeesn't support setting of positon
	assert(x == 0);

	type = ELEMENT_TYPE_VECTOR3_EDIT_FIELD;

	label = Text(_label);

	Edit_Field_Theme theme;
	theme.field_width = 50;

	x_field = Edit_Field("x", &vec3->x, &theme);
	y_field = Edit_Field("y", &vec3->y, &theme);
	z_field = Edit_Field("z", &vec3->z, &theme);
}

void Vector3_Edit_Field::draw()
{
	//draw_debug_rect(&x_field);
	label.draw();
	x_field.draw();
	y_field.draw();
	z_field.draw();
}

void Vector3_Edit_Field::handle_event(Event * event)
{
	x_field.handle_event(event);
	y_field.handle_event(event);
	z_field.handle_event(event);
}

void Vector3_Edit_Field::set_position(u32 x, u32 y)
{
	rect.set(x, y);

	label.set_position(x, place_in_middle(*this, label.get_height()));

	int x_field_position = x + label.get_width() + place_between_fields;
	x_field.set_position(x_field_position, y);

	int y_field_position = x_field_position + x_field.get_width() + place_between_fields;
	y_field.set_position(y_field_position, y);
	
	int z_field_position = y_field_position + y_field.get_width() + place_between_fields;
	z_field.set_position(z_field_position, y);
}

void Vector3_Edit_Field::get_vector(Vector3 *vector)
{
	vector->x = x_field.get_float_value();
	vector->y = y_field.get_float_value();
	vector->z = z_field.get_float_value();
}

Form::Form()
{
	type = ELEMENT_TYPE_FORM;
	submit = new Text_Button("Submit");
}

void Form::draw()
{
	submit->draw();
}

void Form::add_field(Input_Field *input_field)
{
	input_fields.push(input_field);
}

void Form::set_position(u32 x, u32 y)
{
	submit->set_position(x, y);
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

void Form::fill_args(Args *args, Array<Input_Field *> *fields)
{
	Input_Field *input_field = NULL;
	For((*fields), input_field)
	{
		String str = input_field->label.string;
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
	For(elements, element) {
		delete element;
	}

	List_Box *list_box = NULL;
	For(list_boxies, list_box) {
		delete list_box;
	}
}

void Window::make_window(int x, int y, u32 width, u32 height, int _flags, Window_Theme *_theme)
{
	next_place.x = 0;
	next_place.y = 0;
	x = x > -1 ? x : 0;
	y = y > -1 ? y : 0;
	rect = Rect_u32((u32)x, (u32)y, width, height);

	type = ELEMENT_TYPE_WINDOW;
	theme = _theme ? *_theme : window_theme;

	if (x == -1 || y == -1) {
		place_in_middle(&rect, win32.window_width, win32.window_height);
	} 

	if (_flags & WINDOW_CENTER) {
		place_in_middle(&rect, win32.window_width, win32.window_height);
	}

	if (_flags & WINDOW_WITH_HEADER) {
		flags |= WINDOW_WITH_HEADER;
		next_place.y += window_theme.header_height + window_theme.offset_from_header;

	}
	next_place.x += rect.x + window_theme.shift_element_from_window_side;
	next_place.y += rect.y;
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
		input_fields.push(&vec3->x_field);
		input_fields.push(&vec3->y_field);
		input_fields.push(&vec3->z_field);
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
	header = Rect_u32(rect.x, rect.y, rect.width, theme.header_height);
	
	Texture *cross = texture_manager.get_texture("cross_button1.png");
	float cross_scale_factor = calculate_scale_based_on_percent_from_element_height(window_theme.header_height, cross->height, 70.0f);

	close_button = Texture_Button(cross, 3.0f);
	close_button.callback = new Member_Callback<Window>(this, &Window::window_callback);
	close_button.texture = texture_manager.get_texture("cross_button1.png");
}

void Window::calculate_current_place(Element *element)
{
	if (place == PLACE_VERTICALLY) {
		if (aligment == RIGHT_ALIGNMENT) {		
			next_place.x = rect.right() - (element->get_width() + theme.shift_element_from_window_side);
		} else {
			next_place.x = rect.x + theme.shift_element_from_window_side;
		}

	} else if (place == PLACE_HORIZONTALLY) {
		if (aligment == RIGHT_ALIGNMENT) {
			next_place.x -= element->get_width() + window_theme.place_between_elements;
			if (next_place.x < rect.x) {
				next_place.x = (rect.right() - theme.shift_element_from_window_side) - (element->get_width() + window_theme.place_between_elements);
				next_place.y += element->get_height() + window_theme.place_between_elements;
			}
		} else {
			if ((next_place.x + element->get_width()) > rect.right()) {
				next_place.x = rect.x + theme.shift_element_from_window_side;
				next_place.y += element->get_height() + window_theme.place_between_elements;
			}
		}
	} else if (place == PLACE_HORIZONTALLY_AND_IN_MIDDLE) {
		next_place.y = place_in_middle(&rect, &element->rect);
	}
}

void Window::calculate_next_place(Element * element)
{
	if (place == PLACE_VERTICALLY) {
		next_place.y += element->get_height() + window_theme.place_between_elements;
	} else {
		if (aligment == RIGHT_ALIGNMENT) {
			//next_place.x -= element->width + window_theme.place_between_elements;
		} else {
			next_place.x += element->get_width() + window_theme.place_between_elements;
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
		For(windows_will_be_disabled, window) {
			window->window_active = false;
		}
	}
}

void Window::move(int x_delta, int y_delta)
{
	rect.x += x_delta;
	rect.y += y_delta;

	header_text_position.x += x_delta;
	header_text_position.y += y_delta;

	place_in_middle_and_by_right(&rect, close_button, theme.shift_cross_button_from_left_on);

	Element *element = NULL;
	For(elements, element) {
		element->set_position(element->get_x() + x_delta, element->get_y() + y_delta);
	}


	List_Box *list_box = NULL;
	For(list_boxies, list_box) {
		list_box->set_position(list_box->get_x() + x_delta, list_box->get_y() + y_delta);
	}
}

void Window::set_name(const char *_name)
{
	int _x, _y;
	Size_u32 size = font.get_text_size(_name);
	
	name = _name;
	
	place_in_center(&_x, &_y, size.width, size.height, rect.width, theme.header_height);
	header_text_position.x = _x + rect.x;
	header_text_position.y = _y + rect.y;
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
			For(panel->input_fields, input_field) {
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
		next_place.x = rect.right() - theme.shift_element_from_window_side;
	} else {
		aligment = LEFT_ALIGNMENT;
		next_place.x = rect.x + theme.shift_element_from_window_side;
	}
}

void Window::handle_event(Event *event)
{
	if (event->type == EVENT_TYPE_MOUSE) {
		if (detect_collision(&event->mouse_info, &rect)) {
			flags |= ELEMENT_HOVER;
		} else {
			flags &= ~ELEMENT_HOVER;
		}
	}

	if (flags & WINDOW_WITH_HEADER) {

		if (event->type == EVENT_TYPE_MOUSE) {

			Rect_u32 header_rect = Rect_u32(rect.x, rect.y, rect.width, window_theme.header_height);
			if (detect_collision(&event->mouse_info, &header_rect)) {
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
	//if (close_window) {
	//	return;
	//}

	//float factor = window_theme.window_rounded_factor + 2;
	//direct2d.draw_rounded_rect(x, y, width, height, factor, factor, window_theme.header_color);
	//direct2d.draw_rounded_rect(x, y + window_theme.header_height, width, height - window_theme.header_height, factor, factor, window_theme.color);
	//direct2d.fill_rect(x, y + window_theme.header_height, width, 10, window_theme.color);

	Render_2D *render_2d = render_sys.get_render_2d();
	
	//render_2d->draw_rect(&rect, window_theme.color);
	//direct2d.draw_rect(x, y, width, height, window_theme.header_color, 3.0f);

	if (flags & WINDOW_WITH_HEADER) {

		Rect_u32 header_rect = Rect_u32(rect.x, rect.y, rect.width, window_theme.header_height);
		if (flags & ELEMENT_FOCUSED) {
			//render_2d->draw_rect(&header_rect, Color(26, 26, 26));
		} else {
			//render_2d->draw_rect(&header_rect, window_theme.header_color);
		}

		if (theme.draw_window_name_in_header && !name.is_empty()) {
			//render_2d->draw_text(header_text_position.x, header_text_position.y, name);
			//direct2d.draw_text(header_text_position.x, header_text_position.y, name);
		}
	}

	Texture *cross = texture_manager.get_texture("cross_button1.png");
	//render_2d->draw_texture(100, 100, close_button.get_width(), close_button.get_height(), cross);
	//close_button.draw();

	Element *element = NULL;
	For(elements, element) {
		element->draw();
	}

	List_Box *list_box = NULL;
	For(list_boxies, list_box) {
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
	For(windows, window) {
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
		current_window->rect.x = 0;
		current_window->rect.y = 0;
	}

	if (WINDOW_RIGHT & flags) {
		current_window->rect.x = win32.window_width - window_width;
		current_window->rect.y = 0;
	}

	current_window->rect.height = window_height;
	current_window->rect.width = window_width;
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

void Editor::make_button(const char *text, Callback *callback)
{
	assert(current_window != NULL);

	current_button = new Text_Button(text);
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

Text_Button::Text_Button(const char *_text, u32 width, u32 height)
{
	assert(_text);

	text = _text;
	theme = button_theme;
	
	if (width > 0) {
		if (width > text.get_width()) {
			rect.width = width;
		} else {
			rect.width = text.get_width();
			print("Text_Button::Text_Button : The Button width can't be less than width of the text.");
		}
	} else {
		rect.width = text.get_width();
	}

	if (height > 0) {
		if (height > text.get_height()) {
			rect.height = height;
		} else {
			rect.height = text.get_height();
			print("Text_Button::Text_Button : The Button width can't be less than height of the text.");
		}
	} else {
		rect.height = text.get_height();
	}

	rect.width += theme.border_about_text;
	rect.height += theme.border_about_text;
}

void Text_Button::draw()
{
	Render_2D *render_2d = get_render_2d();
	
	if (flags & ELEMENT_HOVER) {
		//render_2d->draw_rect(&rect, theme.hover_color, theme.rounded_border);
	} else {
		//render_2d->draw_rect(&rect, theme.color, theme.rounded_border);
	}
	
	text.draw();
}

void Text_Button::set_theme(Text_Button_Theme *_theme)
{
	rect.width -= theme.border_about_text;
	rect.height -= theme.border_about_text;
	
	theme = *_theme;
	
	rect.width += theme.border_about_text;
	rect.height += theme.border_about_text;

	place_in_center(&rect, text);
}

void Text_Button::set_position(u32 x, u32 y)
{
	rect.set(x, y);
	place_in_center(&rect, text);
}

Texture_Button::Texture_Button(Texture *_texture, float scale)
{
	assert(_texture);

	texture = _texture;
	rect.width = texture->width * scale;
	rect.height = texture->height * scale;
}

void Texture_Button::draw()
{
	Render_2D *render_2d = get_render_2d();
	//render_2d->draw_texture(rect.x, rect.y, rect.width, rect.height, texture);
}

void Texture_Button::set_position(u32 x, u32 y)
{
	rect.set(x, y);
}
