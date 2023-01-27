#ifndef INPUT_H
#define INPUT_H

#include "../ds/array.h"
#include "../../win32/win_types.h"

const u32 KEY_NUMBER = 255;


enum Keys {
	Key_A = 0x41,
	Key_B,
	Key_C,
	Key_D,
	Key_E,
	Key_F,
	Key_G,
	Key_H,
	Key_I,
	Key_J,
	Key_K,
	Key_L,
	Key_M,
	Key_N,
	Key_O,
	Key_P,
	Key_Q,
	Key_R,
	Key_S,
	Key_T,
	Key_U,
	Key_V,
	Key_W,
	Key_X,
	Key_Y,
	Key_Z,
};

enum Hades_Keys {
	KEY_UNKNOWN = 0,
	KEY_LMOUSE,
	KEY_RMOUSE,
	KEY_W,
	KEY_S,
	KEY_A,
	KEY_D,
	KEY_E,
	KEY_Q,
};

Hades_Keys win32_key_to_engine_key(u32 win32_key_code);


struct Mouse_Input {
	static s32 x;
	static s32 y;
};

struct Key_Input {
	static bool was_char_key_input;
	static bool keys[KEY_NUMBER];
	static char inputed_char;
	static void init();
	static void key_down(int key);
	static void key_up(int key);
	static bool is_key_down(int key);
};
#endif