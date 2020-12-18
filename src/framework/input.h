#ifndef INPUT_H
#define INPUT_H

#define KEY_NUMBER 255

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

struct Mouse_Input {
	static int x;
	static int y;
};

struct Key_Input {
	static bool keys[KEY_NUMBER];
	static void init();
	static void key_down(int key);
	static void key_up(int key);
	static bool is_key_down(int key);
};
#endif