#ifndef ELEMENT_H
#define ELEMENT_H

#include <stdlib.h>
#include <d2d1.h>

#include "../render/base.h"
#include "../libs/ds/array.h"

struct Element {
	int x;
	int y;
	int width;
	int height;

	virtual void draw() = 0;
};

struct Window : Element {
	Color background_color;
	Array<Element *> window_elements;
	
	void draw();
};

struct Button : Element {
	Button(int _x, int _y, int _width, int _height, const char *_text, Color &_background_color, Color &_stroke_color, ID2D1StrokeStyle *_stroke_style = NULL);
	~Button();

	char *text = NULL;
	Color background_color;
	Color stroke_color;
	ID2D1StrokeStyle *stroke_style = NULL;

	void draw();
};

struct Editor {
	Array<Element *> elements;

	void draw();
	void test();
	void add_element(Element *element);
};

#endif