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

void create_console();
void append_text_to_text_buffer(const char *text);
void get_window_size(HWND window, int *width, int *height);

enum Cursor_Type {
	CURSOR_TYPE_ARROW,
	CURSOR_TYPE_CROSS,
	CURSOR_TYPE_RESIZE_LEFT_RIGHT,
	CURSOR_TYPE_RESIZE_TOP_BUTTON,
	CURSOR_TYPE_RESIZE_TOP_RIGHT,
	CURSOR_TYPE_RESIZE_TOP_LEFT,
};

void set_cursor(Cursor_Type type);
#endif