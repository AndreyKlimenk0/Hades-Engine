#include <windows.h>
#include <windowsx.h>

#include "event.h"
#include "input.h"

#include <stdio.h>

#include "../../sys/sys_local.h"
#include "../../editor/editor.h"


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

	switch (type) {
		case EVENT_TYPE_KEY: {
			event->key_info.key = first_value;
			event->key_info.is_pressed = second_value;
			break;
		}
		case EVENT_TYPE_MOUSE: {
			event->mouse_info.x = first_value;
			event->mouse_info.y = second_value;
			break;
		}
		case EVENT_TYPE_CHAR: {
			//event->char_key = first_value;
			event->key_info.key = first_value;
			break;
		}
	}
	//event->first_value = first_value;
	//event->second_value = second_value;
	event_queue.push(*event);
}

void run_event_loop()
{
	while (!event_queue.is_empty()) {
		Event event = event_queue.pop();
		
		handle_event_for_editor(&event);
	}
}