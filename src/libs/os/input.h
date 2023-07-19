#ifndef INPUT_H
#define INPUT_H

#include "../str.h"
#include "../../win32/win_types.h"

const u32 KEYBOARD_KEY_NUMBER = 255 + 1; // The engine has KEY_UNKNOWN in the key enum
const u32 ENGLISH_ALPHABET_KEYS_OFFSET = 230;

enum Key : u8 {
	KEY_UNKNOWN = 0,
	KEY_LMOUSE,
	KEY_RMOUSE,
	KEY_ENTER,
	KEY_BACKSPACE,
	KEY_HOME,
	KEY_END,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_UP,
	KEY_DOWN,
	KEY_SHIFT, // Insert a new key above KEY_SHIFT
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

const char *to_string(Key key);
Key win32_key_to_engine_key(s32 win32_key_code);

struct Mouse_Input {
	static s32 x;
	static s32 y;
	static s32 last_x;
	static s32 last_y;

	static s32 x_delta();
	static s32 y_delta();
};

struct Key_Input {
	static bool was_char_key_input;
	static bool keys[KEYBOARD_KEY_NUMBER];
	static char inputed_char;
	
	static void setup();
	static void key_down(int key);
	static void key_up(int key);
	static bool is_key_down(int key);
};

struct Key_Command {
	Key key;
	String command;
};

enum Find_Command_Result {
	COMMAND_FIND,
	COMMAND_FIND_ON_KEY_UP_EVENT,
	COMMAND_FIND_ON_KEY_DOWN_EVENT,
	COMMAND_NOT_FOUND
};

struct Key_Binding {
	Key_Command key_command_list_for_up_keys[KEYBOARD_KEY_NUMBER];
	Key_Command key_command_list_for_down_keys[KEYBOARD_KEY_NUMBER];

	void init();
	void set(const char *command, Key key, bool key_must_be_pressed = true);
	Find_Command_Result find_command(Key key, bool key_must_be_pressed, String *command);
};
#endif