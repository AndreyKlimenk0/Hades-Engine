#ifndef EDITOR_H
#define EDITOR_H

#include <stdlib.h>

#include "../render/directx.h"
#include "../render/texture.h"
#include "../libs/str.h"
#include "../libs/os/camera.h"
#include "../libs/ds/array.h"
#include "../libs/ds/dict.h"
#include "../libs/ds/linked_list.h"
#include "../libs/ds/hash_table.h"
#include "../sys/sys_local.h"
#include "../render/font.h"

struct Event;
struct Callback;
struct Form;
struct Picked_Panel;


struct Text_Button_Theme {
	u32 border_about_text = 10;
	u32 rounded_border = 5;
	Color stroke_color = Color::Black;
	Color color = Color(0, 75, 168);
	Color hover_color = Color(0, 60, 168);
};

struct List_Box_Theme {
	u32 field_text_offset_from_left = 5;
	u32 drop_button_offset_from_right = 5;
	u32 border_about_text = 0;
	u32 list_box_offset = 5;
	u32 list_box_top_bottom_border = 10;
	u32 field_width = 180;
	u32 field_height = 20;
	u32 list_box_shift_from_text = 4;
	float rounded_border = 4.0f;
	Color color = Color(74, 82, 90);
};

struct Edit_Field_Theme {
	u32 width = 180;
	u32 height = 20;
	u32 shift_caret_from_left = 4;
	u32 text_offset_from_left = shift_caret_from_left;
	u32 field_shift_from_text = 4;
	float caret_height_in_percents = 80.0f;
	float rounded_border = 4.0f;
	Color color = Color(74, 82, 90);
};

struct Window_Theme {
	bool draw_window_name_in_header = true;
	u32 offset_from_header = 20;
	u32 shift_element_from_window_side = 20;
	u32 place_between_elements = 10;
	u32 shift_element_from_left = 15;
	u32 header_height = 25;
	u32 shift_cross_button_from_left_on = 5;
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
	ELEMENT_TYPE_TEXT,
	ELEMENT_TYPE_BUTTON,
	ELEMENT_TYPE_TEXT_BUTTON,
	ELEMENT_TYPE_TEXTURE_BUTTON,
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
	Element() {}
	Element(u32 x, u32 y) : rect(x, y, 0, 0) {}
	Element(u32 x, u32 y, u32 width, u32 height) : rect(x, y, 0, 0) {}

	Element_Type type = ELEMENT_TYPE_UNDEFINED;
	int flags = 0;
	Rect_u32 rect;

	operator Rect_u32*() { return &rect; }

	virtual void draw() = 0;
	virtual void handle_event(Event *event) = 0;
	virtual void set_position(u32 x, u32 y) = 0;

	u32 get_x() { return rect.x; }
	u32 get_y() { return rect.y; }
	u32 get_width() { return rect.width; }
	u32 get_height() { return rect.height; }
};

struct Text : Element {
	Text() { type = ELEMENT_TYPE_TEXT; }
	Text(const char *text);

	String string;

	void operator=(const char *text);

	void draw();
	void handle_event(Event *event) { assert(false); }
	void set_position(u32 x, u32 y);
};

struct Caret : Element {
	Caret() {};
	Caret(int x, int y) : Element(x, y, 1, font.max_height) {} // blink time are a time in milliseconds
	Caret(float x, float y, float height);

	bool use_float_rect = false;
	
	int blink_time = 500;
	float fx;
	float fy;
	float fwidth = 1;
	float fheight;

	void draw();
	void handle_event(Event *event) {};
	void set_position(u32 x, u32 y);
};

enum Button_Place_Type {
	BUTTON_IS_PLASED_BY_ITSELF,
	BUTTON_IS_PLASED_BY_WINDOW,
};

struct Button : Element {
	Button() { type = ELEMENT_TYPE_BUTTON; }
	~Button();

	Callback *callback = NULL;
	Text_Button_Theme theme;
	
	void draw() = 0;
	void set_position(u32 x, u32 y) = 0;

	Button(const Button &other);
	void handle_event(Event *event);
	void operator=(const Button &other);
};

struct Text_Button : Button {
	Text_Button() { type = ELEMENT_TYPE_TEXT_BUTTON; }
	Text_Button(const char *_text, u32 width = 0, u32 height = 0);
	
	Text text;
	
	void draw();
	void set_theme(Text_Button_Theme *_theme);
	void set_position(u32 x, u32 y);
};

struct Texture_Button : Button {
	Texture_Button() { type = ELEMENT_TYPE_TEXTURE_BUTTON; }
	Texture_Button(Texture *_texture, float scale = 1.0f);
	
	Texture *texture = NULL;

	void draw();
	void set_position(u32 x, u32 y);
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
	void set_position(u32 x, u32 y) { rect.set(x, y); }
};

struct Input_Field : Element {
	Input_Field() { type = ELEMENT_TYPE_INPUT_FIELD; }
	Input_Field(int x, int y, int width, int height) : Element(x, y, width, height) {};
	Text label;

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
	List_Box(const char *_label, u32 x = 0, u32 y = 0);

	u32 list_box_size;
	Text_Button *button = NULL;	
	List_Box_State list_state = LIST_BOX_IS_PICKED_UP;
	
	Rect_u32 field_rect;
	Rect_u32 list_box_rect;
	Text current_chosen_item_text;
	Texture_Button drop_button;

	List_Box_Theme theme;
	Text_Button_Theme button_theme;
	
	Array<Text_Button> item_list;
	Hash_Table<String, int> string_enum_pairs;

	void draw();
	void handle_event(Event *event);
	void set_position(u32 x, u32 y);
	
	void on_list_item_click();
	void on_drop_button_click();
	void add_item(const char *item_text);
	void add_item(const char *string, int enum_value);
	
	int get_chosen_enum_value();
	String *get_chosen_item_string();
	
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
	void set_position(u32 x, u32 y);
};

enum Edit_Data_Type {
	EDIT_DATA_INT,
	EDIT_DATA_FLOAT,
	EDIT_DATA_STRING,
};

struct Edit_Field : Input_Field {
	Edit_Field() { type = ELEMENT_TYPE_EDIT_FIELD; }
	Edit_Field(const char *_label_text, Edit_Data_Type _edit_data_type, Edit_Field_Theme *edit_theme = NULL, u32 x = 0, u32 y = 0);
	Edit_Field(const char *_label_text, float *float_value, Edit_Field_Theme *edit_theme = NULL, u32 x = 0, u32 y = 0);


	bool edit_itself_data;

	int caret_index_in_text; // this caret index specifies at character is placed befor the caret.
	int max_text_width;
	int text_width;
	int caret_index_for_inserting; // this caret index specifies at character is placed after the caret.
	int field_width;

	Point field_position;
	
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
	void set_position(u32 x, u32 y);
	void set_caret_position_on_mouse_click(int mouse_x, int mouse_y);
	void set_text(const char *_text);
	void update_edit_data(const char *text);

	int get_int_value();
	float get_float_value();
};

struct Vector3_Edit_Field : Input_Field {
	Vector3_Edit_Field() { type = ELEMENT_TYPE_VECTOR3_EDIT_FIELD; }
	Vector3_Edit_Field(const char *_label, u32 x = 0, u32 y = 0);
	Vector3_Edit_Field(const char *_label, Vector3 *vec3, u32 x = 0, u32 y = 0);

	u32 place_between_fields = 10;

	Edit_Field x_field;
	Edit_Field y_field;
	Edit_Field z_field;

	void draw();
	void handle_event(Event *event);
	void set_position(u32 x, u32 y);
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
	void set_position(u32 x, u32 y);
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
	Rect_u32 header;

	Texture_Button close_button;

	String name;

	Window_Theme theme;

	Array<Element *> elements;
	Array<Input_Field *> input_fields;
	Array<List_Box *> list_boxies;
	Array<Window *> windows_will_be_disabled;

	void draw();
	void update();
	void make_header();
	void make_window(int x, int y, u32 width, u32 height, int _flags, Window_Theme *_theme);
	void handle_event(Event *event);
	
	void add_header_text(const char *);
	void add_element(Element *element);
	
	void calculate_current_place(Element *element);
	void calculate_next_place(Element *element);
	
	void window_callback();

	void move(int x_delta, int y_delta);

	void set_name(const char *_name);
	void set_position(u32 x, u32 y) { assert(false); }
	void set_element_position(Element *element);
	void set_element_place(Place _place);
	void set_alignment(Alignment _alignment);
	
	int get_element_x_place() { return rect.x + next_place.x;}
	int get_element_y_place() { return rect.y + next_place.y;}
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