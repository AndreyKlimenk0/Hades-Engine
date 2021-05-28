#include <assert.h>

#include "editor.h"
#include "../sys/sys_local.h"
#include "../win32/win_local.h"
#include "../libs/color.h"
#include "../libs/os/input.h"


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


struct Button_Draw_Info {
	int border_about_text = 4;
	float button_rounded_factor = 3.0f;
	Color stroke_color = Color::Black;
	Color background_color =  Color(0, 75, 168);
	Color background_color_for_cursor = Color(0, 60, 168);
};

struct Window_Draw_Info {
	int   header_height = 20;
	int   shift_cross_button_from_left_on = 5;
	float window_rounded_factor = 6.0f;
	float header_botton_height_in_percents = 50;
	Color header_color = Color(10, 7, 7);
	Color background_color = Color(36, 39, 43);
};

static Window_Draw_Info window_draw_info;
static Button_Draw_Info button_draw_info;

static inline bool check_collision_between_element_and_mouse(Element *element)
{
	if (Mouse_Input::x >= element->x && Mouse_Input::x <= (element->x + element->width) &&
		Mouse_Input::y >= element->y && Mouse_Input::y <= (element->y + element->height)) {
		return true;
	} else {
		return false;
	}
}


static inline float calculate_scale_based_on_percent_from_element_height(int header_height, int image_height, float percents)
{
	float needed_height = header_height * percents / 100.0f;
	float ratios = needed_height / image_height;
	return ratios;
}

inline u32 get_width_from_bitmap(const ID2D1Bitmap *bitmap)
{
	D2D1_SIZE_U size = bitmap->GetPixelSize();
	return size.width;
}

inline u32 get_height_from_bitmap(const ID2D1Bitmap *bitmap)
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
}

Button::Button(int x, int y, const char *text) : Element(x, y)
{
	this->text = text;
	D2D1_SIZE_F size = direct_write.get_text_size_in_pixels(text);
	width = size.width;
	height = size.height;
	//place_type = BUTTON_IS_PLASED_BY_ITSELF;
	type = ELEMENT_TYPE_BUTTON;
}

Button::Button(int x, int y, int width, int height, const char * text) : Element(x, y, width, height)
{
	this->text = text;
	//place_type = BUTTON_IS_PLASED_BY_ITSELF;
	type = ELEMENT_TYPE_BUTTON;
}

Button::Button(int x, int y, ID2D1Bitmap *image, float scale) : Element(x, y)
{
	assert(image);

	this->image = image;
	this->scale = scale;
	type = ELEMENT_TYPE_BUTTON;
}

Button::Button(int x, int y, int width, int height, ID2D1Bitmap *image, float scale) : Element(x, y, width, height)
{
	assert(image);

	this->image = image;
	this->scale = scale;
	type = ELEMENT_TYPE_BUTTON;
}

void Button::draw()
{
	if (image) {
		direct2d.draw_bitmap(x, y, width, height, image, scale);
		return;
	}

	if (cursor_on_button) {
		direct2d.draw_rounded_rect(x, y, width + button_draw_info.border_about_text, height + button_draw_info.border_about_text, button_draw_info.button_rounded_factor, button_draw_info.button_rounded_factor, button_draw_info.background_color_for_cursor);
		cursor_on_button = false;
	} else {
		direct2d.draw_rounded_rect(x, y, width + button_draw_info.border_about_text, height + button_draw_info.border_about_text, button_draw_info.button_rounded_factor, button_draw_info.button_rounded_factor, button_draw_info.background_color);
	}

	if (!text.is_empty()) {
		direct2d.draw_text(x + (button_draw_info.border_about_text / 2), y + (button_draw_info.border_about_text / 2), text);
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

List_Box::List_Box(int x, int y, int width, int height) : Element(x, y, width, height)
{
	type = ELEMENT_TYPE_LIST_BOX;
	ID2D1Bitmap *down_image = NULL;
	load_bitmap_from_file("E:\\andrey\\dev\\hades\\data\\editor\\down.png", 1, 1, &down_image);
	float cross_scale_factor = calculate_scale_based_on_percent_from_element_height(height, get_height_from_bitmap(down_image), 70);

	//drop_button = Button(x, y, 200, height, down_image, NULL, 1.0f);
	drop_button = Button(x + (width - 20), y, 20, height, down_image, 1.0f);
	//Function_Callback *f = new Function_Callback(&test_callback);
}

List_Box::List_Box(int x, int y, int width, int height, const char *lable, Array<String> *items) : Element(x, y, width, height)
{
	type = ELEMENT_TYPE_LIST_BOX;
	this->label = lable;
	
	ID2D1Bitmap *down_image = NULL;
	load_bitmap_from_file("E:\\andrey\\dev\\hades\\data\\editor\\cross.png", 1, 1, &down_image);
	float cross_scale_factor = calculate_scale_based_on_percent_from_element_height(height, get_height_from_bitmap(down_image), 70);

	drop_button = Button(x, y, 200, height, down_image, 1.0f);
	drop_button.callback = new Member_Callback<List_Box>(this, &List_Box::on_button_click);
	//drop_button = Button(x, y, 200, height, "lafff");s
}

void List_Box::on_button_click()
{
	if (list_state == LIST_BOX_IS_PICKED_UP) {
		list_state = LIST_BOX_IS_DROPPING;
	} else if (list_state == LIST_BOX_IS_DROPPED) {
		list_state = LIST_BOX_IS_PICKING_UP;
	}
}

void List_Box::update()
{
	if (list_state == LIST_BOX_IS_PICKED_UP || list_state == LIST_BOX_IS_DROPPED) {
		return;
	}

	if (list_state == LIST_BOX_IS_DROPPING) {
		if (list_box_x >= list_box_size) {
			list_state = LIST_BOX_IS_DROPPED;
		} else {
			list_box_x += 1;
		}

	}
}

void List_Box::draw()
{
	direct2d.draw_rounded_rect(x, y, width, height, 4.0f, 4.0f, Color(74, 82, 90));
	drop_button.draw();

	if (list_state == LIST_BOX_IS_DROPPED || list_state == LIST_BOX_IS_DROPPING, list_state == LIST_BOX_IS_PICKING_UP) {
		direct2d.draw_rounded_rect(x, y + height, width, list_box_size, 5.0f, 5.0f, Color::Black);
	}
}

Window::Window(int x, int y, int width, int height) : Element(x, y, width, height + window_draw_info.header_height)
{
	//type = ELEMENT_TYPE_WINDOW;
	ID2D1Bitmap *cross_image = NULL;
	load_bitmap_from_file("E:\\andrey\\dev\\hades\\data\\editor\\cross.png", 1, 1, &cross_image);

	float cross_scale_factor = calculate_scale_based_on_percent_from_element_height(window_draw_info.header_height, get_height_from_bitmap(cross_image), 70);

	D2D1_SIZE_U size = cross_image->GetPixelSize();

	int button_x = x + width - (size.width * cross_scale_factor) - window_draw_info.shift_cross_button_from_left_on;
	int button_y = (y + window_draw_info.header_height / 2) - ((size.height * cross_scale_factor) / 2);
	int button_width = x + size.width;
	int button_height = y + size.height;

	close_button = Button(button_x, button_y, button_width, button_height, cross_image, cross_scale_factor);
	close_button.callback = new Member_Callback<Window>(this, &Window::close);
}


void Window::add_button(Button *button)
{
	elements.push(button);
}

void Window::add_element(Element *element)
{
	if (element->type == ELEMENT_TYPE_LIST_BOX) {
		List_Box *list_box = static_cast<List_Box *>(element);
		list_boxies.push(list_box);
	}
	elements.push(element);
}

void Window::handle_event()
{
	if (check_collision_between_element_and_mouse(&close_button)) {
		close_button.handle_event();
	}

	Element *window_element = NULL;
	FOR(elements, window_element) {
		
		if (window_element->type = ELEMENT_TYPE_LIST_BOX) {
			List_Box *list_box = static_cast<List_Box *>(window_element);
			if (check_collision_between_element_and_mouse(&list_box->drop_button)) {
				list_box->drop_button.handle_event();
				continue;
			}
		}

		if (check_collision_between_element_and_mouse(window_element)) {
			window_element->handle_event();
		}
	}
}

void Window::update()
{
	List_Box *list_box = NULL;
	FOR(list_boxies, list_box) {
		list_box->update();
	}
}

void Window::draw()
{
	if (close_window) {
		return;
	}
	float factor = window_draw_info.window_rounded_factor;
	direct2d.draw_rounded_rect(x, y, width, height + window_draw_info.header_height, factor, factor, window_draw_info.header_color);
	direct2d.draw_rounded_rect(x, y + window_draw_info.header_height, width, height, factor, factor, window_draw_info.background_color);
	direct2d.fill_rect(x, y + window_draw_info.header_height, width, height - window_draw_info.header_height, window_draw_info.background_color);

	close_button.draw();

	Element *element = NULL;
	FOR(elements, element) {
		element->draw();
	}
}

void Editor::add_window(Window *window)
{
	windows.push(window);
}

void Editor::draw()
{
	Window *window = NULL;
	FOR(windows, window) {
		window->draw();
	}
}

void Editor::init()
{

	Window *w = new Window(0, 0, 300, win32.window_height);
	add_window(w);


	//Window *w2 = new Window(300, 10, 200, 500);
	//add_element(w2);

	//
	Button *b1 = new Button(10, 30, "Button");
	Button *b2 = new Button(10, 70, "Create Mesh");
	Button *b3 = new Button(10, 120, "Create Entity");
	w->add_element(b1);
	w->add_element(b2);
	w->add_element(b3);

	List_Box *list = new List_Box(10, 150, 200, 20);
	w->add_element(list);
	//Button *b2 = new Button(10, 100, "SSSSSSSSSSSSSSSSSSSSSSSaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
	//Button *b3 = new Button(10, 200, 50, 20, "alalla");
	//Button *b4 = new Button(10, 300, 50, 20, "alalla");

	//add_element(b1);
	//add_element(b2);
	//add_element(b3);
	//add_element(b4);

}


void Editor::handle_event()
{
	Window *window = NULL;
	Element *window_element = NULL;
	FOR(windows, window) {
		if (check_collision_between_element_and_mouse(window)) {
			window->handle_event();
			Key_Input::keys[VK_LBUTTON] = false;
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