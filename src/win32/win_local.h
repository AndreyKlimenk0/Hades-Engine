#ifndef WIN_LOCAL_H
#define WIN_LOCAL_H

#include <windows.h>

struct Render_System;

struct Win32_Info {
	HWND window;
	int window_width;
	int window_height;
	HINSTANCE hinstance;

	static LRESULT CALLBACK win32_procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

bool create_console(Win32_Info *win32_state);
void append_text_to_text_buffer(const char *text);
void get_window_size(HWND window, int *width, int *height);

enum Cursor_Type {
	CURSOR_TYPE_ARROW,
	CURSOR_TYPE_CROSS,
	CURSOR_TYPE_MOVE,
	CURSOR_TYPE_RESIZE_LEFT_RIGHT,
	CURSOR_TYPE_RESIZE_TOP_BUTTOM,
	CURSOR_TYPE_RESIZE_TOP_RIGHT,
	CURSOR_TYPE_RESIZE_TOP_LEFT,
};

void set_cursor(Cursor_Type type);

#endif