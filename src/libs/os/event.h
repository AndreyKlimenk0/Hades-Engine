#ifndef EVENT_H
#define EVENT_H

#include "../ds/queue.h"


enum Event_Type {
	EVENT_TYPE_NONE,
	EVENT_TYPE_KEY,
	EVENT_TYPE_MOUSE,
	EVENT_TYPE_MOUSE_MOVE,
	EVENT_TYPE_MOUSE_KEY,
	EVENT_TYPE_CHAR_KEY,
};

struct Event {
	Event() {};
	Event_Type type;
	int first_value;
	int second_value;

	bool is_key_event() { return type == EVENT_TYPE_KEY; }
	bool is_mouse_event() { return type == EVENT_TYPE_MOUSE; }
	bool is_mouse_move_event() { return type == EVENT_TYPE_MOUSE_MOVE; }
	bool is_mouse_key_event() { return type == EVENT_TYPE_MOUSE_KEY; };
	bool is_char_key_event() { return type == EVENT_TYPE_CHAR_KEY; }
	bool is_key_down() { return second_value != 0; }
	int get_x_coord() { return first_value; }
	int get_y_coord() { return second_value; }
};

void pump_events();
void push_event(Event_Type type, int first_value, int second_value);
void run_event_loop();
#endif