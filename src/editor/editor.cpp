#include <assert.h>
#include <stdlib.h>

#include "editor.h"
#include "../sys/sys_local.h"
#include "../win32/win_local.h"
#include "../libs/color.h"
#include "../libs/os/input.h"


static Window_Theme window_theme;
static Button_Theme button_theme;
static List_Box_Theme list_box_theme;


struct Callback {
	virtual void call() = 0;
};

template< typename T>
struct Member_Callback : Callback {
	Member_Callback(T *object, void (T::*callback)()) : object(object), member(callback) {}
	
	T *object;
	void (T::*member)();

	void call() { (object->*member)(); }
};

struct Function_Callback : Callback {
	
	Function_Callback(void (*callback)()) : callback(callback) {}
	void (*callback)();

	void call() { (*callback)(); }
};


static inline bool check_collision_between_element_and_mouse(Element *element)
{
	if (Mouse_Input::x > element->x && Mouse_Input::x < (element->x + element->width) &&
		Mouse_Input::y > element->y && Mouse_Input::y < (element->y + element->height)) {
		return true;
	}
	return false;
}


static inline float calculate_scale_based_on_percent_from_element_height(int header_height, int image_height, float percents)
{
	float needed_height = header_height * percents / 100.0f;
	float ratios = needed_height / image_height;
	return ratios;
}

static inline u32 get_width_from_bitmap(const ID2D1Bitmap *bitmap)
{
	D2D1_SIZE_U size = bitmap->GetPixelSize();
	return size.width;
}

static inline u32 get_height_from_bitmap(const ID2D1Bitmap *bitmap)
{
	D2D1_SIZE_U size = bitmap->GetPixelSize();
	return size.height;
}

Button::Button(const char *text) : Element()
{
	this->text = text;
	//place_type = BUTTON_IS_PLASED_BY_WINDOW;
	type = ELEMENT_TYPE_BUTTON;

	D2D1_SIZE_F size = direct_write.get_text_size_in_pixels(text);
	width = size.width;
	height = size.height;
	theme = button_theme;
}

Button::Button(int x, int y, const char *text) : Element(x, y)
{
	this->text = text;
	D2D1_SIZE_F size = direct_write.get_text_size_in_pixels(text);
	width = size.width;
	height = size.height;
	//place_type = BUTTON_IS_PLASED_BY_ITSELF;
	type = ELEMENT_TYPE_BUTTON;
	theme = button_theme;
}

Button::Button(int x, int y, int width, int height, const char * text) : Element(x, y, width, height)
{
	this->text = text;
	//place_type = BUTTON_IS_PLASED_BY_ITSELF;
	type = ELEMENT_TYPE_BUTTON;
	theme = button_theme;
}

Button::Button(int x, int y, ID2D1Bitmap *image, float scale) : Element(x, y)
{
	assert(image);

	this->image = image;
	this->scale = scale;
	type = ELEMENT_TYPE_BUTTON;
	theme = button_theme;
}

Button::Button(int x, int y, int width, int height, ID2D1Bitmap *image, float scale) : Element(x, y, width, height)
{
	assert(image);

	this->image = image;
	this->scale = scale;
	type = ELEMENT_TYPE_BUTTON;
	theme = button_theme;
}

void Button::draw()
{
	if (image) {
		direct2d.draw_bitmap(x, y, width, height, image, scale);
		return;
	}

	if (cursor_on_button) {
		cursor_on_button = false;
		direct2d.draw_rounded_rect(x, y, width + theme.border_about_text, height + theme.border_about_text, theme.rounded_border, theme.rounded_border, theme.hover_color);
	} else {
		direct2d.draw_rounded_rect(x, y, width + theme.border_about_text, height + theme.border_about_text, theme.rounded_border, theme.rounded_border, theme.color);
	}

	if (!text.is_empty()) {
		direct2d.draw_text(x + (theme.border_about_text / 2) + theme.text_shift, y + (theme.border_about_text / 2), text);
	}
}

void Button::handle_event()
{
	cursor_on_button = true;

	static bool key_is_down = false;
	if (Key_Input::is_key_down(VK_LBUTTON)) {
		key_is_down = true;
	} else {
		if (key_is_down) {
			key_is_down = false;
			if (callback) {
				callback->call();
			}
		}
	}
}

List_Box::List_Box(int x, int y, Array<String> *items, const char *l) : Element(x, y, list_box_theme.output_filed_width, list_box_theme.output_filed_height)
{
	assert(items);
	assert(items->count > 1);

	theme = list_box_theme;

	label = l;
	current_chosen_item_text = &label;
	type = ELEMENT_TYPE_LIST_BOX;

	Button_Theme button_theme;
	button_theme.rounded_border = 2.0f;
	button_theme.border_about_text = 0.0f;
	button_theme.color = Color(74, 82, 90);
	button_theme.text_shift = 2;

	
	D2D1_SIZE_F text_size = direct_write.get_text_size_in_pixels(items->at(0));
	button_height = text_size.height + button_theme.border_about_text;
	list_box_size = text_size.height * items->count;

	for (int i = 0; i < items->count; i++) {
		Button b = Button(x, (y + 2 + height) + (button_height * i), width, button_height, items->at(i));
		b.theme = button_theme;
		b.callback = new Member_Callback<List_Box>(this, &List_Box::on_item_list_click);
		list_items.push(b);
	}


	ID2D1Bitmap *down_image = NULL;
	load_bitmap_from_file("E:\\andrey\\dev\\hades\\data\\editor\\down.png", 1, 1, &down_image);
	float cross_scale_factor = calculate_scale_based_on_percent_from_element_height(height, get_height_from_bitmap(down_image), 70);


	drop_button = Button(x + (width - 20), y, 20, height, down_image, 1.0f);
	drop_button.callback = new Member_Callback<List_Box>(this, &List_Box::on_drop_button_click);

	
	text_y = y + ((height - text_size.height) / 2);
	text_x = x + 2;

}


void List_Box::on_drop_button_click()
{
	if (list_state == LIST_BOX_IS_PICKED_UP) {
		list_state = LIST_BOX_IS_DROPPED;
	} else if (list_state == LIST_BOX_IS_DROPPED) {
		list_state = LIST_BOX_IS_PICKED_UP;
	}
}

void List_Box::on_item_list_click()
{
	if (button) {
		current_chosen_item_text = &button->text;
	} else {
		current_chosen_item_text = &label;
	}
}

void List_Box::update()
{
}

void List_Box::draw()
{

	direct2d.draw_rounded_rect(x, y, width, height, 4.0f, 4.0f, Color(74, 82, 90));
	D2D1_SIZE_F text_size = direct_write.get_text_size_in_pixels(label);
	direct2d.draw_text(text_x, text_y, *current_chosen_item_text);
	drop_button.draw();
	
	if (list_state == LIST_BOX_IS_DROPPED) {
		direct2d.draw_rounded_rect(x, y + 2 + height, width, list_box_size, 5.0f, 5.0f, Color(74, 82, 90));
		for (int i = 0; i < list_items.count; i++) {
			list_items[i].draw();
		}
	}
}

Window::Window(int x, int y, int width, int height) : Element(x, y, width, height + window_theme.header_height)
{
	type = ELEMENT_TYPE_WINDOW;
	ID2D1Bitmap *cross_image = NULL;
	load_bitmap_from_file("E:\\andrey\\dev\\hades\\data\\editor\\cross.png", 1, 1, &cross_image);

	float cross_scale_factor = calculate_scale_based_on_percent_from_element_height(window_theme.header_height, get_height_from_bitmap(cross_image), 70);

	D2D1_SIZE_U size = cross_image->GetPixelSize();

	int button_x = x + width - (size.width * cross_scale_factor) - window_theme.shift_cross_button_from_left_on;
	int button_y = (y + window_theme.header_height / 2) - ((size.height * cross_scale_factor) / 2);
	int button_width = x + size.width;
	int button_height = y + size.height;

	close_button = Button(button_x, button_y, button_width, button_height, cross_image, cross_scale_factor);
	close_button.callback = new Member_Callback<Window>(this, &Window::close);
}

static int compare_list_boxies(const void *first, const void *second) 
{
	assert(first);
	assert(second);

	const List_Box *first_list_box = static_cast<const List_Box *>(first);
	const List_Box *second_list_box = static_cast<const List_Box *>(second);
	
	return first_list_box->y > second_list_box->y;
}


void Window::add_element(Element *element)
{
	if (element->type == ELEMENT_TYPE_LIST_BOX) {
		List_Box *list_box = static_cast<List_Box *>(element);
		list_boxies.push(list_box);

		if (list_boxies.count > 1) {
			qsort(list_boxies.items, list_boxies.count, sizeof(list_boxies[0]), compare_list_boxies);
		}
		return;
	}
	elements.push(element);
}

void Window::handle_event()
{
	if (check_collision_between_element_and_mouse(&close_button)) {
		close_button.handle_event();
		return;
	}

	Element *window_element = NULL;
	FOR(elements, window_element) {	
		if (check_collision_between_element_and_mouse(window_element)) {
			window_element->handle_event();
			break;
		}
	}

	List_Box *list_box = NULL;
	FOR(list_boxies, list_box) {
		if (check_collision_between_element_and_mouse(&list_box->drop_button)) {
			list_box->drop_button.handle_event();
			break;
		}
		if (list_box->list_state == LIST_BOX_IS_DROPPED) {
			Button *b = NULL;
			FOR(list_box->list_items, b) {
				list_box->button = b;
				if (check_collision_between_element_and_mouse(b)) {
					b->handle_event();
					break;
				}
			}
		}
	}
}

void Window::update()
{
}

void Window::draw()
{
	if (close_window) {
		return;
	}

	float factor = window_theme.window_rounded_factor;
	direct2d.draw_rounded_rect(x, y, width, height + window_theme.header_height, factor, factor, window_theme.header_color);
	direct2d.draw_rounded_rect(x, y + window_theme.header_height, width, height, factor, factor, window_theme.color);
	direct2d.fill_rect(x, y + window_theme.header_height, width, height - window_theme.header_height, window_theme.color);

	close_button.draw();

	Array<List_Box *> l;

	Element *element = NULL;
	FOR(elements, element) {
		element->draw();
	}

	List_Box *list_box = NULL;
	FOR(list_boxies, list_box) {
		list_box->draw();
	}
}

void Editor::add_window(Window *window)
{
	windows.push(window);
}

void Editor::init()
{

	Window *w = new Window(0, 0, 300, win32.window_height);
	add_window(w);
	//dd_window(w2);


	//Window *w2 = new Window(300, 10, 200, 500);
	//add_element(w2);

	//
	Button *b1 = new Button(10, 30, "Button");
	Button *b2 = new Button(10, 70, "Create Mesh");
	Button *b3 = new Button(10, 250, "Create Mesh 22");
	w->add_element(b1);
	w->add_element(b2);
	w->add_element(b3);

	Array<String> items;
	items.push(String("Imte 1"));
	items.push(String("Imte 2"));
	items.push(String("Imte 3"));
	items.push(String("Imte 4"));
	items.push(String("Imte 5"));
	items.push(String("Imte 6"));
	items.push(String("Imte 7"));
	items.push(String("Imte 8"));
	items.push(String("Imte 9"));

	//items.push(String("Imte 10"));
	//items.push(String("Imte 11"));
	//items.push(String("Imte 12"));
	//items.push(String("Imte 13"));
	//items.push(String("Imte 14"));
	//items.push(String("Imte 15"));
	//items.push(String("Imte 16"));
	//items.push(String("Imte 17"));
	//items.push(String("Imte 18"));


	//items.push("Imte 2");
	//items.push("Imte 3");
	//items.push("Imte 4");
	//items.push("Imte 5");
	//items.push("Imte 6");
	//items.push("Imte 7");
	//items.push("Imte 8");


	List_Box *list = new List_Box(10, 150, &items, "Entity Type");

	w->add_element(list);

	List_Box *list2 = new List_Box(10, 200, &items, "Items type");
	//List_Box *list3 = new List_Box(10, 250, &items, "Items type");
	List_Box *list4 = new List_Box(10, 300, &items, "Items type");

	w->add_element(list2);
	//w->add_element(list3);
	//w->add_element(list4);

}


void Editor::handle_event()
{
	Window *window = NULL;
	FOR(windows, window) {
		if (check_collision_between_element_and_mouse(window)) {
			window->handle_event();
			Key_Input::keys[VK_LBUTTON] = false;
			break;
		}
	}
}

void Editor::update()
{
	Window *window = NULL;
	FOR(windows, window) {
		window->update();
	}
}

void Editor::draw()
{
	Window *window = NULL;
	FOR(windows, window) {
		window->draw();
	}
}
