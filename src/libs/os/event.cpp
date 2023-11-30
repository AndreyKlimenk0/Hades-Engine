#include <stdio.h>
#include <windows.h>
#include <windowsx.h>

#include "event.h"
#include "../../sys/sys_local.h"

static Queue<Event> event_queue;

static bool click_by_left_mouse_button = false;
static bool left_mouse_button_state = false;
static bool left_mouse_button_just_pressed = false;

static void update_just_pressed_left_mouse_button(Event *event)
{
	if (event->type == EVENT_TYPE_KEY) {
		if (event->is_key_down(KEY_LMOUSE) && !left_mouse_button_just_pressed) {
			left_mouse_button_just_pressed = true;
		}
	}
}

static void update_left_mouse_button_state(Event *event)
{
	static bool key_is_down = false;
	
	if (event->is_key_down(KEY_LMOUSE)) {
		key_is_down = true;
		left_mouse_button_state = true;
	} else {
		left_mouse_button_state = false;
		if (key_is_down) {
			key_is_down = false;
			click_by_left_mouse_button = true;
			return;
		}
	}
}

void pump_events()
{
	click_by_left_mouse_button = false;
	if (left_mouse_button_just_pressed) {
		left_mouse_button_just_pressed = false;
	}

	MSG msg = { };
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void push_event(Event_Type type, int first_value, int second_value)
{
	if ((type == EVENT_TYPE_KEY) && (win32_key_to_engine_key(first_value) == KEY_UNKNOWN)) {
		print("push_event: Failed to convers a virtual key code to an engine key. An event will be skiped.");
		return;
	}

	Event event;
	event.type = type;

	switch (type) {
		case EVENT_TYPE_KEY: {
			event.key_info.key = win32_key_to_engine_key(first_value);
			event.key_info.key_state = second_value > 0 ? KEY_DOWN : KEY_UP;
			update_left_mouse_button_state(&event);
			break;
		}
		case EVENT_TYPE_MOUSE: {
			event.mouse_info.set(first_value, second_value);
			break;
		}
		case EVENT_TYPE_CHAR: {
			event.char_key = first_value;
			break;
		}
		case EVENT_TYPE_MOUSE_WHEEL: {
			event.mouse_wheel_delta = first_value;
			break;
		}
	}
	update_just_pressed_left_mouse_button(&event);
	event_queue.push(event);
}

void run_event_loop()
{
	for (Queue_Node<Event> *node = event_queue.first; node != NULL; node = node->next) {
		Event event = node->item;

		if (event.type == EVENT_TYPE_KEY) {
			if (event.key_info.key_state == KEY_DOWN) {
				Keys_State::key_down(event.key_info.key);
			} else {
				Keys_State::key_up(event.key_info.key);
			}
		} else if (event.type == EVENT_TYPE_MOUSE) {
			Mouse_State::last_x = Mouse_State::x;
			Mouse_State::last_y = Mouse_State::y;
			Mouse_State::x = event.mouse_info.x;
			Mouse_State::y = event.mouse_info.y;
		}
	}
}

void clear_event_queue()
{
	event_queue.clear();
}

Queue<Event>* get_event_queue()
{
	return &event_queue;
}

bool was_click_by_left_mouse_button()
{
	return click_by_left_mouse_button;
}

bool is_left_mouse_button_down()
{
	return left_mouse_button_state;
}

bool was_left_mouse_button_just_pressed()
{
	return left_mouse_button_just_pressed;
}

bool Event::is_key_up(Key key)
{
	return (key_info.key == key) && (key_info.key_state == KEY_UP);
}

bool Event::is_key_down(Key key)
{
	return (key_info.key == key) && (key_info.key_state == KEY_DOWN);
}
