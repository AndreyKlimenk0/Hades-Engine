#include <assert.h>
#include <string.h>

#include "key_binding.h"
#include "os/event.h"
#include "../sys/sys_local.h"

void Key_Command_Bindings::init()
{
	for (u32 i = 0; i < INPUT_KEYS_NUMBER; i++) {
		key_command_list_for_up_keys[i].key = KEY_UNKNOWN;
		key_command_list_for_down_keys[i].key = KEY_UNKNOWN;
	}
}

void Key_Command_Bindings::set(const char *command, Key key, bool key_must_be_pressed)
{
	if (key_must_be_pressed) {
		key_command_list_for_up_keys[key].key = key;
		key_command_list_for_up_keys[key].command = command;
	} else {
		key_command_list_for_down_keys[key].key = key;
		key_command_list_for_down_keys[key].command = command;
	}
}

Find_Command_Result Key_Command_Bindings::find_command(Key key, Key_State key_state, String *command)
{
	if ((key_command_list_for_up_keys[key].key == KEY_UNKNOWN) && (key_command_list_for_down_keys[key].key == KEY_UNKNOWN)) {
		//print("Key_Binding::get_command: There is no any binding for the key {}.", to_string(key));
		return COMMAND_NOT_FOUND;
	}

	if (key_state == KEY_DOWN) {
		if (key_command_list_for_up_keys[key].key != KEY_UNKNOWN) {
			*command = key_command_list_for_up_keys[key].command;
			return COMMAND_FIND;
		} else {
			if (key_command_list_for_down_keys[key].key != KEY_UNKNOWN) {
				return COMMAND_FIND_ON_KEY_DOWN_EVENT;
			}
		}
	} else if (key_state == KEY_UP) {
		if (key_command_list_for_down_keys[key].key != KEY_UNKNOWN) {
			*command = key_command_list_for_down_keys[key].command;
			return COMMAND_FIND;
		} else {
			if (key_command_list_for_up_keys[key].key != KEY_UNKNOWN) {
				return COMMAND_FIND_ON_KEY_UP_EVENT;
			}
		}
	}
	return COMMAND_NOT_FOUND;
}

void Key_Bindings::handle_events() 
{
	Queue<Event> *events = get_event_queue();

	Key_Binding *key_binding = NULL;
	For(key_bindings, key_binding) {
		key_binding->second_key_was_just_pressed = false;

		for (Queue_Node<Event> *node = events->first; node != NULL; node = node->next) {
			Event *event = &node->item;
			if (event->type == EVENT_TYPE_KEY) {
				if (event->is_key_down(key_binding->modifier_key)) {
					if (key_binding->second_key_state != KEY_DOWN) {
						key_binding->modifier_key_pressed_first = true;
					}
					key_binding->modifier_key_state = KEY_DOWN;
				}
				if (event->is_key_up(key_binding->modifier_key)) {
					key_binding->modifier_key_state = KEY_UP;
					key_binding->modifier_key_pressed_first = false;
				}
				if (event->is_key_down(key_binding->second_key)) {
					key_binding->second_key_state = KEY_DOWN;
				}
				if (event->is_key_up(key_binding->second_key)) {
					key_binding->second_key_state = KEY_UP;
					key_binding->second_key_was_just_pressed = true;
				}
			}
		}
	}
}

void Key_Bindings::bind(Key modifier_key, Key second_key) 
{
	if ((modifier_key != KEY_CTRL) && (modifier_key != KEY_SHIFT) && (modifier_key != KEY_ALT)) {
		print("Key_Bindings::bind: Failed to bind keys. The first key must be modifer key (CTRL | SHIFT | ALT) not {}.", to_string(modifier_key));
	} else if ((second_key == KEY_CTRL) || (second_key == KEY_SHIFT)|| (second_key == KEY_ALT)) {
		print("Key_Bindings::bind: Failed to bind keys. The second key must be not modifer key (CTRL | SHIFT | ALT).");
	} else {
		key_bindings.push(Key_Binding(modifier_key, second_key));
	}
}

bool Key_Bindings::is_binding_down(Key modifier_key, Key second_key)
{
	Key_Binding *key_binding = NULL;
	For(key_bindings, key_binding) {
		if ((key_binding->modifier_key == modifier_key) && (key_binding->second_key == second_key)) {
			if (key_binding->modifier_key_pressed_first && (key_binding->modifier_key_state == KEY_DOWN) && (key_binding->second_key_state == KEY_DOWN)) {
				return true;
			} else {
				return false;
			}
		}
	}
	loop_print("Key_Bindings::is_binding_down: Key binding {}+{} was not found in the list.", to_string(modifier_key), to_string(second_key));
	return false;
}

bool Key_Bindings::was_binding_triggered(Key modifier_key, Key second_key) 
{
	Key_Binding *key_binding = NULL;
	For(key_bindings, key_binding) {
		if ((key_binding->modifier_key == modifier_key) && (key_binding->second_key == second_key)) {
			if ((key_binding->modifier_key_state == KEY_DOWN) && key_binding->modifier_key_pressed_first && key_binding->second_key_was_just_pressed) {
				return true;
			} else {
				return false;
			}
		}
	}
	loop_print("Key_Bindings::is_binding_down: Key binding {}+{} was not found in the list.", to_string(modifier_key), to_string(second_key));
	return false;
}

Key_Binding::Key_Binding() 
{
	modifier_key_pressed_first = false;
	second_key_was_just_pressed = false;
	modifier_key = KEY_UNKNOWN;
	second_key = KEY_UNKNOWN;
	modifier_key_state = KEY_UP;
	second_key_state = KEY_UP;
}

Key_Binding::Key_Binding(Key modifier_key, Key second_key) : modifier_key(modifier_key), second_key(second_key) 
{
	modifier_key_pressed_first = false;
	second_key_was_just_pressed = false;
	modifier_key_state = KEY_UP;
	second_key_state = KEY_UP;
}

Key_Binding::~Key_Binding() 
{
}

bool Key_Binding::valid()
{
	return (modifier_key != KEY_UNKNOWN) && (second_key != KEY_UNKNOWN);
}

char *to_string(Key_Binding *key_binding)
{
	assert(key_binding);

	String first_key = key_to_pretty_string(key_binding->modifier_key);
	String second_key = key_to_pretty_string(key_binding->second_key);
	String result = first_key + " + " + second_key;
	char *buffer = new char[result.len + 1];
	memcpy((void *)buffer, result.data, result.len + 1);
	return buffer;
}
