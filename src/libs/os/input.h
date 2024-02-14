#ifndef INPUT_H
#define INPUT_H

#include "../str.h"
#include "../ds/array.h"
#include "../../win32/win_types.h"

const u32 INPUT_KEYS_NUMBER = 255 + 1; // The engine has KEY_UNKNOWN in the key enum
const u32 ENGLISH_ALPHABET_KEYS_OFFSET = 230;

enum Key_State {
	KEY_UP,
	KEY_DOWN,
};

enum Key : u8 {
	KEY_UNKNOWN = 0,
	KEY_LMOUSE,
	KEY_RMOUSE,
	KEY_ENTER,
	KEY_BACKSPACE,
	KEY_HOME,
	KEY_END,
	KEY_ARROW_UP,
	KEY_ARROW_DOWN,
	KEY_ARROW_LEFT,
	KEY_ARROW_RIGHT,
	KEY_CTRL,
	KEY_ALT,
	KEY_SHIFT, // Insert a new key above KEY_SHIFT and don't forget update string_keys in the cpp file
	KEY_A = ENGLISH_ALPHABET_KEYS_OFFSET,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z
};

bool is_alpha_key(Key key);
const char *to_string(Key key);
Key win32_key_to_engine_key(s32 win32_key_code);

struct Key_Info {
	Key key;
	Key_State key_state;
};

struct Mouse_Info {
	s32 x = 0;
	s32 y = 0;
	s32 last_x = 0;
	s32 last_y = 0;

	void set(s32 _x, s32 _y);
	s32 x_delta();
	s32 y_delta();
};

struct Mouse_State {
	static s32 x;
	static s32 y;
	static s32 last_x;
	static s32 last_y;

	static s32 x_delta();
	static s32 y_delta();
};

struct Keys_State {
	static bool was_char_key_input;
	static bool keys[INPUT_KEYS_NUMBER];
	static char inputed_char;
	
	static void setup();
	static void key_down(int key);
	static void key_up(int key);
	static bool is_key_down(int key);
};

struct Async_Input {
	static s32 mouse_x;
	static s32 mouse_y;
	static s32 last_mouse_x;
	static s32 last_mouse_y;

	static void new_frame();
	static void add_mouse_wheel(s32 wheel_delta);
	static void update_key_state(Key key, Key_State key_state);
	static bool is_key_up(Key key);
	static bool is_key_down(Key key);
	static Array<s32> *get_mouse_wheels();
};

#endif