#include <windows.h>

#include "input.h"
#include "../../sys/sys_local.h"

s32 Mouse_Input::x = 0;
s32 Mouse_Input::y = 0;
s32 Mouse_Input::last_x = 0;
s32 Mouse_Input::last_y = 0;

bool Key_Input::was_char_key_input;
bool Key_Input::keys[KEYBOARD_KEY_NUMBER];
char Key_Input::inputed_char;

const char *string_keys[KEYBOARD_KEY_NUMBER] = {
	"KEY_UNKNOWN",
	"KEY_LMOUSE",
	"KEY_RMOUSE",
	"KEY_ENTER",
	"KEY_BACKSPACE",
	"KEY_HOME",
	"KEY_END",
	"KEY_LEFT",
	"KEY_RIGHT",
	"KEY_UP",
	"KEY_DOWN",
	"KEY_SHIFT",
	"KEY_A",
	"KEY_B",
	"KEY_C",
	"KEY_D",
	"KEY_E",
	"KEY_F",
	"KEY_G",
	"KEY_H",
	"KEY_I",
	"KEY_J",
	"KEY_K",
	"KEY_L",
	"KEY_M",
	"KEY_N",
	"KEY_O",
	"KEY_P",
	"KEY_Q",
	"KEY_R",
	"KEY_S",
	"KEY_T",
	"KEY_U",
	"KEY_V",
	"KEY_W",
	"KEY_X",
	"KEY_Y",
	"KEY_Z"
};

const char *to_string(Key key)
{
	u32 index = 0;
	if (key >= KEY_A) {
		index = ((u8)key - ENGLISH_ALPHABET_KEYS_OFFSET) + (u8)KEY_SHIFT + 1;
	}
	else if ((key >= KEY_UNKNOWN) && (key <= KEY_SHIFT)) {
		index = (u8)key;
	}
	return string_keys[index];
}

Key win32_key_to_engine_key(s32 win32_key_code)
{
	if ((win32_key_code >= 0x41) && (win32_key_code <= 0x5a)) {
		return (Key)(ENGLISH_ALPHABET_KEYS_OFFSET + (win32_key_code - 0x41));
	}
	switch (win32_key_code) {
	case VK_LBUTTON:
		return KEY_LMOUSE;
	case VK_RBUTTON:
		return KEY_RMOUSE;
	case VK_RETURN:
		return KEY_ENTER;
	case VK_BACK:
		return KEY_BACKSPACE;
	case VK_HOME:
		return KEY_HOME;
	case VK_END:
		return KEY_END;
	case VK_UP:
		return KEY_UP;
	case VK_DOWN:
		return KEY_DOWN;
	case VK_RIGHT:
		return KEY_RIGHT;
	case VK_LEFT:
		return KEY_LEFT;
	}
	return KEY_UNKNOWN;
}

s32 Mouse_Input::x_delta()
{
	return Mouse_Input::x - Mouse_Input::last_x;
}

s32 Mouse_Input::y_delta()
{
	return Mouse_Input::y - Mouse_Input::last_y;
}

void Key_Input::setup()
{
	for (int i = 0; i < KEYBOARD_KEY_NUMBER; i++) {
		keys[i] = false;
	}
}

void Key_Input::key_down(int key)
{
	keys[key] = true;
}

void Key_Input::key_up(int key)
{
	keys[key] = false;
}

bool Key_Input::is_key_down(int key)
{
	if (key < KEYBOARD_KEY_NUMBER) {
		return keys[key];
	}
	return false;
}

void Key_Binding::init()
{
	for (u32 i = 0; i < KEYBOARD_KEY_NUMBER; i++) {
		key_command_list_for_up_keys[i].key = KEY_UNKNOWN;
		key_command_list_for_down_keys[i].key = KEY_UNKNOWN;
	}
}

void Key_Binding::set(const char *command, Key key, bool key_must_be_pressed)
{
	if (key_must_be_pressed) {
		key_command_list_for_up_keys[key].key = key;
		key_command_list_for_up_keys[key].command = command;
	} else {
		key_command_list_for_down_keys[key].key = key;
		key_command_list_for_down_keys[key].command = command;
	}
}

Find_Command_Result Key_Binding::find_command(Key key, bool key_must_be_pressed, String *command)
{
	if ((key_command_list_for_up_keys[key].key == KEY_UNKNOWN) && (key_command_list_for_down_keys[key].key == KEY_UNKNOWN)) {
		print("Key_Binding::get_command: There is no any binding for the key {}.", to_string(key));
		return COMMAND_NOT_FOUND;
	}
	
	if (key_must_be_pressed) {
		if (key_command_list_for_up_keys[key].key != KEY_UNKNOWN) {
			*command = key_command_list_for_up_keys[key].command;
			return COMMAND_FIND;
		} else {
			if (key_command_list_for_down_keys[key].key != KEY_UNKNOWN) {
				return COMMAND_FIND_ON_KEY_DOWN_EVENT;
			}
		}
	} else {
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
