#ifndef EDITOR_H
#define EDITOR_H

#include <stdlib.h>
#include <d2d1.h>

#include "../render/directx.h"
#include "../libs/str.h"
#include "../libs/ds/array.h"
#include "../libs/ds/linked_list.h"
#include "../libs/os/camera.h"


struct Event;
struct Callback;


struct Button_Theme {
	int border_about_text = 6;
	int text_shift = 0;
	float rounded_border = 3.0f;
	Color stroke_color = Color::Black;
	Color color = Color(0, 75, 168);
	Color hover_color = Color(0, 60, 168);
};

struct List_Box_Theme {
	int border_about_text = 0;
	int list_box_shift = 2;
	int header_width = 200;
	int header_height = 20;
	float rounded_border = 4.0f;
	Color color = Color(74, 82, 90);
};

struct Input_Filed_Theme {
	int width = 200;
	int height = 20;
	int shift_caret_from_left = 3;
	float caret_height_in_percents = 80.0f;
	float rounded_border = 4.0f;
	Color color = Color(74, 82, 90);
};

struct Window_Theme {
	int   header_height = 20;
	int   shift_cross_button_from_left_on = 5;
	float window_rounded_factor = 6.0f;
	float header_botton_height_in_percents = 50;
	Color header_color = Color(10, 7, 7);
	Color color = Color(36, 39, 43);
};

enum Element_Type {
	ELEMENT_TYPE_UNDEFINED,
	ELEMENT_TYPE_BUTTON,
	ELEMENT_TYPE_LIST_BOX,
	ELEMENT_TYPE_INPUT_FIELD,
	ELEMENT_TYPE_WINDOW,
};

const int ELEMENT_HOVER = 0x1;
const int ELEMENT_FOCUSED = 0x2;

struct Element {
	Element() : x(0), y(0), width(0), height(9) {}
	Element(int x, int y) : x(x), y(y), width(0), height(9) {}
	Element(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) {}

	Element_Type type = ELEMENT_TYPE_UNDEFINED;
	int flags = 0;
	int x;
	int y;
	int width;
	int height;

	virtual void draw() = 0;
	virtual void handle_event(Event *event) = 0;
};

struct Caret : Element {
	Caret() {};
	Caret(int x, int y, int blink_time = 500) : Element(x, y, 1, direct_write.glyph_height), blink_time(blink_time) {} // blink time are a time in milliseconds

	int blink_time;
	float fx;
	float fy;
	float fwidth = 1;
	float fheight;

	void handle_event(Event *event) {};
	void draw();
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

	ID2D1Bitmap *image = NULL;
	Callback *callback = NULL;


	bool cursor_on_button = false;
	float scale;

	Rect text_rect;
	Rect button_rect;

	String text;
	Button_Theme theme;
	//Button_Place_Type place_type = BUTTON_IS_PLASED_BY_ITSELF;

	void draw();
	void handle_event(Event *event);
	//void handle_event(Event *event) {};
	void calculate_rects();
};

enum List_Box_State {
	LIST_BOX_IS_PICKED_UP,
	LIST_BOX_IS_PICKING_UP,
	LIST_BOX_IS_DROPPING,
	LIST_BOX_IS_DROPPED,
};

struct List_Box : Element {
	List_Box(int x, int y, Array<String> *items, const char *label);

	List_Box_State list_state = LIST_BOX_IS_PICKED_UP;
	int list_box_size;
	int button_height;
	int text_x;
	int text_y;

	List_Box_Theme theme;
	String label;
	Button drop_button;
	Array<Button> list_items;

	Button *button = NULL;
	String *current_chosen_item_text = NULL;

	void draw();
	void handle_event(Event *event) {};
	void on_drop_button_click();
	void on_item_list_click();
};

enum Input_Data_Type {
	INPUT_DATA_INT,
	INPUT_DATA_FLOAT,
	INPUT_DATA_STRING,
};

struct Input_Filed : Element {
	Input_Filed(int x, int y);

	int max_text_width;
	int text_width;
	int caret_index_in_text;

	Input_Filed_Theme theme;
	Caret caret;
	String text;
	//Array<char> text_buffer;
	void handle_event(Event *event);
	void draw();
};

struct Window : Element {
	Window(int x, int y, int width, int height);

	Button close_button;

	Array<Element *> elements;
	Array<List_Box *> list_boxies;

	bool close_window = false;
	void draw();
	void handle_event(Event *event);
	void handle_event() {};
	void update();
	void add_button(Button *button);
	void add_element(Element *element);
	void close() { close_window = true; };
};

struct Editor {
	Free_Camera free_camera;
	Array<Window *> windows;

	void init();
	void handle_event(Event *event);
	void draw();
	void update();
	void add_window(Window *window);
};

extern Editor editor;

inline void handle_event_for_editor(Event *event)
{
	editor.handle_event(event);
}

#endif