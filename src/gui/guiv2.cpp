#include <limits.h>

#include "guiv2.h"
#include "../sys/sys.h"
#include "../sys/engine.h"
#include "../libs/color.h"
#include "../libs/str.h"
#include "../libs/math/functions.h"
#include "../libs/math/structures.h"
#include "../libs/structures/array.h"
#include "../libs/structures/stack.h"

static u32 AUTO_WIDTH = UINT_MAX - 1;
static u32 AUTO_HEIGHT = UINT_MAX - 2;


static u32 ui_element_debug_counter = 0;

struct Element_ID {
	Element_ID();
	explicit Element_ID(const char *str);
	Element_ID(const Element_ID &other);
	~Element_ID();

	u32 hash = 0;
	String string;
	
	Element_ID &operator=(const Element_ID &other);
};

Element_ID::Element_ID()
{
}

Element_ID::Element_ID(const char *str)
{
	hash = fast_hash(str);
	string = str;
}

Element_ID::Element_ID(const Element_ID &other)
{
	*this = other;
}

Element_ID::~Element_ID()
{
}

Element_ID &Element_ID::operator=(const Element_ID &other)
{
	if (this != &other) {
		hash = other.hash;
		string = other.string;
	}
	return *this;
}

bool operator==(Element_ID first_id, Element_ID second_id)
{
	return first_id.hash == second_id.hash;
}

bool operator!=(Element_ID first_id, Element_ID second_id)
{
	return !(first_id == second_id);
}

struct UI_Element {
	UI_Element();
	UI_Element(Element_ID element_id, UI_Element *parent_element);
	~UI_Element();

	u32 called = 0;

	/*Rect_s32 rect;*/
	Point_s32 position;
	Size_Dimension width;
	Size_Dimension height;
	
	Color background_color;

	//Context
	Element_ID id;
	UI_Element *parent_element = NULL;
	Array<UI_Element *> child_elements;

	void add_child(UI_Element *ui_element);

	bool root_element();
	UI_Element *find_child(Element_ID element_id);
};

UI_Element::UI_Element() : position(-1, -1), width(fixed_size(100)), height(fixed_size(100))
{
}

UI_Element::UI_Element(Element_ID element_id, UI_Element *parent_element) : id(element_id), parent_element(parent_element)
{
}

UI_Element::~UI_Element()
{
}

void UI_Element::add_child(UI_Element *ui_element)
{
	child_elements.push(ui_element);
}

bool UI_Element::root_element()
{
	return parent_element == NULL;
}

static bool compare_element(UI_Element *ui_element, Element_ID element_id)
{
	return ui_element->id == element_id;
}

UI_Element *UI_Element::find_child(Element_ID element_id)
{
	Find_Result<UI_Element *> result = find_in_array(child_elements, element_id, compare_element);
	if (result.found) {
		return result.data;
	}
	return NULL;
}

struct UI_Context {
	Render_2D *render_2d = NULL;
	Render_Primitive_List *render_primitive_list = NULL;
	
	UI_Element *root_element = NULL;
	
	Stack<UI_Element *> elements_stack;

	UI_Element *get_root_ui_element();
	UI_Element *get_top_ui_element();
	void push_ui_element(UI_Element *ui_element);
	void pop_ui_element();
};

static UI_Context ui_context;

UI_Element *UI_Context::get_root_ui_element()
{
	return ui_context.root_element;
}

UI_Element *UI_Context::get_top_ui_element()
{
	assert(!ui_context.elements_stack.is_empty());
	return ui_context.elements_stack.top();
}

void UI_Context::push_ui_element(UI_Element *ui_element)
{
	ui_context.elements_stack.push(ui_element);
}

void UI_Context::pop_ui_element()
{
	ui_context.elements_stack.pop();
}

void init_guiv2(u32 window_width, u32 window_height, const char *font_name, u32 font_size, Render_2D *render_2d)
{
	UI_Element *root_element = new UI_Element();
	root_element->background_color = Color(0, 0, 0, 0);
	root_element->position = Point_s32(0, 0);
	root_element->width = fixed_size(window_width);
	root_element->height = fixed_size(window_height);

	ui_context.root_element = root_element;
	ui_context.render_2d = render_2d;

	Font *new_font = Engine::get_font_manager()->get_font(font_name, font_size);
	assert(new_font);
	Render_Font *new_render_font = ui_context.render_2d->get_render_font(new_font);
	assert(new_render_font);

	ui_context.render_primitive_list = new Render_Primitive_List(render_2d, new_font, new_render_font);

	ui_context.push_ui_element(root_element);
}

void shutdown_guiv2()
{
	ui_context.pop_ui_element();

	assert(ui_context.elements_stack.is_empty());
}

void begin_frame()
{
}

static void fill_render_primitive_list(const Point_s32 &parent_position, UI_Element *ui_element, Render_Primitive_List *render_primitive_list)
{
	ui_element->called = 0;

	Point_s32 position = parent_position + ui_element->position;
	Rect_s32 rect = { position.x, position.y, static_cast<s32>(ui_element->width.fixed), static_cast<s32>(ui_element->height.fixed) };
	
	render_primitive_list->add_rect(&rect, ui_element->background_color);
	render_primitive_list->push_clip_rect(&rect);

	for (u32 i = 0; i < ui_element->child_elements.count; i++) {
		fill_render_primitive_list(position, ui_element->child_elements[i], render_primitive_list);
	}
	render_primitive_list->pop_clip_rect();
}

void end_frame()
{
	ASSERT_MSG(ui_element_debug_counter == 0, "UI element stack imbalance, begin_ui_element() and end_ui_element() must be called in pairs.");

	fill_render_primitive_list(Point_s32(0, 0), ui_context.get_root_ui_element(), ui_context.render_primitive_list);

	ui_context.render_2d->add_render_primitive_list(ui_context.render_primitive_list);
}

void begin_ui_element(const char *name)
{
	ui_element_debug_counter++;
	
	Element_ID element_id = Element_ID(name);
	UI_Element *parent_ui_element = ui_context.get_top_ui_element();
	UI_Element *ui_element = parent_ui_element->find_child(element_id);
	if (!ui_element) {
		ui_element = new UI_Element(element_id, parent_ui_element);
		parent_ui_element->add_child(ui_element);
	}
	ASSERT_MSG(++ui_element->called == 1, "UI element was declared more then once.");

	ui_context.push_ui_element(ui_element);
}

void end_ui_element()
{
	ui_element_debug_counter--;
	
	UI_Element *ui_element = ui_context.get_top_ui_element();
	if ((ui_element->width.type == SIZE_TYPE_FILLED) && (ui_element->height.type == SIZE_TYPE_FILLED)) {
		ui_element->width.fixed = 0;
		ui_element->height.fixed = 0;
		Point_s32 position_offset = { 0, 0 };
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			ui_element->width.fixed = math::max(ui_element->width.fixed, child->width.fixed);
			ui_element->height.fixed += child->height.fixed;
			child->position = position_offset;
			position_offset.y += child->height.fixed;
		}
	}
	ui_context.pop_ui_element();
}

void set_position(s32 x, s32 y)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->position = Point_s32(x, y);
}

void set_size(Size_Dimension horizontal, Size_Dimension vertical)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->width = horizontal;
	ui_element->height = vertical;
}

void set_background_color(const Color &color)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->background_color = color;
}