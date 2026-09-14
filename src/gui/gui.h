#ifndef CORE_GUI_H
#define CORE_GUI_H

#include "../libs/str.h"
#include "../libs/number_types.h"
#include "../libs/math/structures.h"
#include "../render/renderer.h"
#include "../render/font.h"

typedef void* Texture_View;

enum AxisV2 {
	X_AXISV2 = 0,
	Y_AXISV2 = 1
};

enum Layout {
	ROW_LAYOUT = 0,    // Place UI elements horizontally
	COLUMN_LAYOUT = 1, // Place UI elements vertically
};

const u32 ALIGNMENT_TOP    = 0x1; // Top to bottom.
const u32 ALIGNMENT_BOTTOM = 0x2; // Bottom to top.
const u32 ALIGNMENT_LEFT   = 0x4; // Left to right.
const u32 ALIGNMENT_RIGHT  = 0x8; // Right to left
const u32 ALIGNMENT_HORIZONTAL_CENTER = 0x10;
const u32 ALIGNMENT_VERTICAL_CENTER   = 0x20;
const u32 ALIGNMENT_CENTER = 0x40;

const u32 TEXT_ELEMENT_MAX_SYMBOL_HEIGHT = 0x80;
const u32 TEXT_ELEMENT_MAX_ALPHABET_HEIGHT = 0x100;

enum Size_Type {
	SIZE_TYPE_FIXED,
	SIZE_TYPE_FIT,     // The element takes into account size of child elements, spacing, padding and adapts its own size accordingly.
	SIZE_TYPE_PERCENT,
	SIZE_TYPE_GROW,    // The element takes into account the size of the parent element and other child elements and occupies the remaining available space.
};

struct Size_Dimension {
	Size_Type type;
	float value = 0.0f;
	void calculate_percent_size(s32 parent_size) { value = value * (float)parent_size; };
	s32 get() { return (s32)value; };
	void set(s32 _value) { value = (float)_value; }
	void add(s32 _value) { value += (float)_value; }
	void sub(s32 _value) { value -= (float)_value; }
};

Size_Dimension fit_size();
Size_Dimension fixed_size(u32 value);
Size_Dimension percent_size(float value);
Size_Dimension grow_size();

struct Padding {
	Padding() {};
	explicit Padding(s32 padding);
	Padding(s32 left, s32 top, s32 right, s32 bottom);
	~Padding();

	s32 left = 0;
	s32 top = 0;
	s32 right = 0;
	s32 bottom = 0;

	s32 operator[](AxisV2 axis);
};

Padding operator+(const Padding &first, const Padding &second);
Padding operator-(const Padding &first, const Padding &second);

struct Element_ID {
	Element_ID();
	explicit Element_ID(const char *str);
	~Element_ID();

	u32 hash = 0;
	String string;

	Element_ID(const Element_ID &other);
	Element_ID &operator=(const Element_ID &other);

	void reset();
};

bool operator==(const Element_ID &first_id, const Element_ID &second_id);
bool operator!=(const Element_ID &first_id, const Element_ID &second_id);

struct Element_Size {
	Size_Dimension width;
	Size_Dimension height;

	Size_Dimension &operator[](AxisV2 axis);
};

struct UI_Element {
	UI_Element();
	UI_Element(Element_ID element_id, UI_Element *parent_element);
	~UI_Element();

	u32 flags;

	u32 called = 0;
	u32 children_id_counter;
	u32 outlining_thikness;

	Point_s32 max_content_border;
	Point_s32 position;
	Point_s32 prev_position;
	Point_s32 content_offset;
	Element_Size size;
	Element_Size prev_size;

	//Theme
	s32 space;
	Padding padding;

	Layout layout;
	u32 rounding;
	u32 rounding_flags;
	u32 alignment_flags;

	Color background_color;
	Color outlining_color;

	//Content
	Text_Alignment text_alignment;
	String text;
	Texture_View texture_view;

	//Context
	Element_ID id;
	UI_Element *parent_element = NULL;
	Array<UI_Element *> child_elements;

	void begin_frame();
	void add_child(UI_Element *ui_element);

	bool root_element();
	UI_Element *find_child(Element_ID element_id);

	Rect_s32 get_rect();
};

void init_gui(u32 window_width, u32 window_height, const char *font_name, u32 font_size, Render_2D *render_2d);
void shutdown_gui();

void begin_frame();
void end_frame();

void begin_ui_element(const char *format_name, ...);
void end_ui_element();

void set_absolute_position_x(s32 x);
void set_absolute_position_y(s32 y);
void set_absolute_position(s32 x, s32 y);
void set_relative_position_x(s32 x);
void set_relative_position_y(s32 y);
void set_relative_position(s32 x, s32 y);
	
void set_size(Size_Dimension horizontal, Size_Dimension vertical);
void set_space(s32 space);
void set_padding(Padding padding);
void set_layout(Layout layout);
void set_alignment(u32 alignment_flags);
void set_background_color(const Color &color);
void set_rounding(u32 rounding, u32 rounding_flags = ROUND_RECT);
void set_outlining(u32 thikness, const Color &color);

void ui_text_element(const char *text, Text_Alignment text_alignment = ALIGN_TEXT_BY_MAX_SYMBOL_IN_TEXT);
void ui_text_element(const char *ui_element_name, const char *text, Text_Alignment text_alignment = ALIGN_TEXT_BY_MAX_SYMBOL_IN_TEXT);

void ui_image_element(const char *ui_element_name, Texture_View texture_view, u32 width, u32 height);

bool ui_element_hovered();
bool ui_element_clicked();
bool ui_element_double_clicked();
bool ui_element_active();
bool ui_element_focused(Element_ID element_id);

Rect_s32 ui_element_rect();

UI_Element *get_ui_element();
UI_Element *get_last_ui_element();
Font *get_font();

s32 get_delta_y();
#endif
