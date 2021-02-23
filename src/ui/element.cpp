#include "element.h"
#include "../sys/sys_local.h"
#include "../libs/color.h"



void Window::draw()
{
	directx_render.fill_rect(x, y, width, height, background_color);
	directx_render.draw_rect(x, y, width, height, background_color);
	
	directx_render.fill_rect(x, y, width, 30, Color(255, 255, 255));
	directx_render.draw_rect(x, y, width, 30, Color(255, 255, 255));

}

Button::Button(int x, int y, int width, int height, const char * _text, Color & _background_color, Color & _stroke_color, ID2D1StrokeStyle * _stroke_style) : Element(x, y, width, height)
{
	text = (char *)_text;
	background_color = _background_color;
	stroke_color = _stroke_color;
	stroke_style = _stroke_style ? _stroke_style : directx_render.create_round_stroke_style();
}

Button::~Button()
{
	DELETE_PTR(text);
}

void Button::draw()
{
	directx_render.fill_rect(x, y, width, height, background_color);
	directx_render.draw_rect(x, y, width, height, stroke_color, stroke_style);
}

void Editor::add_element(Element *element)
{
	elements.push(element);
}

void Editor::draw()
{
	Element *e = NULL;
	FOR(elements, e) {
		e->draw();
	}
}

void Editor::init()
{
	int k = 49;
	Color color = Color(k, k, k);
	Window *w = new Window(10, 10, 200, 500, color);
	add_element(w);

	//
	//Button *b1 = new Button(10, 10, 50, 20, "alalla",  Color::Red, Color::Black);
	//Button *b2 = new Button(10, 100, 50, 20, "alalla", color, color);
	//Button *b3 = new Button(10, 200, 50, 20, "alalla", color, color);
	//Button *b4 = new Button(10, 300, 50, 20, "alalla", color, color);

	//add_element(b1);
	//add_element(b2);
	//add_element(b3);
	//add_element(b4);
}
