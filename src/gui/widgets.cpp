#include "widgets.h"
#include "guiv2.h"

using namespace imgui;

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
	width = 170;
	height = 30;
	rounding = 5;
	color = Color(50, 50, 50);
}

List_Box_Theme::~List_Box_Theme()
{
}

static Text_Button_Theme button_theme;
static List_Box_Theme list_theme;

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

void begin_list_box(const char *name)
{
	begin_ui_element("List box #id");
	set_size(filled_size(), filled_size());
	set_layout(ROW_LAYOUT);
	set_alignment(ALIGNMENT_LEFT | ALIGNMENT_VERTICAL_CENTER);

	text(name);

	begin_ui_element("List box header #id");
	set_size(fixed_size(list_theme.width), fixed_size(list_theme.height));
	set_rounding(list_theme.rounding, ROUND_RECT);
	set_background_color(list_theme.color);
	end_ui_element(); // List box header

	end_ui_element(); // List box
}