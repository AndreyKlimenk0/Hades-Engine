#include <math.h>
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

using namespace imgui;

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

enum AxisV2 {
	X_AXISV2 = 0,
	Y_AXISV2 = 1
};

struct Element_Size {
	Size_Dimension width;
	Size_Dimension height;

	Size_Dimension operator[](AxisV2 axis);
};

Size_Dimension Element_Size::operator[](AxisV2 axis)
{
	u32 index = static_cast<u32>(axis);
	assert(index < 2);
	return ((Size_Dimension *)this)[index];
}

struct UI_Element {
	UI_Element();
	UI_Element(Element_ID element_id, UI_Element *parent_element);
	~UI_Element();

	u32 called = 0;
	u32 children_id_counter;

	/*Rect_s32 rect;*/
	Point_s32 position;
	Element_Size size;

	Layout layout;
	u32 alignment_flags;
	
	Color background_color;

	//Context
	Element_ID id;
	UI_Element *parent_element = NULL;
	Array<UI_Element *> child_elements;

	void begin_frame();
	void add_child(UI_Element *ui_element);

	bool root_element();
	UI_Element *find_child(Element_ID element_id);
};

UI_Element::UI_Element() : position(-1, -1)
{
	size.width = fixed_size(100);
	size.height = fixed_size(100);
}

UI_Element::UI_Element(Element_ID element_id, UI_Element *parent_element) : id(element_id), parent_element(parent_element)
{

}

UI_Element::~UI_Element()
{
}


const u32 AUTO_LAYOUT = INT_MAX;

void UI_Element::begin_frame()
{
	position.x = AUTO_LAYOUT;
	position.y = AUTO_LAYOUT;
	size.width = filled_size();
	size.height = filled_size();
	layout = COLUMN_LAYOUT;
	alignment_flags = ALIGNMENT_TOP | ALIGNMENT_LEFT;

	children_id_counter = 0;
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

	UI_Element *get_top_ui_element();
	void push_ui_element(UI_Element *ui_element);
	void pop_ui_element();
};

static UI_Context ui_context;

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

void imgui::init_guiv2(u32 window_width, u32 window_height, const char *font_name, u32 font_size, Render_2D *render_2d)
{
	UI_Element *root_element = new UI_Element();
	root_element->id = Element_ID("Root Element");
	root_element->background_color = Color(0, 0, 0, 0);
	root_element->position = Point_s32(0, 0);
	root_element->size.width = fixed_size(window_width);
	root_element->size.height = fixed_size(window_height);

	ui_context.root_element = root_element;
	ui_context.render_2d = render_2d;

	Font *new_font = Engine::get_font_manager()->get_font(font_name, font_size);
	assert(new_font);
	Render_Font *new_render_font = ui_context.render_2d->get_render_font(new_font);
	assert(new_render_font);

	ui_context.render_primitive_list = new Render_Primitive_List(render_2d, new_font, new_render_font);

	ui_context.push_ui_element(root_element);
}

void imgui::shutdown_guiv2()
{
	ui_context.pop_ui_element();

	assert(ui_context.elements_stack.is_empty());
}

void imgui::begin_frame()
{
	s32 temp_width = ui_context.root_element->size.width.get();
	s32 temp_height = ui_context.root_element->size.height.get();
	ui_context.root_element->begin_frame();
	ui_context.root_element->size.width = fixed_size(temp_width);
	ui_context.root_element->size.height = fixed_size(temp_height);
}

static void reset_call_counters(UI_Element *ui_element)
{
	ui_element->called = 0;
	for (u32 i = 0; i < ui_element->child_elements.count; i++) {
		reset_call_counters(ui_element->child_elements[i]);
	}
}

static void fill_render_primitive_list(const Point_s32 &parent_position, UI_Element *ui_element, Render_Primitive_List *render_primitive_list)
{
	Point_s32 position = parent_position + ui_element->position;
	s32 width = ui_element->size.width.get();
	s32 height = ui_element->size.height.get();

	if ((width > 0) && (height > 0)) { // Should Render 2D handle zero size ?
		Rect_s32 rect = { position.x, position.y, width, height };

		render_primitive_list->add_rect(&rect, ui_element->background_color);
		render_primitive_list->push_clip_rect(&rect);

		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			fill_render_primitive_list(position, ui_element->child_elements[i], render_primitive_list);
		}
		render_primitive_list->pop_clip_rect();
	}
}

static String process_ui_element_name(const char *name)
{
	String string = name;
	if (string.find("#id") >= 0) {
		UI_Element *parent_ui_element = ui_context.get_top_ui_element();
		char *id = to_string(parent_ui_element->children_id_counter++);
		string.replace("#id", id);
		free_string(id);
	}
	return string;
}

void imgui::begin_ui_element(const char *name)
{
	ui_element_debug_counter++;
	
	Element_ID element_id = Element_ID(process_ui_element_name(name));
	UI_Element *parent_ui_element = ui_context.get_top_ui_element();
	UI_Element *ui_element = parent_ui_element->find_child(element_id);
	if (!ui_element) {
		ui_element = new UI_Element(element_id, parent_ui_element);
		parent_ui_element->add_child(ui_element);
	}
	ASSERT_MSG(++ui_element->called == 1, "UI element was declared more then once.");

	ui_element->begin_frame();

	ui_context.push_ui_element(ui_element);
}

AxisV2 flip_axis(AxisV2 axis)
{
	assert((axis == X_AXISV2) || (axis == Y_AXISV2));
	return axis == X_AXISV2 ? Y_AXISV2 : X_AXISV2;
}

void layout_ui_elements_left_to_right_or_top_to_bottom(UI_Element *parent_ui_element, AxisV2 axis)
{
	Point_s32 point;
	point[0] = 1;
	point[1] = 2;
	point[2] = 3;
	s32 offset = 0;
	for (u32 i = 0; i < parent_ui_element->child_elements.count; i++) {
		UI_Element *child = parent_ui_element->child_elements[i];
		if (child->position[static_cast<u32>(axis)] == AUTO_LAYOUT) {
			child->position[static_cast<u32>(axis)] = offset;
			offset += child->size[axis].get();
		}
		auto result = flip_axis(axis);
		if (child->position[static_cast<u32>(flip_axis(axis))] == AUTO_LAYOUT) {
			child->position[0] = 0;
		}
	}
}

void layout_ui_elements_to_right_or_bottom(UI_Element *parent_ui_element, AxisV2 axis)
{
	s32 offset = parent_ui_element->size[axis].get();
	for (u32 i = 0; i < parent_ui_element->child_elements.count; i++) {
		UI_Element *child = parent_ui_element->child_elements[i];
		child->position[static_cast<u32>(axis)] = parent_ui_element->size[axis].get() - child->size[axis].get();
	}
}

void layout_ui_elements_right_to_left_or_bottom_to_top(UI_Element *parent_ui_element, AxisV2 axis)
{
	s32 offset = parent_ui_element->size[axis].get();
	for (u32 i = 0; i < parent_ui_element->child_elements.count; i++) {
		UI_Element *child = parent_ui_element->child_elements[i];
		offset -= child->size[axis].get();
		child->position[static_cast<u32>(axis)] = offset;
	}
}

void layout_ui_elements_in_center(UI_Element *parent_ui_element, AxisV2 axis)
{
	for (u32 i = 0; i < parent_ui_element->child_elements.count; i++) {
		UI_Element *child = parent_ui_element->child_elements[i];
		child->position[static_cast<u32>(axis)] = (parent_ui_element->size[axis].get() / 2) - (child->size[axis].get() / 2);
	}
}

void ground_ui_elements_and_layout_in_center(UI_Element *parent_ui_element, AxisV2 axis)
{
	s32 children_total_size = 0;
	for (u32 i = 0; i < parent_ui_element->child_elements.count; i++) {
		children_total_size += parent_ui_element->child_elements[i]->size[axis].get();
	}
	u32 offset = (parent_ui_element->size[axis].get() / 2) - (children_total_size / 2);
	for (u32 i = 0; i < parent_ui_element->child_elements.count; i++) {
		UI_Element *child = parent_ui_element->child_elements[i];
		child->position[static_cast<u32>(axis)] = offset;
		offset += child->size[axis].get();
	}
}

AxisV2 layout_to_axis(Layout layout)
{
	u32 index = static_cast<u32>(layout);
	assert(index < 2);
	AxisV2 temp[] = { X_AXISV2, Y_AXISV2 };
	return temp[index];
}

typedef void (*Layout_Function)(UI_Element *, AxisV2);

void fill_size(UI_Element *ui_element, AxisV2 axis)
{
	s32 width = 0;
	s32 height = 0;
	if (axis == X_AXISV2) {
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			width += child->size.width.get();
			height = math::max(height, child->size.height.get());
		}
	} else {
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			height += child->size.height.get();
			width = math::max(width, child->size.width.get());
		}
	}
	ui_element->size.width.set(width);
	ui_element->size.height.set(height);
}

void imgui::end_ui_element()
{
	ui_element_debug_counter--;
	ui_context.pop_ui_element();
}

void fill_size_if_needed(UI_Element *ui_element, AxisV2 axis)
{
	assert(ui_element->size[axis].get() > 0);

	if (layout_to_axis(ui_element->layout) == axis) {
		s32 available_space = ui_element->size[axis].get();
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			if (child->size[axis].type != SIZE_TYPE_GROW) {
				available_space -= child->size[axis].get();
			}
		}
		s32 number_filling_elements = 0;
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			if (child->size[axis].type == SIZE_TYPE_GROW) {
				number_filling_elements++;
			}
		}
		s32 filling_element_size = number_filling_elements > 0 ? available_space / number_filling_elements : available_space;
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			if (child->size[axis].type == SIZE_TYPE_GROW) {
				child->size[axis].set(filling_element_size);
			}
		}
	} else {
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			if (child->size[axis].type == SIZE_TYPE_GROW) {
				child->size[axis].set(ui_element->size[axis].get());
			}
		}
	}
}

void layout(UI_Element *ui_element)
{
	if ((ui_element->alignment_flags & ALIGNMENT_TOP) && (ui_element->alignment_flags & ALIGNMENT_LEFT)) {
		layout_ui_elements_left_to_right_or_top_to_bottom(ui_element, layout_to_axis(ui_element->layout));
	}
	if ((ui_element->alignment_flags & ALIGNMENT_BOTTOM) && (ui_element->alignment_flags & ALIGNMENT_LEFT)) {
		if (ui_element->layout == ROW_LAYOUT) {
			layout_ui_elements_left_to_right_or_top_to_bottom(ui_element, X_AXISV2);
			layout_ui_elements_to_right_or_bottom(ui_element, Y_AXISV2);
		} else if (ui_element->layout == COLUMN_LAYOUT) {
			layout_ui_elements_right_to_left_or_bottom_to_top(ui_element, Y_AXISV2);
		}
	}
	if ((ui_element->alignment_flags & ALIGNMENT_TOP) && (ui_element->alignment_flags & ALIGNMENT_RIGHT)) {
		if (ui_element->layout == ROW_LAYOUT) {
			layout_ui_elements_right_to_left_or_bottom_to_top(ui_element, X_AXISV2);
		} else if (ui_element->layout == COLUMN_LAYOUT) {
			layout_ui_elements_left_to_right_or_top_to_bottom(ui_element, layout_to_axis(ui_element->layout));
			layout_ui_elements_to_right_or_bottom(ui_element, ui_element->layout == ROW_LAYOUT ? Y_AXISV2 : X_AXISV2);
		}
	}
	if ((ui_element->alignment_flags & ALIGNMENT_BOTTOM) && (ui_element->alignment_flags & ALIGNMENT_RIGHT)) {
		layout_ui_elements_right_to_left_or_bottom_to_top(ui_element, layout_to_axis(ui_element->layout));
		layout_ui_elements_to_right_or_bottom(ui_element, ui_element->layout == ROW_LAYOUT ? Y_AXISV2 : X_AXISV2);
	}

	if (ui_element->layout == COLUMN_LAYOUT) {
		if (ui_element->alignment_flags & ALIGNMENT_HORIZONTAL_CENTER) {
			if (ui_element->alignment_flags & ALIGNMENT_TOP) {
				layout_ui_elements_in_center(ui_element, X_AXISV2);
				layout_ui_elements_left_to_right_or_top_to_bottom(ui_element, layout_to_axis(ui_element->layout));
			} else if (ui_element->alignment_flags & ALIGNMENT_BOTTOM) {
				layout_ui_elements_in_center(ui_element, X_AXISV2);
				layout_ui_elements_right_to_left_or_bottom_to_top(ui_element, layout_to_axis(ui_element->layout));
			}
		}

		if (ui_element->alignment_flags & ALIGNMENT_VERTICAL_CENTER) {
			if (ui_element->alignment_flags & ALIGNMENT_LEFT) {
				ground_ui_elements_and_layout_in_center(ui_element, Y_AXISV2);
			} if (ui_element->alignment_flags & ALIGNMENT_RIGHT) {
				ground_ui_elements_and_layout_in_center(ui_element, Y_AXISV2);
				layout_ui_elements_to_right_or_bottom(ui_element, X_AXISV2);
			}
		}

		if (ui_element->alignment_flags & ALIGNMENT_CENTER) {
			ground_ui_elements_and_layout_in_center(ui_element, Y_AXISV2);
			layout_ui_elements_in_center(ui_element, X_AXISV2);
		}
	} else if (ui_element->layout == ROW_LAYOUT) {
		if (ui_element->alignment_flags & ALIGNMENT_HORIZONTAL_CENTER) {
			if (ui_element->alignment_flags & ALIGNMENT_TOP) {
				ground_ui_elements_and_layout_in_center(ui_element, X_AXISV2);
			} else if (ui_element->alignment_flags & ALIGNMENT_BOTTOM) {
				ground_ui_elements_and_layout_in_center(ui_element, X_AXISV2);
				layout_ui_elements_to_right_or_bottom(ui_element, Y_AXISV2);
			}
		}

		if (ui_element->alignment_flags & ALIGNMENT_VERTICAL_CENTER) {
			if (ui_element->alignment_flags & ALIGNMENT_LEFT) {
				layout_ui_elements_in_center(ui_element, Y_AXISV2);
				layout_ui_elements_left_to_right_or_top_to_bottom(ui_element, X_AXISV2);
			} if (ui_element->alignment_flags & ALIGNMENT_RIGHT) {
				layout_ui_elements_in_center(ui_element, Y_AXISV2);
				layout_ui_elements_right_to_left_or_bottom_to_top(ui_element, X_AXISV2);
			}
		}

		if (ui_element->alignment_flags & ALIGNMENT_CENTER) {
			ground_ui_elements_and_layout_in_center(ui_element, X_AXISV2);
			layout_ui_elements_in_center(ui_element, Y_AXISV2);
		}
	}

	for (u32 i = 0; i < ui_element->child_elements.count; i++) {
		layout(ui_element->child_elements[i]);
	}
}

s32 calculate_fit_size(UI_Element *ui_element, AxisV2 axis)
{
	s32 result = 0;
	if (layout_to_axis(ui_element->layout) == axis) {
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			ASSERT_MSG(child->size[axis].type != SIZE_TYPE_GROW, "It is not allowed to define UI element with fill size inside fit size UI element.");
			result += child->size[axis].get();
		}
	} else {
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			ASSERT_MSG(child->size[axis].type != SIZE_TYPE_GROW, "It is not allowed to define UI element with fill size inside fit size UI element.");
			result = math::max(child->size[axis].get(), result);
		}
	}
	return result;
}

void fit_size_if_needed(UI_Element *ui_element)
{
	for (u32 i = 0; i < ui_element->child_elements.count; i++) {
		UI_Element *child = ui_element->child_elements[i];
		fit_size_if_needed(child);
	}
	if (ui_element->size.width.type == SIZE_TYPE_FILLED) {
		ui_element->size.width.set(calculate_fit_size(ui_element, X_AXISV2));
	}
	if (ui_element->size.height.type == SIZE_TYPE_FILLED) {
		ui_element->size.height.set(calculate_fit_size(ui_element, Y_AXISV2));
	}
}

void imgui::end_frame()
{
	ASSERT_MSG(ui_element_debug_counter == 0, "UI element stack imbalance, begin_ui_element() and end_ui_element() must be called in pairs.");

	reset_call_counters(ui_context.root_element);

	fit_size_if_needed(ui_context.root_element);

	fill_size_if_needed(ui_context.root_element, X_AXISV2);
	fill_size_if_needed(ui_context.root_element, Y_AXISV2);

	layout(ui_context.root_element);

	fill_render_primitive_list(Point_s32(0, 0), ui_context.root_element, ui_context.render_primitive_list);

	ui_context.render_2d->add_render_primitive_list(ui_context.render_primitive_list);
}

void imgui::set_position(s32 x, s32 y)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->position = Point_s32(x, y);
}

void imgui::set_size(Size_Dimension horizontal, Size_Dimension vertical)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->size.width = horizontal;
	ui_element->size.height = vertical;
}

void imgui::set_layout(Layout layout)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->layout = layout;
}

void imgui::set_alignment(u32 alignment_flags)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->alignment_flags = alignment_flags;
}

void imgui::set_background_color(const Color &color)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->background_color = color;
}