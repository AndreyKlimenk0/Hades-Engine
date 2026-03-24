#include "widgets.h"
#include "guiv2.h"

using namespace imgui;


Button_Theme default_button_theme()
{
	Button_Theme theme;
	theme.width = 100;
	theme.height = 50;
	theme.color = Color(0, 75, 168);
	theme.hover_color = Color(0, 60, 168);
	return theme;
}

static Button_Theme button_theme;

void init_widgets()
{
	button_theme = default_button_theme();
}

bool button(const char *text)
{
	begin_ui_element(text);
	set_size(fixed_size(button_theme.width), fixed_size(button_theme.height));
	set_background_color(button_theme.color);
	end_ui_element();
	return false;
}

bool button(Texture_View *texture_view)
{
	return false;
}
