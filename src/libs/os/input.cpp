#include <windows.h>

#include "input.h"


s32 Mouse_Input::x = 0;
s32 Mouse_Input::y = 0;

bool Key_Input::was_char_key_input;
bool Key_Input::keys[KEY_NUMBER];
char Key_Input::inputed_char;

void Key_Input::setup()
{
	for (int i = 0; i < KEY_NUMBER; i++) {
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
	if (key < KEY_NUMBER) {
		return keys[key];
	}
	return false;
}

Hades_Keys win32_key_to_engine_key(u32 win32_key_code)
{
	switch (win32_key_code) {
		case VK_LBUTTON:
			return KEY_LMOUSE;
		case VK_RBUTTON:
			return KEY_RMOUSE;
		case 0x41:
			return KEY_A;
		case 0x44:
			return KEY_D;
		case 0x45:
			return KEY_E;
		case 0x51:
			return KEY_Q;
		case 0x53:
			return KEY_S;
		case 0x57:
			return KEY_W;
	}
	return KEY_UNKNOWN;
}
