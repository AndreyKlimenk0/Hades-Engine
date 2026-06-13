#include "test_gui.h"
#include "guiv2.h"
#include "../sys/engine.h"

using namespace imgui;

const Color TEAL = Color(0, 128, 128);
const Color NAVY = Color(0, 0, 128);
const Color GRAY = Color(128, 128, 128);
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
	set_absolute_position(100, 200);
	set_size(fixed_size(400), fixed_size(400));
	set_background_color(Color::Red);
	end_ui_element();

	begin_ui_element("Rect");
	set_absolute_position(200, 300);
	set_size(fixed_size(400), fixed_size(400));
	set_background_color(Color::Green);
	end_ui_element();
}

void test_default_element_cliping()
{
	begin_ui_element("Red rect");
	set_absolute_position(100, 200);
	set_size(fixed_size(400), fixed_size(400));
	set_background_color(Color::Red);

	begin_ui_element("Green rect");
	set_absolute_position(200, 300);
	set_size(fixed_size(400), fixed_size(400));
	set_background_color(Color::Green);
	end_ui_element();
}

void test_elements_layouting(u32 x, u32 y, Layout layout_direction, u32 alignment_flags)
{
	begin_ui_element("Gray rect #id");
	set_absolute_position(x, y);
	set_size(fixed_size(400), fixed_size(300));
	set_space(10);
	set_padding(Padding(15));
	set_layout(layout_direction);
	set_alignment(alignment_flags);
	set_background_color(GRAY);

	begin_ui_element("Teal rect");
	set_size(fixed_size(200), fixed_size(100));
	set_background_color(TEAL);
	end_ui_element();

	begin_ui_element("Dark Orange");
	set_size(fixed_size(100), fixed_size(100));
	set_background_color(DARK_ORANGE);
	end_ui_element();

	begin_ui_element("Red Color");
	set_size(fixed_size(25), fixed_size(25));
	set_background_color(Color::Red);
	end_ui_element();

	end_ui_element();
}

void test_center_layout()
{
	test_elements_layouting(10, 10, COLUMN_LAYOUT, ALIGNMENT_CENTER);
	test_elements_layouting(500, 10, ROW_LAYOUT, ALIGNMENT_CENTER);
}

void test_horizontal_and_vertical_center_layout()
{
	test_elements_layouting(10, 10, COLUMN_LAYOUT, ALIGNMENT_TOP | ALIGNMENT_HORIZONTAL_CENTER);
	test_elements_layouting(500, 10, COLUMN_LAYOUT, ALIGNMENT_BOTTOM | ALIGNMENT_HORIZONTAL_CENTER);
	
	test_elements_layouting(1000, 10, COLUMN_LAYOUT, ALIGNMENT_LEFT | ALIGNMENT_VERTICAL_CENTER);
	test_elements_layouting(1450, 10, COLUMN_LAYOUT, ALIGNMENT_RIGHT | ALIGNMENT_VERTICAL_CENTER);

	test_elements_layouting(10, 400, ROW_LAYOUT, ALIGNMENT_TOP | ALIGNMENT_HORIZONTAL_CENTER);
	test_elements_layouting(500, 400, ROW_LAYOUT, ALIGNMENT_BOTTOM | ALIGNMENT_HORIZONTAL_CENTER);

	test_elements_layouting(1000, 400, ROW_LAYOUT, ALIGNMENT_LEFT | ALIGNMENT_VERTICAL_CENTER);
	test_elements_layouting(1450, 400, ROW_LAYOUT, ALIGNMENT_RIGHT | ALIGNMENT_VERTICAL_CENTER);
}

void test_layout()
{
	test_elements_layouting(10, 10, COLUMN_LAYOUT, ALIGNMENT_TOP | ALIGNMENT_LEFT);
	test_elements_layouting(500, 10, COLUMN_LAYOUT, ALIGNMENT_BOTTOM | ALIGNMENT_LEFT);

	test_elements_layouting(1000, 10, COLUMN_LAYOUT, ALIGNMENT_TOP | ALIGNMENT_RIGHT);
	test_elements_layouting(1450, 10, COLUMN_LAYOUT, ALIGNMENT_BOTTOM | ALIGNMENT_RIGHT);

	test_elements_layouting(10, 400, ROW_LAYOUT, ALIGNMENT_TOP | ALIGNMENT_LEFT);
	test_elements_layouting(500, 400, ROW_LAYOUT, ALIGNMENT_BOTTOM | ALIGNMENT_LEFT);

	test_elements_layouting(1000, 400, ROW_LAYOUT, ALIGNMENT_TOP | ALIGNMENT_RIGHT);
	test_elements_layouting(1450, 400, ROW_LAYOUT, ALIGNMENT_BOTTOM | ALIGNMENT_RIGHT);
}

void test_elements_size_filling(u32 x, u32 y, Layout layout_direction, u32 alignment_flags)
{
	begin_ui_element("Gray rect #id");
	set_absolute_position(x, y);
	set_size(fit_size(), fit_size());
	set_layout(layout_direction);
	set_alignment(alignment_flags);
	set_background_color(GRAY);

	begin_ui_element("Teal rect");
	set_size(fixed_size(300), fixed_size(200));
	set_background_color(TEAL);
	end_ui_element();

	begin_ui_element("Dark Orange");
	set_size(fixed_size(100), fixed_size(100));
	set_background_color(DARK_ORANGE);
	end_ui_element();

	end_ui_element();
}

void test_center_layout_with_filled_size()
{
	test_elements_size_filling(10, 10, COLUMN_LAYOUT, ALIGNMENT_CENTER);
	test_elements_size_filling(500, 10, ROW_LAYOUT, ALIGNMENT_CENTER);
}

void test_horizontal_and_vertical_center_layout_with_filled_size()
{
	test_elements_size_filling(10, 10, COLUMN_LAYOUT, ALIGNMENT_TOP | ALIGNMENT_HORIZONTAL_CENTER);
	test_elements_size_filling(500, 10, COLUMN_LAYOUT, ALIGNMENT_BOTTOM | ALIGNMENT_HORIZONTAL_CENTER);

	test_elements_size_filling(1000, 10, COLUMN_LAYOUT, ALIGNMENT_LEFT | ALIGNMENT_VERTICAL_CENTER);
	test_elements_size_filling(1450, 10, COLUMN_LAYOUT, ALIGNMENT_RIGHT | ALIGNMENT_VERTICAL_CENTER);

	test_elements_size_filling(10, 400, ROW_LAYOUT, ALIGNMENT_TOP | ALIGNMENT_HORIZONTAL_CENTER);
	test_elements_size_filling(500, 400, ROW_LAYOUT, ALIGNMENT_BOTTOM | ALIGNMENT_HORIZONTAL_CENTER);

	test_elements_size_filling(1000, 400, ROW_LAYOUT, ALIGNMENT_LEFT | ALIGNMENT_VERTICAL_CENTER);
	test_elements_size_filling(1450, 400, ROW_LAYOUT, ALIGNMENT_RIGHT | ALIGNMENT_VERTICAL_CENTER);
}

void test_layout_with_filled_size()
{
	test_elements_size_filling(10, 10, COLUMN_LAYOUT, ALIGNMENT_TOP | ALIGNMENT_LEFT);
	test_elements_size_filling(500, 10, COLUMN_LAYOUT, ALIGNMENT_BOTTOM | ALIGNMENT_LEFT);

	test_elements_size_filling(1000, 10, COLUMN_LAYOUT, ALIGNMENT_TOP | ALIGNMENT_RIGHT);
	test_elements_size_filling(1450, 10, COLUMN_LAYOUT, ALIGNMENT_BOTTOM | ALIGNMENT_RIGHT);

	test_elements_size_filling(10, 400, ROW_LAYOUT, ALIGNMENT_TOP | ALIGNMENT_LEFT);
	test_elements_size_filling(500, 400, ROW_LAYOUT, ALIGNMENT_BOTTOM | ALIGNMENT_LEFT);

	test_elements_size_filling(1000, 400, ROW_LAYOUT, ALIGNMENT_TOP | ALIGNMENT_RIGHT);
	test_elements_size_filling(1450, 400, ROW_LAYOUT, ALIGNMENT_BOTTOM | ALIGNMENT_RIGHT);
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
	
	//test_center_layout();
	//test_horizontal_and_vertical_center_layout();
	//test_layout();

	//test_center_layout_with_filled_size()
	//test_horizontal_and_vertical_center_layout_with_filled_size();
	//test_layout_with_filled_size();

	//set_layout(ROW_LAYOUT);

	//begin_ui_element("Gray rect #id");
	//set_alignment(ALIGNMENT_CENTER);
	//set_absolute_position(20, 20);
	//set_size(fixed_size(500), fixed_size(500));
	//set_background_color(TEAL);

	//begin_ui_element("Gray rect #id");
	//set_size(filled_size(), filled_size());
	//set_background_color(Color::Black);
	//set_space(10);
	//text("Andrey Klimenko IMGUI");
	//text("Andrey Klimenko IMGUI 1");
	//text("Andrey Klimenko IMGUI 2");
	//
	//end_ui_element();


	//begin_ui_element("Red box");
	//set_absolute_position(-10, 0);
	//set_size(fixed_size(100), fixed_size(100));
	//
	//if (ui_element_hovered()) {
	//	set_background_color(Color::Red);
	//} else {
	//	set_background_color(Color::Black);
	//}
	//
	//begin_ui_element("Blue box");
	//set_absolute_position(-20, 0);
	//set_size(fixed_size(70), fixed_size(40));
	//set_background_color(Color::Blue);
	//end_ui_element();
	//
	//end_ui_element();
	//
	//end_ui_element();

	//set_alignment(ALIGNMENT_CENTER);

	//begin_ui_element("Red rect");
	//auto red_ui_element = get_ui_element();
	//auto red_rect = red_ui_element->get_rect();

	//set_absolute_position(100, 20);
	//set_size(fixed_size(400), fixed_size(400));
	//set_background_color(Color::Red);
	//end_ui_element(); // Red

	//begin_ui_element("Green rect");
	//set_absolute_position(red_rect.right() + 10, 20);
	//set_size(fixed_size(400), fixed_size(400));
	//set_background_color(Color::Green);
	//end_ui_element(); // Green


	//begin_ui_element("Test");
	//set_absolute_position(30, 30);
	//set_background_color(Color::Red);

	//begin_ui_element("Rect 1");
	//set_size(fixed_size(100), fixed_size(20));
	//set_background_color(Color::Green);
	//end_ui_element();

	//begin_ui_element("Rect 2");
	//set_size(fixed_size(100), fixed_size(20));
	//set_background_color(Color::Green);
	//end_ui_element();

	//end_ui_element();

	//begin_ui_element("Main rect");
	//set_absolute_position(30, 30);
	////set_size(fit_size(), fit_size());
	//set_size(fixed_size(100), fit_size());
	//set_padding(Padding(10));
	//set_background_color(Color::Cyan);

	//for (int i = 0; i < 3; i++) {
	//	begin_ui_element("Red #id");
	//	//set_size(fixed_size(100), fixed_size(30));
	//	set_size(grow_size(), fixed_size(30));
	//	set_background_color(Color::Red);
	//	end_ui_element();
	//}

	//end_ui_element();

	//begin_ui_element("Main rect");
	//set_absolute_position(10, 10);
	//set_size(fixed_size(600), fixed_size(600));
	//set_layout(ROW_LAYOUT);
	//set_alignment(ALIGNMENT_VERTICAL_CENTER);
	//set_background_color(Color(45));
	//set_space(0);

	//begin_ui_element("Red");
	//set_size(fixed_size(200), fixed_size(200));
	//set_background_color(Color::Red);
	//end_ui_element();

	//begin_ui_element("Green");
	//set_size(fixed_size(100), fixed_size(100));
	//set_background_color(Color::Green);
	//end_ui_element();

	//begin_ui_element("Orange");
	//set_size(fixed_size(50), fixed_size(50));
	//set_background_color(Color::Blue);
	//end_ui_element();

	//end_ui_element();

	end_frame();
}