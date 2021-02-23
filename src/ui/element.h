#ifndef ELEMENT_H
#define ELEMENT_H

#include <stdlib.h>
#include <d2d1.h>

#include "../render/base.h"
#include "../libs/ds/array.h"

struct Element {
	Element(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) {}
	int x;
	int y;
	int width;
	int height;

	virtual void draw() = 0;
};

struct Window : Element {
	Window(int x, int y, int width, int height, Color &background_color) : Element(x, y, width, height), background_color(background_color) {}
	
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
	void init();
	void add_element(Element *element);
};

#endif