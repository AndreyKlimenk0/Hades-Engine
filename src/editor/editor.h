#ifndef EDITOR_H
#define EDITOR_H

#include <stdlib.h>
#include <d2d1.h>

#include "../render/directx.h"
#include "../libs/ds/array.h"
#include "../libs/str.h"

struct Callback;

enum Element_Type {
	ELEMENT_TYPE_UNDEFINED,
	ELEMENT_TYPE_BUTTON,
	ELEMENT_TYPE_LIST_BOX,
	ELEMENT_TYPE_WINDOW,
};


struct Element {
	Element() : x(0), y(0), width(0), height(9) {}
	Element(int x, int y) : x(x), y(y), width(0), height(9) {}
	Element(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) {}
	Element_Type type = ELEMENT_TYPE_UNDEFINED;
	int x;
	int y;
	int width;
	int height;

	virtual void draw() = 0;
	virtual void handle_event() = 0;
};

enum Button_Place_Type {
	BUTTON_IS_PLASED_BY_ITSELF,
	BUTTON_IS_PLASED_BY_WINDOW,
};

struct Button : Element {
	Button() { type = ELEMENT_TYPE_BUTTON; }
	Button(const char *text);
	Button(int x, int y, const char *_text);
	Button(int _x, int _y, int _width, int _height, const char *_text);
	Button(int x, int y, ID2D1Bitmap *image, float _scale = 1.0f);
	Button(int x, int y, int width, int height, ID2D1Bitmap *image, float _scale = 1.0f);

	Callback *callback = NULL;
	ID2D1Bitmap *image = NULL;
	
	bool cursor_on_button = false;
	float scale;
	String text;
	//Button_Place_Type place_type = BUTTON_IS_PLASED_BY_ITSELF;

	void draw();
	void handle_event();
};

enum List_Box_State {
	LIST_BOX_IS_PICKED_UP,
	LIST_BOX_IS_PICKING_UP,
	LIST_BOX_IS_DROPPING,
	LIST_BOX_IS_DROPPED,
};

struct List_Box: Element {
	List_Box(int x, int y, int width, int height);
	List_Box(int x, int y, int width, int height, const char *lable, Array<String> *items);

	String label;
	Button drop_button;
	List_Box_State list_state = LIST_BOX_IS_PICKED_UP;
	Array<String> list_items;
	int list_box_size = 100; // in pixels
	int list_box_x = 0;
	int list_box_y = 0;

	void draw();
	void update();
	void handle_event() {};
	void on_button_click();
	void calculate_list_box_size();
};

struct Window : Element {
	Window(int x, int y, int width, int height);
	
	Button close_button;

	Array<Element *> elements;
	Array<List_Box *> list_boxies;

	bool close_window = false;
	void draw();
	void handle_event();
	void update();
	void add_button(Button *button);
	void add_element(Element *element);
	void close() { close_window = true; };
};

struct Editor {
	Array<Window *> windows;

	void init();
	void handle_event();
	void draw();
	void update();
	void add_window(Window *window);
};

#endif