#include <assert.h>

#include "editor.h"
#include "../sys/sys_local.h"
#include "../libs/color.h"
#include "../libs/os/input.h"



struct Button_Setup {
	Color &_stroke_color;
	Color &_background_color;
	ID2D1StrokeStyle *_stroke_style = NULL;
};

struct Window_Setup {
	int   header_height = 20;
	int   shift_cross_button_from_left_on = 5;
	float window_rounded_factor = 6.0f;
	float header_botton_height_in_percents = 50;
	Color header_color = Color::White;
};

static Window_Setup window_draw_info;

//Rect::operator D2D1_RECT_U()
//{
//	D2D1_RECT_U rect;
//	rect.left = x;
//	rect.top = y;
//	rect.right = width;
//	rect.bottom = height;
//	return rect;
//}

float calculate_scale_based_on_percent_from_header_height(int header_height, int image_height, float percents)
{
	float needed_height = (int)(header_height * percents / 100.0f);
	float ratios = needed_height / image_height;
	return ratios;
}

inline u32 get_width_from_bitmap(const ID2D1Bitmap *bitmap)
{
	D2D1_SIZE_U size = bitmap->GetPixelSize();
	return size.width;
}

inline u32 get_height_from_bitmap(const ID2D1Bitmap *bitmap)
{
	D2D1_SIZE_U size = bitmap->GetPixelSize();
	return size.height;
}

Window::Window(int x, int y, int width, int height, Color &background_color) : Element(x, y, width, height + window_draw_info.header_height), background_color(background_color) {}

void Window::draw()
{
	float factor = window_draw_info.window_rounded_factor;
	direct2d.draw_rounded_rect(x, y, width, height + window_draw_info.header_height, factor, factor, Color(255, 255, 255));
	direct2d.draw_rounded_rect(x, y + window_draw_info.header_height, width, height, factor, factor, background_color);
	direct2d.fill_rect(x, y + window_draw_info.header_height, width, height - window_draw_info.header_height, background_color);


	ID2D1Bitmap *cross_bitmap = NULL;
	load_bitmap_from_file("E:\\andrey\\dev\\hades\\data\\editor\\cross.png", 1, 1, &cross_bitmap);

	float scale = calculate_scale_based_on_percent_from_header_height(window_draw_info.header_height, get_height_from_bitmap(cross_bitmap), 70);

	D2D1_SIZE_U size = cross_bitmap->GetPixelSize();

	D2D1_RECT_F rect;
	rect.left = x + width - (size.width * scale) - window_draw_info.shift_cross_button_from_left_on;
	rect.top = (y + window_draw_info.header_height / 2) - ((size.height * scale) / 2);
	rect.right = rect.left + size.width;
	rect.bottom = rect.top + size.height;

	direct2d.draw_bitmap(rect, cross_bitmap, scale);

}

Button::Button(int x, int y, int width, int height, const char * _text, Color & _background_color, Color & _stroke_color) : Element(x, y, width, height)
{
	text = (char *)_text;
	background_color = _background_color;
	stroke_color = _stroke_color;
}

Button::~Button()
{
	DELETE_PTR(text);
}

void Button::draw()
{
	direct2d.fill_rect(x, y, width, height, background_color);
	direct2d.draw_rect(x, y, width, height, stroke_color);
}

void Editor::add_element(Element *element)
{
	elements.push(element);
}

void Editor::draw()
{

	direct2d.begin_draw();
	Element *e = NULL;
	FOR(elements, e) {
		e->draw();
	}

	//direct2d.end_draw();
	direct2d.render_target->EndDraw();
}

void Editor::init()
{
	int k = 49;
	Color color = Color(k, k, k);
	Window *w = new Window(10, 10, 200, 500, color);
	Window *w2 = new Window(300, 10, 200, 500, color);
	add_element(w);
	add_element(w2);

	//
	//Button *b1 = new Button(20, 20, 70, 30, "alalla",  Color::Red, Color::Black);
	//Button *b2 = new Button(10, 100, 50, 20, "alalla", color, color);
	//Button *b3 = new Button(10, 200, 50, 20, "alalla", color, color);
	//Button *b4 = new Button(10, 300, 50, 20, "alalla", color, color);

	//add_element(b1);
	//add_element(b2);
	//add_element(b3);
	//add_element(b4);
}

void Editor::handle_input()
{
	Element *e = NULL;
	FOR(elements, e) {
		if (Mouse_Input::x >= e->x && Mouse_Input::x <= (e->x + e->width) && Mouse_Input::y >= e->y && Mouse_Input::x <= (e->y + e->height)) {
			print("COLLISION x = {}, y = {}", Mouse_Input::x, Mouse_Input::y);
			//Mouse_Input::x = 0;
			//Mouse_Input::y = 0;
			Key_Input::keys[VK_LBUTTON] = false;
		}
	}
}