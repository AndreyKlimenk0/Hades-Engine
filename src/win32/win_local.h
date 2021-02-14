#ifndef WIN_LOCAL_H
#define WIN_LOCAL_H

#include <windows.h>

struct Win32_State {
	HWND window;
	int window_width;
	int window_height;
	HINSTANCE hinstance;
};

extern Win32_State win32;

void append_text_to_text_edit_buffer(const char *text);
void create_console();
#endif