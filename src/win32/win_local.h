#ifndef WIN_LOCAL_H
#define WIN_LOCAL_H

#include <windows.h>

struct Win32_State {
	HWND window;
	int window_width;
	int window_height;
	HINSTANCE hinstance;
};


struct Win_Console {
	HWND window;
	HBRUSH background_color;
};

extern Win32_State win32;
extern Win_Console win_console;

void create_console();
#endif