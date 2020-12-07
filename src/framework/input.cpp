#include <windows.h>

#include "input.h"

int Mouse_Input::x = 0;
int Mouse_Input::y = 0;
int Mouse_Input::last_x = 0;
int Mouse_Input::last_y = 0;

bool Key_Input::keys[KEY_NUMBER];

void Key_Input::init()
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