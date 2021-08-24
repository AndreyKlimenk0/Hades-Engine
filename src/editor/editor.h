#ifndef EDITOR_H
#define EDITOR_H

#include <stdlib.h>
#include <d2d1.h>

#include "../render/directx.h"
#include "../libs/str.h"
#include "../libs/ds/array.h"
#include "../libs/ds/linked_list.h"
#include "../libs/os/camera.h"
#include "../sys/sys_local.h"


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
	int header_width = 150;
	int header_height = 20;
	int list_box_shift_from_text = 4;
	float rounded_border = 4.0f;
	Color color = Color(74, 82, 90);
};

struct Input_Filed_Theme {
	int width = 150;
	int height = 20;
	int shift_caret_from_left = 4;
	int field_shift_from_text = 4;
	float caret_height_in_percents = 80.0f;
	float rounded_border = 4.0f;
	Color color = Color(74, 82, 90);
};

struct Window_Theme {
	int place_between_elements = 20;
	int shift_element_from_left = 15;
	int header_height = 20;
	int shift_cross_button_from_left_on = 5;
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
	ELEMENT_TYPE_EDIT_FIELD,
	ELEMENT_TYPE_WINDOW,
};

const int ELEMENT_HOVER   = 0x1;
const int ELEMENT_FOCUSED = 0x2;

const int WINDOW_FULL_HEIGHT = 0x4;
const int WINDOW_FULL_WIDTH  = 0x8;
const int WINDOW_LEFT   = 0x10;
const int WINDOW_RIGHT  = 0x20;
const int WINDOW_CENTER = 0x40;
const int WINDOW_AUTO_WIDTH = 0x80;	

struct Point {
	Point() : x(0), y(0) {}
	Point(int x, int y) : x(x), y(y) {}
	int x;
	int y;
};

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
	Caret(int x, int y) : Element(x, y, 1, direct_write.glyph_height) {} // blink time are a time in milliseconds
	Caret(float x, float y, float height);

	bool use_float_rect = false;
	
	int blink_time = 500;
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
	Button(const char *_text, int _x = 0, int _y = 0, int _width = 0, int _height = 0, Button_Theme *_button_theme = NULL);
	Button(int _x, int _y, ID2D1Bitmap *_image, float _scale = 1.0f);
	~Button();

	ID2D1Bitmap *image = NULL;
	Callback *callback = NULL;

	bool cursor_on_button = false;
	float scale;

	Point text_position;

	String text;
	Button_Theme theme;

	void draw();
	void handle_event(Event *event);

	DELETE_COPING(Button);
};

enum Label_Side {
	LABEL_ON_UP_SIDE,
	LABEL_ON_LEFT_SIDE,
	LABEL_ON_RIGHT_SIDE,
};

struct Label : Element {
	Label() {}
	Label(int _x, int _y, const char *_text);

	Label_Side label_side;
	String text;

	void handle_event(Event *event) {};
	void draw();
};

struct Input_Field : Element {
	Input_Field() { type = ELEMENT_TYPE_INPUT_FIELD; }
	Input_Field(int x, int y, int width, int height) : Element(x, y, width, height) {};
	Label label;

	void handle_event(Event *event) {}
	void draw() {}
};

enum List_Box_State {
	LIST_BOX_IS_PICKED_UP,
	LIST_BOX_IS_PICKING_UP,
	LIST_BOX_IS_DROPPING,
	LIST_BOX_IS_DROPPED,
};

struct List_Box : Input_Field {
	List_Box(int _x, int _y, const char *_label);
	List_Box(int x, int y, Array<String> *items, const char *label);
	~List_Box();

	Button *button = NULL;
	Button *drop_button = NULL;
	String *current_chosen_item_text = NULL;
	
	List_Box_State list_state = LIST_BOX_IS_PICKED_UP;
	
	int list_box_size;
	int button_height;
	int text_x;
	int text_y;

	List_Box_Theme theme;
	Button_Theme button_theme;
	
	Array<Button *> list_items;

	void draw();
	void on_list_item_click();
	void on_drop_button_click();
	void push_item(const char *item_text);
	void handle_event(Event *event);
};

enum Edit_Data_Type {
	EDIT_DATA_INT,
	EDIT_DATA_FLOAT,
	EDIT_DATA_STRING,
};

struct Edit_Field : Input_Field {
	Edit_Field(int _x, int _y, const char *_label_text, Edit_Data_Type _edit_data_type);

	int max_text_width;
	int text_width;
	int caret_index_in_text;
	
	Edit_Data_Type edit_data_type;

	Input_Filed_Theme theme;
	Caret caret;
	String text;

	void handle_event(Event *event);
	void draw();
};

struct Window : Element {
	Window();
	Window(int x, int y, int width, int height);
	~Window();

	Point next_place;

	Button *close_button = NULL;

	Array<Element *> elements;
	Array<Input_Field *> input_fields;
	Array<List_Box *> list_boxies;

	bool close_window = false;

	void draw();
	void update();
	void make_header();
	void handle_event(Event *event);
	void add_element(Element *element);
	void go_next_element_place(Element * element);
	void aligning_input_fields();
	void close() { close_window = true; };
};

struct Editor {
	Free_Camera free_camera;
	Array<Window *> windows;

	Window *current_window = NULL;
	Button *current_button = NULL;
	List_Box *current_list_box = NULL;
	Edit_Field *current_edit_field = NULL;

	void init();
	void handle_event(Event *event);
	void draw();
	void update();
	void add_window(Window *window);

	void make_button(const char *text, Callback *callback = NULL);
	void make_list_box(const char *text);
	void make_edit_field(const char *label);
	void make_window(int flags);

	void add_item(const char *item_text);
};

extern Editor editor;

inline void handle_event_for_editor(Event *event)
{
	editor.handle_event(event);
}

#endif