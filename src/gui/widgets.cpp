#include <assert.h>

#include "widgets.h"

using namespace imgui;

static Text_Button_Theme button_theme;
static List_Box_Theme list_theme;
static Check_Box_Theme check_box_theme;

struct UI_State {
	Element_ID active_list_box;
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

void init_widgets()
{
}

bool button(const char *button_text)
{
	begin_ui_element(button_text);
	set_size(fixed_size(button_theme.width), fixed_size(button_theme.height));
	set_alignment(ALIGNMENT_CENTER);
	text(button_text);
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

	begin_ui_element("List Box Header #id");

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

	text(list[*index]);

	if (ui_element_clicked() || ui_element_double_clicked()) {
		if (list_field->id == ui_state.active_list_box) {
			ui_state.active_list_box.reset();
		} else {
			ui_state.active_list_box = list_field->id;
		}
	}

	end_ui_element(); // Field

	text(label); 

	end_ui_element(); // Header

	if (list_field->id == ui_state.active_list_box) {
		begin_ui_element("List Box Drop Panel #id");
		set_position(list_field->get_rect().x, list_field->get_rect().bottom() + list_theme.field_panel_spacing);
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
			text(list[item_index]);
			
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
	begin_ui_element("Check Box #id");
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

	text(label);

	end_ui_element(); // Check box
	
	return check_box_clicked;
}
