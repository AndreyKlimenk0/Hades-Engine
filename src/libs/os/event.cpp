#include <windows.h>
#include <windowsx.h>

#include "event.h"
#include "input.h"

#include <stdio.h>

#include "../../sys/sys_local.h"
#include "../../editor/editor.h"

static Queue<Event> event_queue;

static bool click_by_left_mouse_button = false;
static bool left_mouse_button_state = false;
static bool left_mouse_button_just_pressed = false;

static void update_just_pressed_left_mouse_button(Event *event)
{
	if (event->type == EVENT_TYPE_KEY) {
		if (event->is_key_down(VK_LBUTTON) && !left_mouse_button_just_pressed) {
			left_mouse_button_just_pressed = true;
		}
	}
}

static void update_left_mouse_button_state(Event *event)
{
	static bool key_is_down = false;
	
	if (event->is_key_down(VK_LBUTTON)) {
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
	click_by_left_mouse_button = false;
}

void pump_events()
{
	if (left_mouse_button_just_pressed) {
		left_mouse_button_just_pressed = false;
	}

	MSG msg = { };
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			ExitProcess(0);
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void push_event(Event_Type type, int first_value, int second_value)
{
	Event *event = new Event();
	event->type = type;

	switch (type) {
		case EVENT_TYPE_KEY: {
			event->key_info.key = first_value;
			event->key_info.is_pressed = second_value;
			update_left_mouse_button_state(event);
			break;
		}
		case EVENT_TYPE_MOUSE: {
			event->mouse_info.x = first_value;
			event->mouse_info.y = second_value;
			break;
		}
		case EVENT_TYPE_CHAR: {
			event->char_key = first_value;
			break;
		}
	}
	update_just_pressed_left_mouse_button(event);
	event_queue.push(*event);
}

void run_event_loop()
{
	while (!event_queue.is_empty()) {
		Event event = event_queue.pop();

		if (event.is_key_event()) {
			if (event.key_info.is_pressed) {
				Key_Input::key_down(event.key_info.key);
			} else {
				Key_Input::key_up(event.key_info.key);
			}
		} else if (event.is_mouse_event()) {
			Mouse_Input::x = event.mouse_info.x;
			Mouse_Input::y = event.mouse_info.y;
		}
		handle_event_for_editor(&event);
	}
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
