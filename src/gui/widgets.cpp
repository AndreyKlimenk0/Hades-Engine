#include "widgets.h"
#include "guiv2.h"

using namespace imgui;


Button_Theme default_button_theme()
{
	Button_Theme theme;
	theme.width = 125;
	theme.height = 30;
	theme.rounding = 5;
	theme.rounding_flags = ROUND_RECT;
	theme.color = Color(0, 75, 168);
	theme.hover_color = Color(0, 60, 168);
	return theme;
}

static Button_Theme button_theme;

void init_widgets()
{
	button_theme = default_button_theme();
}

bool button(const char *button_text)
{
	begin_ui_element(button_text);
	set_size(fixed_size(button_theme.width), fixed_size(button_theme.height));
	set_alignment(ALIGNMENT_CENTER);
	text(button_text);
	set_rounding(button_theme.rounding, button_theme.rounding_flags);
	set_background_color(button_theme.color);
	end_ui_element();
	return false;
}

bool button(Texture_View *texture_view)
{
	return false;
}
