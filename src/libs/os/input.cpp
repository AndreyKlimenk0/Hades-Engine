#include <windows.h>

#include "input.h"
#include "../../sys/sys_local.h"

s32 Mouse_Async_Info::x = 0;
s32 Mouse_Async_Info::y = 0;
s32 Mouse_Async_Info::last_x = 0;
s32 Mouse_Async_Info::last_y = 0;

bool Key_Async_Info::was_char_key_input;
bool Key_Async_Info::keys[KEYBOARD_KEY_NUMBER];
char Key_Async_Info::inputed_char;

const char *string_keys[KEYBOARD_KEY_NUMBER] = {
	"KEY_UNKNOWN",
	"KEY_LMOUSE",
	"KEY_RMOUSE",
	"KEY_ENTER",
	"KEY_BACKSPACE",
	"KEY_HOME",
	"KEY_END",
	"KEY_ARROW_UP",
	"KEY_ARROW_DOWN",
	"KEY_ARROW_LEFT",
	"KEY_ARROW_RIGHT",
	"KEY_CTRL",
	"KEY_ALT",
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

bool is_alpha_key(Key key) 
{
	if (((u8)key >= ENGLISH_ALPHABET_KEYS_OFFSET) && ((u8)key <= 255)) {
		return true;
	}
	return false;
}

const char *to_string(Key key)
{
	u32 index = 0;
	if (key >= KEY_A) {
		index = ((u8)key - ENGLISH_ALPHABET_KEYS_OFFSET) + (u8)KEY_SHIFT + 1;
	} else if ((key >= KEY_UNKNOWN) && (key <= KEY_SHIFT)) {
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
			return KEY_ARROW_UP;
		case VK_DOWN:
			return KEY_ARROW_DOWN;
		case VK_RIGHT:
			return KEY_ARROW_RIGHT;
		case VK_LEFT:
			return KEY_ARROW_LEFT;
		case VK_CONTROL: 
			return KEY_CTRL;
		case VK_MENU:
			return KEY_ALT;
	}
	return KEY_UNKNOWN;
}

void Mouse_Info::set(s32 _x, s32 _y)
{
	last_x = x;
	last_y = y;
	x = _x;
	y = _y;
}

s32 Mouse_Info::x_delta()
{
	return x - last_x;
}

s32 Mouse_Info::y_delta()
{
	return y - last_y;
}

s32 Mouse_Async_Info::x_delta()
{
	return Mouse_Async_Info::x - Mouse_Async_Info::last_x;
}

s32 Mouse_Async_Info::y_delta()
{
	return Mouse_Async_Info::y - Mouse_Async_Info::last_y;
}

void Key_Async_Info::setup()
{
	for (int i = 0; i < KEYBOARD_KEY_NUMBER; i++) {
		keys[i] = false;
	}
}

void Key_Async_Info::key_down(int key)
{
	keys[key] = true;
}

void Key_Async_Info::key_up(int key)
{
	keys[key] = false;
}

bool Key_Async_Info::is_key_down(int key)
{
	if (key < KEYBOARD_KEY_NUMBER) {
		return keys[key];
	}
	return false;
}
