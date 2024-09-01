#ifndef WIN_HELPERS_H
#define WIN_HELPERS_H

#include <windows.h>
#include "../libs/number_types.h"

struct Win32_Window {
	HWND handle;
	int width;
	int height;

	static LRESULT CALLBACK procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

bool create_win32_window(HINSTANCE hinstance, Win32_Window *window, s32 window_width, s32 window_height);
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