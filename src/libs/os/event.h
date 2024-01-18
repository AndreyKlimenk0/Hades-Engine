#ifndef EVENT_H
#define EVENT_H

#include "input.h"
#include "../ds/queue.h"
#include "../../win32/win_types.h"


enum Event_Type {
	EVENT_TYPE_MOUSE,
	EVENT_TYPE_MOUSE_WHEEL,
	EVENT_TYPE_KEY,
	EVENT_TYPE_CHAR,
};

struct Event {
	Event() {};
	Event_Type type;

	union {
		s32 mouse_wheel_delta;
		char char_key;
		Key_Info key_info;
		Mouse_Info mouse_info;
	};

	bool is_key_up(Key key);
	bool is_key_down(Key key);
};

void pump_events();
void push_event(Event_Type type, int first_value, int second_value);
void run_event_loop();
void clear_event_queue();

bool was_click(Key key);
bool is_left_mouse_button_down();
bool was_click_by_left_mouse_button();
bool was_left_mouse_button_just_pressed();
bool were_pushed_key_events();

Queue<Event> *get_event_queue();

#endif
