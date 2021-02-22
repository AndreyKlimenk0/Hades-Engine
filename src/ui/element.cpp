#include "element.h"
#include "../sys/sys_local.h"
#include "../libs/color.h"


Button::Button(int _x, int _y, int _width, int _height, const char * _text, Color & _background_color, Color & _stroke_color, ID2D1StrokeStyle * _stroke_style)
{
	x = _x;
	y = _y;
	width = _width;
	height = _height;
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


void Editor::draw()
{
	Element *e = NULL;
	FOR(elements, e) {
		e->draw();
	}
}

void Editor::add_element(Element *element)
{
	elements.push(element);
}

void Editor::test()
{
	Button *b1 = new Button(10, 10, 50, 20, "alalla", Color::Black, Color::Red);
	Button *b2 = new Button(10, 100, 50, 20, "alalla", Color::Black, Color::Red);
	Button *b3 = new Button(10, 200, 50, 20, "alalla", Color::Black, Color::Red);
	Button *b4 = new Button(10, 300, 50, 20, "alalla", Color::Black, Color::Red);
	add_element(b1);
	add_element(b2);
	add_element(b3);
	add_element(b4);
}