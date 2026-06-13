#include <math.h>
#include <limits.h>

#include "guiv2.h"
#include "../sys/sys.h"
#include "../sys/engine.h"
#include "../libs/str.h"
#include "../libs/color.h"
#include "../libs/os/input.h"
#include "../libs/os/event.h"
#include "../libs/math/functions.h"
#include "../libs/math/structures.h"
#include "../libs/structures/array.h"
#include "../libs/structures/stack.h"

using namespace imgui;

static u32 ui_element_debug_counter = 0;

static const u32 UI_ELEMENT_HORIZONTAL_AUTO_LAYOUT = 0x1;
static const u32 UI_ELEMENT_VERTICAL_AUTO_LAYOUT = 0x2;
static const u32 UI_ELEMENT_DRAW_TEXT = 0x4;
static const u32 UI_ELEMENT_DRAW = 0x8;
static const u32 UI_ELEMENT_SET_RELATIVE_X_POSITION = 0x10;
static const u32 UI_ELEMENT_SET_RELATIVE_Y_POSITION = 0x20;


template <typename T>
inline T safe_sub(T x, T y)
{
	return x > y ? x - y : T{0};
}

bool can_auto_layout_ui_element(UI_Element *ui_element, AxisV2 axis)
{
	bool state1 = (axis == X_AXISV2) && (ui_element->flags & UI_ELEMENT_HORIZONTAL_AUTO_LAYOUT);
	bool state2 = (axis == Y_AXISV2) && (ui_element->flags & UI_ELEMENT_VERTICAL_AUTO_LAYOUT);
	return state1 || state2;
}

bool can_auto_layout_ui_element(UI_Element *ui_element, Layout layout)
{
	bool state1 = (layout == ROW_LAYOUT) && (ui_element->flags & UI_ELEMENT_HORIZONTAL_AUTO_LAYOUT);
	bool state2 = (layout == COLUMN_LAYOUT) && (ui_element->flags & UI_ELEMENT_VERTICAL_AUTO_LAYOUT);
	return state1 || state2;
}

static AxisV2 flip_axis(AxisV2 axis)
{
	assert((axis == X_AXISV2) || (axis == Y_AXISV2));
	return axis == X_AXISV2 ? Y_AXISV2 : X_AXISV2;
}

static AxisV2 layout_to_axis(Layout layout)
{
	u32 index = static_cast<u32>(layout);
	assert(index < 2);
	AxisV2 temp[] = { X_AXISV2, Y_AXISV2 };
	return temp[index];
}

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

void Element_ID::reset()
{
	hash = 0;
	string.free();
}

s32 Padding::operator[](AxisV2 axis)
{
	return axis == X_AXISV2 ? left + right : axis == Y_AXISV2 ? top + bottom : 0;
}

Size_Dimension &Element_Size::operator[](AxisV2 axis)
{
	u32 index = static_cast<u32>(axis);
	assert(index < 2);
	return ((Size_Dimension *)this)[index];
}

UI_Element::UI_Element() : position(-1, -1)
{
	size.width = fixed_size(100);
	size.height = fixed_size(100);

	prev_size.width = fixed_size(0);
	prev_size.height = fixed_size(0);
}

UI_Element::UI_Element(Element_ID element_id, UI_Element *parent_element) : id(element_id), parent_element(parent_element)
{

}

UI_Element::~UI_Element()
{
}

void UI_Element::begin_frame()
{
	flags = UI_ELEMENT_HORIZONTAL_AUTO_LAYOUT | UI_ELEMENT_VERTICAL_AUTO_LAYOUT | UI_ELEMENT_DRAW;
	position.x = 0;
	position.y = 0;
	size.width = fit_size();
	size.height = fit_size();
	layout = COLUMN_LAYOUT;
	alignment_flags = ALIGNMENT_TOP | ALIGNMENT_LEFT;

	children_id_counter = 0;
	space = 5;
	outlining_thikness = 0;
	padding = Padding(0);
	rounding = 0;
	rounding_flags = ROUND_RECT;
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
	Font *font = NULL;
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

static bool _detect_intersection(Rect_s32 *rect)
{
	if ((Mouse_State::x > rect->x) && (Mouse_State::x < (rect->x + rect->width)) && (Mouse_State::y > rect->y) && (Mouse_State::y < (rect->y + rect->height))) {
		return true;
	}
	return false;
}

static Rect_s32 _calculate_clip_rect(Rect_s32 *win_rect, Rect_s32 *item_rect)
{
	Rect_s32 clip_rect = *item_rect;

	if ((item_rect->x >= win_rect->right()) || (item_rect->right() <= win_rect->x) || (item_rect->y >= win_rect->bottom()) || (item_rect->bottom() <= win_rect->y)) {
		return *win_rect;
	}

	if ((item_rect->x < win_rect->x) && (item_rect->right() > win_rect->x)) {
		clip_rect.offset_x(win_rect->x - item_rect->x);
	}

	if ((item_rect->y < win_rect->y) && (item_rect->bottom() > win_rect->y)) {
		clip_rect.offset_y(win_rect->y - item_rect->y);
	}

	if ((item_rect->y < win_rect->bottom()) && (item_rect->bottom() > win_rect->bottom())) {
		clip_rect.height -= item_rect->bottom() - win_rect->bottom();
	}

	if ((item_rect->x < win_rect->right()) && (item_rect->right() > win_rect->right())) {
		clip_rect.width -= item_rect->right() - win_rect->right();
	}

	return clip_rect;
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

static void fill_size(UI_Element *ui_element, AxisV2 axis)
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


void add_padding_to_child_elements(UI_Element *ui_element)
{
	if (ui_element->alignment_flags & ALIGNMENT_LEFT) {
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			if (child->flags & UI_ELEMENT_HORIZONTAL_AUTO_LAYOUT) {
				child->position.x += ui_element->padding.left;
			}
		}
	}
	if (ui_element->alignment_flags & ALIGNMENT_TOP) {
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			if (child->flags & UI_ELEMENT_VERTICAL_AUTO_LAYOUT) {
				child->position.y += ui_element->padding.top;
			}
		}
	}
	if (ui_element->alignment_flags & ALIGNMENT_RIGHT) {
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			if (child->flags & UI_ELEMENT_HORIZONTAL_AUTO_LAYOUT) {
				child->position.x -= ui_element->padding.right;
			}
		}
	}
	if (ui_element->alignment_flags & ALIGNMENT_BOTTOM) {
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			if (child->flags & UI_ELEMENT_VERTICAL_AUTO_LAYOUT) {
				child->position.y -= ui_element->padding.bottom;
			}
		}
	}
}

void add_space_to_child_elements(UI_Element *ui_element)
{
	bool condition1 = (ui_element->layout == ROW_LAYOUT) && (ui_element->alignment_flags & ALIGNMENT_RIGHT);
	bool condition2 = (ui_element->layout == COLUMN_LAYOUT) && (ui_element->alignment_flags & ALIGNMENT_BOTTOM);
	s32 sign = condition1 || condition2 ? -1 : 1;

	u32 index = static_cast<u32>(ui_element->layout);
	for (u32 i = 1; i < ui_element->child_elements.count; i++) {
		UI_Element *child = ui_element->child_elements[i];
		if (can_auto_layout_ui_element(child, ui_element->layout)) {
			child->position[index] += sign * ui_element->space * i;
		}
	}

	// Should code be moved in 'calculate_fit_size' function ?
	if ((ui_element->layout == ROW_LAYOUT) && (ui_element->size.width.type == SIZE_TYPE_FIT)) {
		if (ui_element->child_elements.count > 1) {
			ui_element->size.width.add((ui_element->child_elements.count - 1) * ui_element->space);
		}
	}

	if ((ui_element->layout == COLUMN_LAYOUT) && (ui_element->size.height.type == SIZE_TYPE_FIT)) {
		if (ui_element->child_elements.count > 1) {
			ui_element->size.height.add((ui_element->child_elements.count - 1) * ui_element->space);
		}
	}
}

static void reset_call_counters(UI_Element *ui_element)
{
	ui_element->called = 0;
	for (u32 i = 0; i < ui_element->child_elements.count; i++) {
		reset_call_counters(ui_element->child_elements[i]);
	}
}

static s32 calculate_fit_size(UI_Element *ui_element, AxisV2 axis)
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

static void fit_size_if_needed(UI_Element *ui_element)
{
	for (u32 i = 0; i < ui_element->child_elements.count; i++) {
		UI_Element *child = ui_element->child_elements[i];
		fit_size_if_needed(child);
	}
	if (ui_element->size.width.type == SIZE_TYPE_FIT) {
		ui_element->size.width.set(calculate_fit_size(ui_element, X_AXISV2));
		ui_element->size.width.set(ui_element->size.width.get() + ui_element->padding.left + ui_element->padding.right);
	}
	if (ui_element->size.height.type == SIZE_TYPE_FIT) {
		ui_element->size.height.set(calculate_fit_size(ui_element, Y_AXISV2));
		ui_element->size.height.set(ui_element->size.height.get() + ui_element->padding.top + ui_element->padding.bottom);
	}
}

static void grow_size_if_needed(UI_Element *ui_element, AxisV2 axis)
{
	//assert(ui_element->size[axis].get() > 0);

	for (u32 i = 0; i < ui_element->child_elements.count; i++) {
		UI_Element *child = ui_element->child_elements[i];
		grow_size_if_needed(child, axis);
	}

 	if (layout_to_axis(ui_element->layout) == axis) {
		s32 available_space = ui_element->size[axis].get() - ui_element->padding[axis];
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			if (child->size[axis].type != SIZE_TYPE_GROW) {
				available_space -= child->size[axis].get();
			}
		}
		s32 number_growing_elements = 0;
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			if (child->size[axis].type == SIZE_TYPE_GROW) {
				number_growing_elements++;
			}
		}
		s32 growing_element_size = number_growing_elements > 0 ? available_space / number_growing_elements : available_space;
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			if (child->size[axis].type == SIZE_TYPE_GROW) {
				child->size[axis].set(growing_element_size);
			}
		}
	} else {
		s32 available_space = ui_element->size[axis].get() - ui_element->padding[axis];
		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			UI_Element *child = ui_element->child_elements[i];
			if (child->size[axis].type == SIZE_TYPE_GROW) {
				child->size[axis].set(available_space);
			}
		}
	}
}

static void layout_ui_elements_left_to_right_or_top_to_bottom(UI_Element *parent_ui_element, AxisV2 axis)
{
	s32 offset = 0;
	for (u32 i = 0; i < parent_ui_element->child_elements.count; i++) {
		UI_Element *child = parent_ui_element->child_elements[i];
		if (can_auto_layout_ui_element(child, axis)) {
			child->position[static_cast<u32>(axis)] = offset;
			offset += child->size[axis].get();
		}
	}
}

static void layout_ui_elements_to_right_or_bottom(UI_Element *parent_ui_element, AxisV2 axis)
{
	for (u32 i = 0; i < parent_ui_element->child_elements.count; i++) {
		UI_Element *child = parent_ui_element->child_elements[i];
		if (can_auto_layout_ui_element(child, axis)) {
			child->position[static_cast<u32>(axis)] = parent_ui_element->size[axis].get() - child->size[axis].get();
		}
	}
}

static void layout_ui_elements_right_to_left_or_bottom_to_top(UI_Element *parent_ui_element, AxisV2 axis)
{
	s32 offset = parent_ui_element->size[axis].get();
	for (u32 i = 0; i < parent_ui_element->child_elements.count; i++) {
		UI_Element *child = parent_ui_element->child_elements[i];
		if (can_auto_layout_ui_element(child, axis)) {
			offset -= child->size[axis].get();
			child->position[static_cast<u32>(axis)] = offset;
		}
	}
}

static void layout_ui_elements_in_center(UI_Element *parent_ui_element, AxisV2 axis)
{
	for (u32 i = 0; i < parent_ui_element->child_elements.count; i++) {
		UI_Element *child = parent_ui_element->child_elements[i];
		if (can_auto_layout_ui_element(child, axis)) {
			child->position[static_cast<u32>(axis)] = (parent_ui_element->size[axis].get() / 2) - (child->size[axis].get() / 2);
		}
	}
}

static void group_elements_and_layout_in_center(UI_Element *parent_ui_element, AxisV2 axis)
{
	s32 children_total_size = 0;
	for (u32 i = 0; i < parent_ui_element->child_elements.count; i++) {
		UI_Element *child = parent_ui_element->child_elements[i];
		if (can_auto_layout_ui_element(child, axis)) {
			children_total_size += child->size[axis].get();
		}
	}
	s32 spaces_total_size = parent_ui_element->space * (s32)safe_sub(parent_ui_element->child_elements.count, 1u);
	s32 offset = (parent_ui_element->size[axis].get() / 2) - ((children_total_size + spaces_total_size) / 2);
	
	for (u32 i = 0; i < parent_ui_element->child_elements.count; i++) {
		UI_Element *child = parent_ui_element->child_elements[i];
		if (can_auto_layout_ui_element(child, axis)) {
			child->position[static_cast<u32>(axis)] = offset;
			offset += child->size[axis].get();
		}
	}
}

static void layout_child_elements(UI_Element *ui_element)
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
				group_elements_and_layout_in_center(ui_element, Y_AXISV2);
			} if (ui_element->alignment_flags & ALIGNMENT_RIGHT) {
				group_elements_and_layout_in_center(ui_element, Y_AXISV2);
				layout_ui_elements_to_right_or_bottom(ui_element, X_AXISV2);
			}
		}

		if (ui_element->alignment_flags & ALIGNMENT_CENTER) {
			group_elements_and_layout_in_center(ui_element, Y_AXISV2);
			layout_ui_elements_in_center(ui_element, X_AXISV2);
		}
	} else if (ui_element->layout == ROW_LAYOUT) {
		if (ui_element->alignment_flags & ALIGNMENT_HORIZONTAL_CENTER) {
			if (ui_element->alignment_flags & ALIGNMENT_TOP) {
				group_elements_and_layout_in_center(ui_element, X_AXISV2);
			} else if (ui_element->alignment_flags & ALIGNMENT_BOTTOM) {
				group_elements_and_layout_in_center(ui_element, X_AXISV2);
				layout_ui_elements_to_right_or_bottom(ui_element, Y_AXISV2);
			}
		}

		if (ui_element->alignment_flags & ALIGNMENT_VERTICAL_CENTER) {
			// Sometimes I need to align only the vertical center and set the X position manually.
			layout_ui_elements_in_center(ui_element, Y_AXISV2);
			if (ui_element->alignment_flags & ALIGNMENT_LEFT) {
				layout_ui_elements_left_to_right_or_top_to_bottom(ui_element, X_AXISV2);
			} if (ui_element->alignment_flags & ALIGNMENT_RIGHT) {
				//layout_ui_elements_in_center(ui_element, Y_AXISV2);
				layout_ui_elements_right_to_left_or_bottom_to_top(ui_element, X_AXISV2);
			}
		}

		if (ui_element->alignment_flags & ALIGNMENT_CENTER) {
			group_elements_and_layout_in_center(ui_element, X_AXISV2);
			layout_ui_elements_in_center(ui_element, Y_AXISV2);
		}
	}

	add_padding_to_child_elements(ui_element);
	add_space_to_child_elements(ui_element);

	for (u32 i = 0; i < ui_element->child_elements.count; i++) {
		layout_child_elements(ui_element->child_elements[i]);
	}
}

void sort(UI_Element *ui_element)
{
	Array<UI_Element *> first;
	Array<UI_Element *> second;
	for (u32 i = 0; i < ui_element->child_elements.count; i++) {
		if ((ui_element->child_elements[i]->flags & UI_ELEMENT_HORIZONTAL_AUTO_LAYOUT) || (ui_element->child_elements[i]->flags & UI_ELEMENT_HORIZONTAL_AUTO_LAYOUT)) {
			first.push(ui_element->child_elements[i]);
		} else {
			second.push(ui_element->child_elements[i]);
		}
	}
	merge(&first, &second);

	ui_element->child_elements = first;
}

static void fill_render_primitive_list(const Point_s32 &parent_position, Rect_s32 *parent_clip_rect, UI_Element *ui_element, Render_Primitive_List *render_primitive_list)
{
	if (!(ui_element->flags & UI_ELEMENT_DRAW)) {
		return;
	}

	sort(ui_element);

	Point_s32 position = ui_element->position;
	if ((ui_element->flags & UI_ELEMENT_HORIZONTAL_AUTO_LAYOUT) || (ui_element->flags & UI_ELEMENT_SET_RELATIVE_X_POSITION)) {
		position.x += parent_position.x;
	}

	if ((ui_element->flags & UI_ELEMENT_VERTICAL_AUTO_LAYOUT) || (ui_element->flags & UI_ELEMENT_SET_RELATIVE_Y_POSITION)) {
		position.y += parent_position.y;
	}

	ui_element->prev_position = position;
	ui_element->prev_size = ui_element->size;
	
	s32 width = ui_element->size.width.get();
	s32 height = ui_element->size.height.get();

	if ((width > 0) && (height > 0)) { // Should Render 2D handle zero size ?
		Rect_s32 rect = { position.x, position.y, width, height };
		Rect_s32 clip_rect = _calculate_clip_rect(parent_clip_rect, &rect);
		if (ui_element->flags & UI_ELEMENT_DRAW_TEXT) {
			render_primitive_list->push_clip_rect(parent_clip_rect);
			render_primitive_list->add_text(&rect, ui_element->text);
		} else {
			render_primitive_list->push_clip_rect(&clip_rect);
			render_primitive_list->add_rect(&rect, ui_element->background_color, ui_element->rounding, ui_element->rounding_flags);
		}

		if (ui_element->outlining_thikness > 0) {
			render_primitive_list->add_outlines(rect.x, rect.y, rect.width, rect.height, ui_element->outlining_color, (float)ui_element->outlining_thikness, ui_element->rounding);
		}

		for (u32 i = 0; i < ui_element->child_elements.count; i++) {
			fill_render_primitive_list(position, &clip_rect, ui_element->child_elements[i], render_primitive_list);
		}
		render_primitive_list->pop_clip_rect();
	}
	// One of the reasons why the flags are reset here is to avoid drawing a ui_element if begin_ui_element is not called in the next frame.
	ui_element->flags = 0;
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

	ui_context.font = new_font;
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
	ui_context.root_element->position.x = 0;
	ui_context.root_element->position.y = 0;
	ui_context.root_element->size.width = fixed_size(temp_width);
	ui_context.root_element->size.height = fixed_size(temp_height);
}

void imgui::end_frame()
{
	ASSERT_MSG(ui_element_debug_counter == 0, "UI element stack imbalance, begin_ui_element() and end_ui_element() must be called in pairs.");

	reset_call_counters(ui_context.root_element);

	fit_size_if_needed(ui_context.root_element);

	grow_size_if_needed(ui_context.root_element, X_AXISV2);
	grow_size_if_needed(ui_context.root_element, Y_AXISV2);

	layout_child_elements(ui_context.root_element);

	Rect_s32 clip_rect = { 0, 0, ui_context.root_element->size.width.get(), ui_context.root_element->size.height.get() };
	fill_render_primitive_list(Point_s32(0, 0), &clip_rect, ui_context.root_element, ui_context.render_primitive_list);

	ui_context.render_2d->add_render_primitive_list(ui_context.render_primitive_list);
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

void imgui::end_ui_element()
{
	ui_element_debug_counter--;
	ui_context.pop_ui_element();
}

void imgui::set_absolute_position_x(s32 x)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->flags &= ~UI_ELEMENT_HORIZONTAL_AUTO_LAYOUT;
	ui_element->position.x;
}

void imgui::set_absolute_position_y(s32 y)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->flags &= ~UI_ELEMENT_VERTICAL_AUTO_LAYOUT;
	ui_element->position.y = y;
}

void imgui::set_absolute_position(s32 x, s32 y)
{
	set_absolute_position_x(x);
	set_absolute_position_y(y);
}

void imgui::set_relative_position_x(s32 x)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->flags &= ~UI_ELEMENT_HORIZONTAL_AUTO_LAYOUT;
	ui_element->flags |= UI_ELEMENT_SET_RELATIVE_X_POSITION;
	ui_element->position.x = x;
}

void imgui::set_relative_position_y(s32 y)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->flags &= ~UI_ELEMENT_VERTICAL_AUTO_LAYOUT;
	ui_element->flags |= UI_ELEMENT_SET_RELATIVE_Y_POSITION;
	ui_element->position.y = y;
}

void imgui::set_relative_position(s32 x, s32 y)
{
	set_relative_position_x(x);
	set_relative_position_y(y);
}

void imgui::set_size(Size_Dimension horizontal, Size_Dimension vertical)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->size.width = horizontal;
	ui_element->size.height = vertical;
}

void imgui::set_space(s32 space)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->space = space;
}

void imgui::set_padding(Padding padding)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->padding = padding;
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

void imgui::set_rounding(u32 rounding, u32 rounding_flags)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->rounding = rounding;
	ui_element->rounding_flags = rounding_flags;
}

void imgui::set_outlining(u32 thikness, const Color &color)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->outlining_thikness = thikness;
	ui_element->outlining_color = color;
}

static void ui_element_draw_text(const char *text)
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	ui_element->flags |= UI_ELEMENT_DRAW_TEXT;
	ui_element->text = text;
}

void imgui::text(const char *text)
{
	Size_u32 text_size = ui_context.font->get_text_size(text);
	begin_ui_element(text);
	set_size(fixed_size(text_size.width), fixed_size(text_size.height));
	ui_element_draw_text(text);
	set_background_color(Color::Black);
	end_ui_element();
}

void imgui::text(const char *ui_element_name, const char *text)
{
	Size_u32 text_size = ui_context.font->get_text_size(text);
	begin_ui_element(ui_element_name);
	set_size(fixed_size(text_size.width), fixed_size(text_size.height));
	ui_element_draw_text(text);
	set_background_color(Color::Black);
	end_ui_element();
}

bool imgui::ui_element_hovered()
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	Rect_s32 rect = { ui_element->prev_position.x, ui_element->prev_position.y, ui_element->prev_size.width.get(), ui_element->prev_size.height.get() };
	return _detect_intersection(&rect);
}

bool imgui::ui_element_clicked()
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	Rect_s32 rect = { ui_element->prev_position.x, ui_element->prev_position.y, ui_element->prev_size.width.get(), ui_element->prev_size.height.get() };
	return was_click(KEY_LMOUSE) && _detect_intersection(&rect);;
}

bool imgui::ui_element_double_clicked()
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	Rect_s32 rect = { ui_element->prev_position.x, ui_element->prev_position.y, ui_element->prev_size.width.get(), ui_element->prev_size.height.get() };
	return was_double_click(KEY_LMOUSE) && _detect_intersection(&rect);;
}

Rect_s32 imgui::ui_element_rect()
{
	UI_Element *ui_element = ui_context.get_top_ui_element();
	return { ui_element->prev_position.x, ui_element->prev_position.y, ui_element->prev_size.width.get(), ui_element->prev_size.height.get() };
}

UI_Element *imgui::get_ui_element()
{
	return ui_context.get_top_ui_element();
}
