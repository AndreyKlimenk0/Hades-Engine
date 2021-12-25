#ifndef EDITOR_H
#define EDITOR_H

#include <stdlib.h>
#include <d2d1.h>

#include "../render/directx.h"
#include "../libs/str.h"
#include "../libs/os/camera.h"
#include "../libs/ds/array.h"
#include "../libs/ds/dict.h"
#include "../libs/ds/linked_list.h"
#include "../libs/ds/hash_table.h"
#include "../sys/sys_local.h"


struct Event;
struct Callback;
struct Form;
struct Picked_Panel;


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
	int header_width = 180;
	int header_height = 20;
	int list_box_shift_from_text = 4;
	float rounded_border = 4.0f;
	Color color = Color(74, 82, 90);
};

struct Edit_Field_Theme {
	int width = 180;
	int height = 20;
	int shift_caret_from_left = 4;
	int field_shift_from_text = 4;
	float caret_height_in_percents = 80.0f;
	float rounded_border = 4.0f;
	Color color = Color(74, 82, 90);
};

struct Window_Theme {
	bool draw_window_name_in_header = true;
	int offset_from_window_top = 20;
	int shift_element_from_window_side = 20;
	int place_between_elements = 10;
	int shift_element_from_left = 15;
	int header_height = 25;
	int shift_cross_button_from_left_on = 5;
	float window_rounded_factor = 6.0f;
	float header_botton_height_in_percents = 50;
	//Color header_color = Color(10, 7, 7);
	//Color header_color = Color(30, 30, 30);
	Color header_color = Color(74, 82, 90);
	Color color = Color(36, 39, 43);
	//Color color = Color(36, 39, 43);
};

enum Element_Type {
	ELEMENT_TYPE_UNDEFINED,
	ELEMENT_TYPE_BUTTON,
	ELEMENT_TYPE_PANEL_LIST_BOX,
	ELEMENT_TYPE_LIST_BOX,
	ELEMENT_TYPE_INPUT_FIELD,
	ELEMENT_TYPE_EDIT_FIELD,
	ELEMENT_TYPE_VECTOR3_EDIT_FIELD,
	ELEMENT_TYPE_PICKED_PANEL,
	ELEMENT_TYPE_FORM,
	ELEMENT_TYPE_WINDOW,
};

const int ELEMENT_HOVER   = 0x1;
const int ELEMENT_FOCUSED = 0x2;

const int WINDOW_FULL_HEIGHT = 0x4;
const int WINDOW_FULL_WIDTH  = 0x8;
const int WINDOW_LEFT        = 0x10;
const int WINDOW_RIGHT       = 0x20;
const int WINDOW_CENTER      = 0x40;
const int WINDOW_AUTO_WIDTH  = 0x80;	
const int WINDOW_WITH_HEADER = 0x100;
const int WINDOW_HEADER_HOVER = 0x200;

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
	virtual void set_position(int _x, int y_) = 0;
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

	void draw();
	void handle_event(Event *event) {};
	void set_position(int _x, int _y);
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
	void set_position(int _x, int _y);

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

	void draw();
	void handle_event(Event *event) {};
	void set_position(int _x, int _y) { x = _x, y = _y; }
};

struct Input_Field : Element {
	Input_Field() { type = ELEMENT_TYPE_INPUT_FIELD; }
	Input_Field(int x, int y, int width, int height) : Element(x, y, width, height) {};
	Label label;

	void draw() { assert(false); }
	void handle_event(Event *event) { assert(false); }
	void set_position(int _x, int _y) { assert(false); }
};

enum List_Box_State {
	LIST_BOX_IS_PICKED_UP,
	LIST_BOX_IS_PICKING_UP,
	LIST_BOX_IS_DROPPING,
	LIST_BOX_IS_DROPPED,
};

struct List_Box : Input_Field {
	
	List_Box(const char *_label, int _x = 0, int _y = 0);
	~List_Box();

	Button *button = NULL;
	Button *drop_button = NULL;
	String *current_chosen_item_text = NULL;
	
	List_Box_State list_state = LIST_BOX_IS_PICKED_UP;
	
	int text_x;
	int text_y;
	int header_width;
	int list_box_size;
	int button_height;
	int drop_button_image_width;

	List_Box_Theme theme;
	Button_Theme button_theme;
	
	Array<Button *> item_list;
	Hash_Table<String, int> string_enum_pairs;

	void draw();
	void on_list_item_click();
	void on_drop_button_click();
	void handle_event(Event *event);
	void add_item(const char *item_text);
	void add_item(const char *string, int enum_value);
	
	int get_chosen_enum_value();
	String *get_chosen_item_string();
	
	void set_position(int _x, int _y);
};

struct Panel_List_Box : List_Box {
	Panel_List_Box(int _x, int _y, const char *_label);

	Picked_Panel *last_picked_panel = NULL;

	Hash_Table<String, Picked_Panel *> string_picked_panel_pairs;

	void on_list_item_click();
	void add_item(const char *string, int enum_value, Picked_Panel *form);
	Picked_Panel *get_picked_panel();
};

struct Picked_Panel : Element {
	Picked_Panel() { type = ELEMENT_TYPE_PICKED_PANEL; }
	~Picked_Panel();

	bool draw_panel = false;

	Array<Input_Field *> input_fields;
	Array<List_Box *> list_boxies;

	void draw();
	void handle_event(Event *event);
	void add_field(Input_Field *input_field);
	void set_position(int _x, int _y);
};

enum Edit_Data_Type {
	EDIT_DATA_INT,
	EDIT_DATA_FLOAT,
	EDIT_DATA_STRING,
};

struct Edit_Field : Input_Field {
	Edit_Field() { type = ELEMENT_TYPE_EDIT_FIELD; }
	Edit_Field(const char *_label_text, Edit_Data_Type _edit_data_type, Edit_Field_Theme *edit_theme = NULL, int _x = 0, int _y = 0);
	//Edit_Field(int _x, int _y, const char *_label_text, int *int_value);
	Edit_Field(const char *_label_text, float *float_value, Edit_Field_Theme *edit_theme = NULL, int _x = 0, int _y = 0);
	//Edit_Field(int _x, int _y, const char *_label_text, String *string_value);

	bool edit_itself_data;

	int max_text_width;
	int text_width;
	int caret_index_in_text; // this caret index specifies at character is placed befor the caret.
	int caret_index_for_inserting; // this caret index specifies at character is placed after the caret.
	int field_width;
	
	Edit_Data_Type edit_data_type;

	Edit_Field_Theme theme;

	union {
		union {
			int *int_value;
			float *float_value;
		} not_itself_data;

		union {
			int int_value;
			float float_value;
		} itself_data;
	} edit_data;

	Caret caret;
	String text;

	void draw();
	void handle_event(Event *event);
	void set_position(int _x, int _y);
	void set_caret_position_on_mouse_click(int mouse_x, int mouse_y);
	void set_text(const char *_text);
	void update_edit_data(const char *text);

	int get_int_value();
	float get_float_value();
};

struct Vector3_Edit_Field : Input_Field {
	Vector3_Edit_Field() { type = ELEMENT_TYPE_VECTOR3_EDIT_FIELD; }
	Vector3_Edit_Field(const char *_label, int _x = 0, int _y = 0);
	Vector3_Edit_Field(const char *_label, Vector3 *vec3, int _x = 0, int _y = 0);
	~Vector3_Edit_Field() {};

	Edit_Field x;
	Edit_Field y;
	Edit_Field z;

	void draw();
	void handle_event(Event *event);
	void set_position(int _x, int _y);
	void get_vector(Vector3 *vector);
};

struct Form : Element {
	Form();

	String label;

	Button *submit = NULL;
	Callback *callback = NULL;
	Array<Input_Field *> input_fields;
	
	void draw();
	void on_submit();
	void fill_args(Args *args, Array<Input_Field *> *fields);
	void handle_event(Event *event);
	void add_field(Input_Field *input_field);
	void set_position(int _x, int _y);
};

enum Alignment {
	LEFT_ALIGNMENT,
	RIGHT_ALIGNMENT
};

enum Place {
	PLACE_HORIZONTALLY,
	PLACE_VERTICALLY,
	PLACE_HORIZONTALLY_AND_IN_MIDDLE,
};

struct Window : Element {
	Window();
	Window(int _width, int _height, int _flags);
	Window(int _x, int _y, int _width, int _height, int _flags);
	~Window();

	bool can_move = false;
	bool window_active = true;

	Place place = PLACE_VERTICALLY;
	Alignment aligment = RIGHT_ALIGNMENT;

	Point next_place;
	Point last_mouse_position;
	Point header_text_position;

	Button *close_button = NULL;

	String name;

	Window_Theme theme;

	Array<Element *> elements;
	Array<Input_Field *> input_fields;
	Array<List_Box *> list_boxies;
	Array<Window *> windows_will_be_disabled;

	void draw();
	void update();
	void make_header();
	void make_window(int _x, int _y, int _width, int _height, int _flags, Window_Theme *_theme);
	void handle_event(Event *event);
	
	void add_header_text(const char *);
	void add_element(Element *element);
	
	void calculate_current_place(Element *element);
	void calculate_next_place(Element *element);
	
	void window_callback();

	void move(int x_delta, int y_delta);

	void set_name(const char *_name);
	void set_position(int _x, int _y) { assert(false); }
	void set_element_position(Element *element);
	void set_element_place(Place _place);
	void set_alignment(Alignment _alignment);
	
	int get_element_x_place() { return x + next_place.x;}
	int get_element_y_place() { return y + next_place.y;}
};

struct Editor {
	Free_Camera free_camera;
	Array<Window *> windows;
	Linked_List<Window *> drawn_windows;

	Window *focused_window = NULL;

	Form *current_form = NULL;
	Window *current_window = NULL;
	Button *current_button = NULL;
	List_Box *current_list_box = NULL;
	Picked_Panel *current_picked_panel = NULL;
	Edit_Field *current_edit_field = NULL;
	Panel_List_Box *current_panel_list_box = NULL;

	void init();
	void handle_event(Event *event);
	void draw();
	void update();

	void make_window(int flags);
	void make_window(int width, int height, int flags);
	void make_window(int x, int y, int width, int height, int flags);
	void make_window_button(const char *name, Window *window);
	void bind_window(const char *window_will_be_drawn, const char *window_will_be_disabled);
	
	void make_button(const char *text, Callback *callback = NULL);
	void make_list_box(const char *text);
	
	void make_picked_list_box(const char *text);
	void make_end_picked_list_box();
	
	void make_picked_panel();
	void end_picked_panel();
	
	void make_edit_field(const char *label, Edit_Data_Type edit_data_type);
	void make_edit_field(const char *label, int value);
	void make_edit_field(const char *label, float *value);
	
	void make_vector3_edit_field(const char *label);
	void make_vector3_edit_field(const char *label, Vector3 *vec3);
	
	void make_form();
	void end_form();

	void set_form_label(const char *label);
	void set_window_name(const char *name);

	void add_item(const char *item_text, int enum_value);
	void add_form(const char *item_text, int enum_value);
	void add_window(Window *window);
	void add_picked_panel(const char *item_text, int enum_value);

	Window *find_window(const char *name);
};

extern Editor editor;

inline void handle_event_for_editor(Event *event)
{
	editor.handle_event(event);
}

#endif