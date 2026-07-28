#include <assert.h>

#include "widgets.h"
#include "../libs/os/event.h"
#include "../libs/os/input.h"

using namespace imgui;

static Text_Button_Theme button_theme;
static List_Box_Theme list_theme;
static Check_Box_Theme check_box_theme;
static Slider_Theme slider_theme;
static Edit_Field_Theme edit_field_theme;

struct UI_State {
	Element_ID active_list_box;
	Element_ID active_slider;
	Element_ID active_edit_field;
};

static UI_State ui_state;

Text_Button_Theme::Text_Button_Theme()
{
	width = 125;
	height = 30;
	rounding = 5;
	rounding_flags = ROUND_RECT;
	color = Color(0, 75, 168);
	hover_color = Color(0, 60, 168);
}

Text_Button_Theme::~Text_Button_Theme()
{
}

List_Box_Theme::List_Box_Theme()
{
	width = 200;
	height = 26;
	field_panel_spacing = 2;
	rounding = 6;
	list_item_spacing = 0;
	background_color = Color(30);
	item_hover_color = Color(45);
	list_panel_padding = Padding(6);
	list_item_padding = Padding(6);
}

List_Box_Theme::~List_Box_Theme()
{
}

Check_Box_Theme::Check_Box_Theme()
{
	width = 20;
	height = 20;
}

Check_Box_Theme::~Check_Box_Theme()
{
}

Slider_Theme::Slider_Theme()
{
	width = 250;
	height = 20;
	rounding = 10;
	thumb_padding = 4;
	thumb_size = 10;
	background_color = Color(25);
	outlining_color = Color(65);
	thumb_color = Color(99);
}

Slider_Theme::~Slider_Theme()
{
}


Edit_Field_Theme::Edit_Field_Theme()
{
	width = 250;
	height = 24;
	rounding = 5;
	field_color = Color(25);
}

Edit_Field_Theme::~Edit_Field_Theme()
{
}

inline Point_s32 get_async_mouse_position()
{
	return Point_s32(Mouse_State::x, Mouse_State::y);
}

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

static void set_caret_position_on_mouse_click(Rect_s32 *rect, Rect_s32 *editing_value_rect, Edit_Field_State &edit_field_state, Font *font)
{
	u32 text_width = font->get_text_width(edit_field_state.data);
	//u32 mouse_x_relative_text = (u32)math::abs(mouse_x - rect->x - edit_field_theme.text_shift);
	u32 mouse_x_relative_text = (u32)math::abs(Mouse_State::x - rect->x);

	if (mouse_x_relative_text > text_width) {
		edit_field_state.caret.x = editing_value_rect->x + text_width;
		edit_field_state.caret_index_for_inserting = edit_field_state.data.len;
		edit_field_state.caret_index_in_text = edit_field_state.data.len - 1;
	//} else if ((mouse_x >= rect->x) && (mouse_x <= (rect->x + edit_field_theme.text_shift))) {
	} else if ((Mouse_State::x >= rect->x) && (Mouse_State::x <= (rect->x))) {
		//edit_field_state.caret.x = rect->x + edit_field_theme.text_shift;
		edit_field_state.caret.x = rect->x;
		edit_field_state.caret_index_for_inserting = 0;
		edit_field_state.caret_index_in_text = -1;
	} else {
		u32 chars_width = 0;
		u32 prev_chars_advance_width = 0;

		String *ui_text_element = &edit_field_state.data;
		for (u32 i = 0; i < ui_text_element->len; i++) {
			char c = ui_text_element->data[i];
			chars_width += font->get_char_advance(c);

			if ((mouse_x_relative_text >= prev_chars_advance_width) && (mouse_x_relative_text <= chars_width)) {
				edit_field_state.caret.x = rect->x + chars_width;
				//edit_field_state.caret.x = rect->x + edit_field_theme.text_shift + chars_width;
				edit_field_state.caret_index_for_inserting = i + 1;
				edit_field_state.caret_index_in_text = i;
				break;
			}
		}
	}
}

static void handle_events(bool *update_editing_value, bool *update_next_time_editing_value, Rect_s32 *rect, Rect_s32 *editing_value_rect, Edit_Field_State &edit_field_state, Font *font)
{
	Queue<Event> *events = get_event_queue();
	for (Queue_Node<Event> *node = events->first; node != NULL; node = node->next) {
		Event *event = &node->item;
		if (event->type == EVENT_TYPE_KEY) {
			if (was_click(KEY_LMOUSE)) {
				set_caret_position_on_mouse_click(rect, editing_value_rect, edit_field_state, font);
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
				//if ((point_index != -1) && ((edit_field_state.data.len - (point_index + 1)) == edit_field_theme.float_precision) && (edit_field_state.caret_index_in_text == (edit_field_state.data.len - 1))) {
				//	return;
				//}

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

void init_widgets()
{
}

bool button(const char *button_text)
{
	begin_ui_element(button_text);
	set_size(fixed_size(button_theme.width), fixed_size(button_theme.height));
	set_alignment(ALIGNMENT_CENTER);
	ui_text_element(button_text);
	set_rounding(button_theme.rounding, button_theme.rounding_flags);
	set_background_color(ui_element_hovered() ? button_theme.hover_color : button_theme.color);
	bool click = ui_element_clicked();
	end_ui_element();
	return click;
}

bool button(Texture_View *texture_view)
{
	return false;
}

void list_box(const char *label, Array<String> &list, u32 *index)
{
	if (list.is_empty()) {
		return;
	}

	if (*index >= list.count) {
		*index = 0;
	}

	begin_ui_element("List Box Header Name[%s]", label);

	set_size(fit_size(), fit_size());
	set_layout(ROW_LAYOUT);
	set_alignment(ALIGNMENT_LEFT | ALIGNMENT_VERTICAL_CENTER);

	begin_ui_element("Field");
	UI_Element *list_field = get_ui_element();
	set_size(fixed_size(list_theme.width), fixed_size(list_theme.height));
	set_rounding(list_theme.rounding, ROUND_RECT);
	set_background_color(list_theme.background_color);
	set_padding(list_theme.list_panel_padding + list_theme.list_item_padding);

	set_layout(COLUMN_LAYOUT);
	set_alignment(ALIGNMENT_LEFT | ALIGNMENT_VERTICAL_CENTER);

	ui_text_element(list[*index]);

	if (ui_element_clicked() || ui_element_double_clicked()) {
		if (list_field->id == ui_state.active_list_box) {
			ui_state.active_list_box.reset();
		} else {
			ui_state.active_list_box = list_field->id;
		}
	}

	end_ui_element(); // Field

	ui_text_element(label); 

	end_ui_element(); // Header

	if (list_field->id == ui_state.active_list_box) {
		begin_ui_element("List Box Drop Panel Name[%s]", label);
		set_absolute_position(list_field->get_rect().x, list_field->get_rect().bottom() + list_theme.field_panel_spacing);
		set_size(fixed_size(list_theme.width), fit_size());
		set_rounding(list_theme.rounding, ROUND_RECT);
		set_background_color(list_theme.background_color);
		set_space(list_theme.list_item_spacing);
		set_padding(list_theme.list_panel_padding);
		//set_outlining(2, Color(55));

		for (u32 item_index = 0; item_index < list.count; item_index++) {
			begin_ui_element("Item #id");
			set_size(grow_size(), fixed_size(list_theme.height));
			set_layout(COLUMN_LAYOUT);
			set_alignment(ALIGNMENT_LEFT | ALIGNMENT_VERTICAL_CENTER);
			set_padding(list_theme.list_item_padding);
			set_rounding(list_theme.rounding, ROUND_RECT);
			set_space(list_theme.list_item_spacing);
			set_background_color(ui_element_hovered() ? list_theme.item_hover_color : list_theme.background_color);
			ui_text_element(list[item_index]);
			
			if (ui_element_clicked() || ui_element_double_clicked()) {
				*index = item_index;
				ui_state.active_list_box.reset();
			}
			end_ui_element();
		}
		end_ui_element(); //Drop panel
	}
}

bool check_box(const char *label, bool *state)
{
	bool check_box_clicked = false;
	begin_ui_element("Check Box Name[%s]", label);
	set_size(fit_size(), fit_size());
	set_layout(ROW_LAYOUT);
	set_alignment(ALIGNMENT_LEFT | ALIGNMENT_VERTICAL_CENTER);

	begin_ui_element("Box");
	set_size(fixed_size(check_box_theme.width), fixed_size(check_box_theme.height));
	set_alignment(ALIGNMENT_CENTER);
	set_background_color(Color(30));
	set_rounding(6);

	if (ui_element_clicked() || ui_element_double_clicked()) {
		*state = !*state;
		check_box_clicked = true;
	}

	if (*state) {
		begin_ui_element("Check");
		set_size(fixed_size(check_box_theme.width - 6), fixed_size(check_box_theme.height - 6));
		set_background_color(Color(0, 60, 168));
		set_rounding(6);
		end_ui_element();
	}

	end_ui_element(); // Box

	ui_text_element(label);

	end_ui_element(); // Check box
	
	return check_box_clicked;
}

static bool __detect_intersection(Rect_s32 *rect)
{
	if ((Mouse_State::x > rect->x) && (Mouse_State::x < (rect->x + rect->width)) && (Mouse_State::y > rect->y) && (Mouse_State::y < (rect->y + rect->height))) {
		return true;
	}
	return false;
}

void slider(const char *label, float min, float max, float *value)
{
	assert(max > min);

	static s32 thumb_relative_position;
	static Point_s32 thumb_mouse_delta;
	static bool thumb_campured = false;

	*value = math::clamp(*value, min, max);

	begin_ui_element("Slider Name[%s]", label);
	set_size(fit_size(), fit_size());
	set_layout(ROW_LAYOUT);
	set_alignment(ALIGNMENT_LEFT | ALIGNMENT_VERTICAL_CENTER);

	Element_ID id = get_ui_element()->id;

	begin_ui_element("Range");
	set_size(fixed_size(slider_theme.width), fixed_size(slider_theme.height));
	set_background_color(slider_theme.background_color);
	set_outlining(1, slider_theme.outlining_color);
	set_rounding(slider_theme.rounding);
	set_layout(ROW_LAYOUT);
	set_alignment(ALIGNMENT_VERTICAL_CENTER);
	Rect_s32 range_rect = ui_element_rect();

	begin_ui_element("Thumb");
	set_size(fixed_size(slider_theme.thumb_size), fixed_size(slider_theme.thumb_size));
	set_relative_position_x(thumb_relative_position);
	set_background_color(slider_theme.thumb_color);
	set_rounding(slider_theme.thumb_size / 2);
	Rect_s32 thumb_rect = ui_element_rect();

	if (was_key_just_pressed(KEY_LMOUSE) && __detect_intersection(&thumb_rect)) {
		thumb_campured = true;
		thumb_mouse_delta = get_async_mouse_position() - thumb_rect.get_point();
		ui_state.active_slider = id;
	}

	if (was_key_just_released(KEY_LMOUSE) && thumb_campured) {
		thumb_campured = false;
		ui_state.active_slider.reset();
	}

	if (thumb_campured && (id == ui_state.active_slider)) {
		Point_s32 new_thumb_position = get_async_mouse_position() - thumb_mouse_delta;
		new_thumb_position = new_thumb_position - range_rect.get_point();
		thumb_relative_position = math::clamp(new_thumb_position.x, slider_theme.thumb_padding, range_rect.width - thumb_rect.width - slider_theme.thumb_padding);

		float ratio = ((float)thumb_relative_position - (float)slider_theme.thumb_padding) / (float)(range_rect.width - thumb_rect.width - slider_theme.thumb_padding * 2);
		float diff = max - min;
		*value = min + diff * ratio;
	} else {
		float abs_value = *value - min;
		float diff = max - min;
		float ratio = abs_value / diff;
		s32 _thumb_relative_position = (s32)(float(range_rect.width - thumb_rect.width - slider_theme.thumb_padding * 2) * ratio) + slider_theme.thumb_padding;
		_thumb_relative_position = math::clamp(_thumb_relative_position, slider_theme.thumb_padding, range_rect.width - thumb_rect.width - slider_theme.thumb_padding);
		set_relative_position_x(_thumb_relative_position);
	}

	end_ui_element(); // Thumb

	end_ui_element(); // Range

	begin_ui_element("Value");
	set_size(fit_size(), fixed_size(slider_theme.height));
	set_alignment(ALIGNMENT_CENTER);

	char *string = to_string(*value, 1u);
	ui_text_element("String Value", string);
	free_string(string);

	end_ui_element(); // Value

	end_ui_element(); // Slider
}

void edit_field(const char *name, int *value)
{
}

void edit_field(const char *name, float *value)
{

}

static bool is_symbol_string_valid(char symbol)
{
	return !iscntrl(symbol);
}

void edit_field(const char *name, String *string)
{
	begin_ui_element("Edit Field [%s]", name);
	set_layout(ROW_LAYOUT);
	set_alignment(ALIGNMENT_LEFT | ALIGNMENT_VERTICAL_CENTER);
	set_size(fixed_size(edit_field_theme.width), fixed_size(edit_field_theme.height));
	set_rounding(edit_field_theme.rounding);
	set_background_color(edit_field_theme.field_color);

	UI_Element *edit_field = get_ui_element();

	static Rect_s32 caret_rect = { 0, 0, 2, 16 };
	static Edit_Field_State edit_field_state;

	if (ui_element_clicked()) {
		ui_state.active_edit_field = edit_field->id;

		Font *font = get_font();
		s32 text_width =  (s32)font->get_text_size(string->c_str()).width;
		//caret_rect.x = text_width;

		edit_field_state = make_edit_field_state(&caret_rect, string->c_str(), 30, is_symbol_string_valid);
	}

	ui_text_element("Editing Text", string->c_str(), TEXT_ELEMENT_MAX_SYMBOL_HEIGHT);
	auto text_rect = get_last_ui_element()->get_rect();

	if (edit_field->id == ui_state.active_edit_field) {
		bool update_value = false;
		bool update_value_next_time = false;
		auto rect = edit_field->get_rect();
		handle_events(&update_value, &update_value_next_time, &rect, &text_rect, edit_field_state, get_font());
		if (update_value) {
			*string = edit_field_state.data;
			print("Update value");
		}
		if (update_value_next_time) {
			print("Update value next time");
		}

		begin_ui_element("Edit Field Caret");
		set_size(fixed_size(caret_rect.width), fixed_size(caret_rect.height));
		//set_absolute_position();
		//set_relative_position_x(edit_field_state.caret.x);
		set_absolute_position_x(edit_field_state.caret.x);
		set_background_color(Color(180));
		//set_background_color(Color::White);
		end_ui_element();
	}
	
	end_ui_element(); // Edit Field
}