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

struct Key_Info {
	bool is_pressed = false;
	Key key;
};

struct Mouse_Info {
	s32 x = 0;
	s32 y = 0;
	s32 last_x = 0;
	s32 last_y = 0;

	void set(s32 _x, s32 _y);
	s32 x_delta();
	s32 y_delta();
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

	bool is_key_down(Key key);
};

inline bool Event::is_key_down(Key key)
{
	return (key_info.key == key) && key_info.is_pressed;
}

void pump_events();
void push_event(Event_Type type, int first_value, int second_value);
void run_event_loop();
void clear_event_queue();

bool is_left_mouse_button_down();
bool was_click_by_left_mouse_button();
bool was_left_mouse_button_just_pressed();

Queue<Event> *get_event_queue();

#endif
