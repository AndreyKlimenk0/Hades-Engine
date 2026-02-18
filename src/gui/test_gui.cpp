#include "test_gui.h"
#include "guiv2.h"
#include "../sys/engine.h"

const Color TEAL = Color(0, 128, 128);
const Color NAVY = Color(0, 128, 128);
const Color DARK_ORANGE = Color(255, 140, 0);
const Color CABET_BLUE = Color(95, 158, 160);
const Color STEEL_BLUE = Color(70, 130, 180);

void test_forget_close_ui_element()
{
	begin_ui_element("Rect1");
	begin_ui_element("Rect2");
}

void test_forget_open_ui_element()
{
	begin_ui_element("Rect1");
	end_ui_element();
	end_ui_element();
}

void test_two_elements_have_same_name()
{
	begin_ui_element("Rect");
	set_position(100, 200);
	set_size(fixed_size(400), fixed_size(400));
	set_background_color(Color::Red);
	end_ui_element();

	begin_ui_element("Rect");
	set_position(200, 300);
	set_size(fixed_size(400), fixed_size(400));
	set_background_color(Color::Green);
	end_ui_element();
}

void test_default_element_cliping()
{
	begin_ui_element("Red rect");
	set_position(100, 200);
	set_size(fixed_size(400), fixed_size(400));
	set_background_color(Color::Red);

	begin_ui_element("Green rect");
	set_position(200, 300);
	set_size(fixed_size(400), fixed_size(400));
	set_background_color(Color::Green);
	end_ui_element();
}

void draw_test_gui()
{
	static bool init_gui = true;
	if (init_gui) {
		init_gui = false;
		Engine *engine = Engine::get_instance();
		Render_System *render_sys = &engine->render_sys;
		Size_u32 size = render_sys->get_window_size();
		init_guiv2(size.width, size.height, "FiraCode-Regular", 12, &render_sys->render_2d);
	}
	begin_frame();

	//test_forget_close_ui_element();
	//test_forget_open_ui_element();
	//test_two_elements_have_same_name();
	//test_default_element_cliping();
	
	begin_ui_element("Red rect");
	set_position(100, 100);
	set_size(filled_size(), filled_size());
	set_background_color(Color::Red);

	begin_ui_element("Green rect");
	//set_position(200, 300);
	set_size(fixed_size(400), fixed_size(200));
	set_background_color(Color::Green);
	end_ui_element();

	begin_ui_element("Dark Orange");
	//set_position(200, 300);
	set_size(fixed_size(400), fixed_size(200));
	set_background_color(DARK_ORANGE);
	end_ui_element();

	end_ui_element();
	
	
	end_frame();
}