#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <windowsx.h>

#include "event.h"
#include "../../sys/sys.h"

static bool left_mouse_double_click = false;
static bool right_mouse_double_click = false;

static bool at_least_one_key_event_pushed = false;

static bool click_key_states[INPUT_KEYS_NUMBER];
static bool down_key_states[INPUT_KEYS_NUMBER];

static bool just_pressed_keys[INPUT_KEYS_NUMBER];
static bool just_released_keys[INPUT_KEYS_NUMBER];

static Queue<Event> event_queue;

inline void update_click_key_states(Event *event)
{
	if (event->key_info.key_state == KEY_DOWN) {
		down_key_states[(u8)event->key_info.key] = true;
	} else {
		if (down_key_states[(u8)event->key_info.key]) {
			down_key_states[(u8)event->key_info.key] = false;
			click_key_states[(u8)event->key_info.key] = true;
		}
	}
}

inline void update_just_pressed_keys(Event *event)
{
	assert(event->type == EVENT_TYPE_KEY);

	if ((event->key_info.key_state == KEY_DOWN) && !just_pressed_keys[event->key_info.key]) {
		just_pressed_keys[event->key_info.key] = true;
	}
}

inline void update_just_released_keys(Event *event)
{
	assert(event->type == EVENT_TYPE_KEY);

	if ((event->key_info.key_state == KEY_UP) && !just_released_keys[event->key_info.key]) {
		just_released_keys[event->key_info.key] = true;
	}
}

void pump_events()
{
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
		case EVENT_TYPE_DOUBLE_CLICK: {
			if (first_value > 0) {
				event.key_info.key = KEY_LMOUSE;
			} else if (second_value > 0) {
				event.key_info.key = KEY_RMOUSE;
			}
			break;
		}
	}
	event_queue.push(event);
}

void run_event_loop()
{
	Async_Input::new_frame();

	at_least_one_key_event_pushed = false;
	left_mouse_double_click = false;
	right_mouse_double_click = false;
	memset((void *)just_pressed_keys, 0, sizeof(bool) * INPUT_KEYS_NUMBER);
	memset((void *)just_released_keys, 0, sizeof(bool) * INPUT_KEYS_NUMBER);
	memset((void *)click_key_states, 0, sizeof(bool) * INPUT_KEYS_NUMBER);

	for (Queue_Node<Event> *node = event_queue.first; node != NULL; node = node->next) {
		Event event = node->item;

		if (event.type == EVENT_TYPE_KEY) {
			if (event.key_info.key_state == KEY_DOWN) {
				Keys_State::key_down(event.key_info.key);
			} else {
				Keys_State::key_up(event.key_info.key);
			}
			at_least_one_key_event_pushed = true;
			update_click_key_states(&event);
			update_just_pressed_keys(&event);
			update_just_released_keys(&event);

		} else if (event.type == EVENT_TYPE_MOUSE) {
			Mouse_State::last_x = Mouse_State::x;
			Mouse_State::last_y = Mouse_State::y;
			Mouse_State::x = event.mouse_info.x;
			Mouse_State::y = event.mouse_info.y;
		} else if (event.type == EVENT_TYPE_MOUSE_WHEEL) {
			Async_Input::add_mouse_wheel(event.mouse_wheel_delta);
		
		} else if (event.type == EVENT_TYPE_DOUBLE_CLICK) {
			if (event.key_info.key == KEY_LMOUSE) {
				left_mouse_double_click = true;
			} else if (event.key_info.key == KEY_RMOUSE) {
				right_mouse_double_click = true;
			} else {
				print("run_event_loop: Can recognize a key of a double key event.");
			}
		}
	}
}

void clear_event_queue()
{
	event_queue.clear();
}

Queue<Event> *get_event_queue()
{
	return &event_queue;
}

bool was_click(Key key)
{
	return click_key_states[(u8)key];
}

bool were_key_events()
{
	return at_least_one_key_event_pushed;
}

bool was_key_just_pressed(Key key)
{
	return just_pressed_keys[key];
}

bool was_key_just_released(Key key)
{
	return just_released_keys[key];
}

bool was_double_click(Key key)
{
	if (key == KEY_LMOUSE) {
		return left_mouse_double_click;
	} else if (key == KEY_RMOUSE) {
		return right_mouse_double_click;
	} else {
		return false;
	}
}

bool Event::is_key_up(Key key)
{
	return (key_info.key == key) && (key_info.key_state == KEY_UP);
}

bool Event::is_key_down(Key key)
{
	return (key_info.key == key) && (key_info.key_state == KEY_DOWN);
}
