#include "guiv2.h"
#include "../libs/color.h"
#include "../libs/str.h"
#include "../libs/math/structures.h"
#include "../libs/structures/array.h"

struct Element_ID {
	Element_ID();
	Element_ID(const char *string);
	~Element_ID();

	u32 hash;
	String string;
};

struct UI_Element {
	UI_Element();
	UI_Element(UI_Element *parent_element);
	~UI_Element();

	Rect_s32 rect;
	Color background_color;

	UI_Element *parent_element = NULL;
	Array<UI_Element *> child_elements;

	// Rendering
	Render_Primitive_List *primitive_list = NULL;

	bool root_element();
	UI_Element *find_child(const char *name);
};

struct UI_Manager {
	UI_Element *get_context();
	void push_context(UI_Element *ui_element);
	void pop_context();

	UI_Element *root_element = NULL;
};

UI_Manager ui_manager;

void init_guiv2(u32 window_width, u32 window_height, Render_2D *render_2d)
{
	UI_Element *root_element = new UI_Element();
	root_element->rect.set_size(static_cast<s32>(window_width), static_cast<s32>(window_height));
}

void begin_ui_element(const char *name)
{
	UI_Element *parent_ui_element = ui_manager.get_context();
	UI_Element *ui_element = parent_ui_element->find_child(name);
	if (!ui_element) {
		ui_element = new UI_Element(parent_ui_element);
	}

	ui_manager.push_context(ui_element);
}

void end_ui_element()
{
	ui_manager.pop_context();
}

void set_position(s32 x, s32 y)
{
}

void set_size(s32 width, s32 height)
{
}

void set_background_color(const Color &color)
{
}

UI_Element::UI_Element()
{
}

UI_Element::UI_Element(UI_Element *parent_element) : parent_element(parent_element)
{
}

UI_Element::~UI_Element()
{
}
