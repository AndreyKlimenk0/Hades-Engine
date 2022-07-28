#ifndef EVENT_H
#define EVENT_H

#include "../ds/queue.h"
#include "../../win32/win_types.h"


enum Event_Type {
	EVENT_TYPE_MOUSE,
	EVENT_TYPE_KEY,
	EVENT_TYPE_CHAR,
};

struct Key_Info {
	u8 key;
	bool is_pressed = false;
};

struct Mouse_Info {
	int x;
	int y;
};

struct Event {
	Event() {};
	Event_Type type;
	int first_value;
	int second_value;

	char char_key;
	Key_Info key_info;
	Mouse_Info    mouse_info;

	bool is_key_event() { return type == EVENT_TYPE_KEY; }
	bool is_mouse_event() { return type == EVENT_TYPE_MOUSE; }
	bool is_char_event() { return type == EVENT_TYPE_CHAR; }
	bool is_key_down() { return second_value != 0; }
	bool is_key_down(u8 key) { return (key_info.key == key) && key_info.is_pressed; }

	int get_x_coord() { return first_value; }
	int get_y_coord() { return second_value; }
};

void pump_events();
void push_event(Event_Type type, int first_value, int second_value);
void run_event_loop();

bool was_click_by_left_mouse_button();
bool is_left_mouse_button_down();
bool was_left_mouse_button_just_pressed();

#endif