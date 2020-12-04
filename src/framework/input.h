#ifndef INPUT_H
#define INPUT_H

#define KEY_NUMBER 255

struct Mouse_Input {
	static int x;
	static int y;
	static int last_x;
	static int last_y;
};

struct Key_Input {
	static bool keys[KEY_NUMBER];
	static void init();
	static void key_down(int key);
	static void key_up(int key);
	static bool is_key_down(int key);
};
#endif