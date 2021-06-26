#include <windows.h>
#include <windowsx.h>

#include "event.h"
#include "input.h"

#include <stdio.h>

#include "../../sys/sys_local.h"

Queue<Event> event_queue;

void pump_events()
{
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
	event->first_value = first_value;
	event->second_value = second_value;
	event_queue.push(*event);
}

void run_event_loop()
{
	while (!event_queue.is_empty()) {
		Event event = event_queue.pop();
		if (event.is_key_event()) {
			if (event.is_key_down()) {
				Key_Input::key_down(event.first_value);
			} else {
				Key_Input::key_up(event.first_value);
			}
		} else if (event.is_mouse_event()) {
			Mouse_Input::x = event.first_value;
			Mouse_Input::y = event.second_value;
		} else if (event.is_char_key_event()) {
			Key_Input::was_char_key_input = true;
			Key_Input::inputed_char = event.first_value;
		}
	}
}