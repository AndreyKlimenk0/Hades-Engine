#ifndef KEY_BINDING_H
#define KEY_BINDING_H

#include "str.h"
#include "os/input.h"
#include "structures/array.h"


struct Key_Command_Binding {
	Key key;
	String command;
};

enum Find_Command_Result {
	COMMAND_FIND,
	COMMAND_FIND_ON_KEY_UP_EVENT,
	COMMAND_FIND_ON_KEY_DOWN_EVENT,
	COMMAND_NOT_FOUND
};

struct Key_Command_Bindings {
	Key_Command_Binding key_command_list_for_up_keys[INPUT_KEYS_NUMBER];
	Key_Command_Binding key_command_list_for_down_keys[INPUT_KEYS_NUMBER];

	void init();
	void set(const char *command, Key key, bool key_must_be_pressed = true);
	Find_Command_Result find_command(Key key, Key_State key_state, String *command);
};

struct Key_Binding {
	Key_Binding();
	Key_Binding(Key modifier_key, Key second_key);
	~Key_Binding();

	bool modifier_key_pressed_first;
	bool second_key_was_just_pressed;
	Key modifier_key;
	Key second_key;
	Key_State modifier_key_state;
	Key_State second_key_state;

	bool valid();
};

char *to_string(Key_Binding *key_binding);

struct Key_Bindings {
	Array<Key_Binding> key_bindings;

	void handle_events();
	void bind(Key modifier_key, Key second_key);
	bool is_binding_down(Key modifer_key, Key second_key);
	bool was_binding_triggered(Key modifier_key, Key second_key);
};

#endif