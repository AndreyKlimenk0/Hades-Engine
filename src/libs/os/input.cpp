#include <windows.h>

#include "input.h"

s32 Mouse_State::x = 0;
s32 Mouse_State::y = 0;
s32 Mouse_State::last_x = 0;
s32 Mouse_State::last_y = 0;

s32 Async_Input::mouse_x = 0;
s32 Async_Input::mouse_y = 0;
s32 Async_Input::last_mouse_x = 0;
s32 Async_Input::last_mouse_y = 0;
static Array<s32> mouse_wheels;

bool Keys_State::was_char_key_input;
bool Keys_State::keys[INPUT_KEYS_NUMBER];
char Keys_State::inputed_char;

const char *string_keys[INPUT_KEYS_NUMBER] = {
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
	"KEY_ESC",
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

const char *pretty_string_keys[INPUT_KEYS_NUMBER] = {
	"Unknown key code",
	"Left mouse",
	"Right mouse",
	"Enter",
	"Backspace",
	"Home",
	"End",
	"Arrow up",
	"Arrow down",
	"Arrow left",
	"Arrow right",
	"Ctrl",
	"Alt",
	"Escape",
	"Shift",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z"
};

static u32 key_code_to_index(Key key)
{
	u32 index = 0;
	if (key >= KEY_A) {
		index = ((u8)key - ENGLISH_ALPHABET_KEYS_OFFSET) + (u8)KEY_SHIFT + 1;
	} else if ((key >= KEY_UNKNOWN) && (key <= KEY_SHIFT)) {
		index = (u8)key;
	}
	return index;
}

bool is_alpha_key(Key key)
{
	if (((u8)key >= ENGLISH_ALPHABET_KEYS_OFFSET) && ((u8)key <= 255)) {
		return true;
	}
	return false;
}

const char *to_string(Key key)
{
	return string_keys[key_code_to_index(key)];
}

const char *key_to_pretty_string(Key key)
{
	return pretty_string_keys[key_code_to_index(key)];
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
		case VK_ESCAPE:
			return KEY_ESC;
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

s32 Mouse_State::x_delta()
{
	return Mouse_State::x - Mouse_State::last_x;
}

s32 Mouse_State::y_delta()
{
	return Mouse_State::y - Mouse_State::last_y;
}

void Keys_State::setup()
{
	for (int i = 0; i < INPUT_KEYS_NUMBER; i++) {
		keys[i] = false;
	}
}

void Keys_State::key_down(int key)
{
	keys[key] = true;
}

void Keys_State::key_up(int key)
{
	keys[key] = false;
}

bool Keys_State::is_key_down(int key)
{
	if (key < INPUT_KEYS_NUMBER) {
		return keys[key];
	}
	return false;
}

void Async_Input::new_frame()
{
	mouse_wheels.count = 0;
}

void Async_Input::add_mouse_wheel(s32 wheel_delta)
{
	mouse_wheels.push(wheel_delta);
}

void Async_Input::update_key_state(Key key, Key_State key_state)
{
}

bool Async_Input::is_key_up(Key key)
{
	return false;
}

bool Async_Input::is_key_down(Key key)
{
	return false;
}

Array<s32> *Async_Input::get_mouse_wheels()
{
	return &mouse_wheels;
}
