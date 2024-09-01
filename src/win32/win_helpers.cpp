#include "win_helpers.h"

void set_cursor(Cursor_Type type)
{
	HCURSOR cursor = LoadCursor(NULL, IDC_ARROW);
	switch (type) {
	case CURSOR_TYPE_ARROW: {
		cursor = LoadCursor(NULL, IDC_ARROW);
		break;
	}
	case CURSOR_TYPE_CROSS: {
		cursor = LoadCursor(NULL, IDC_CROSS);
		break;
	}
	case CURSOR_TYPE_RESIZE_LEFT_RIGHT: {
		cursor = LoadCursor(NULL, IDC_SIZEWE);
		break;
	}
	case CURSOR_TYPE_RESIZE_TOP_BUTTOM: {
		cursor = LoadCursor(NULL, IDC_SIZENS);
		break;
	}
	case CURSOR_TYPE_RESIZE_TOP_RIGHT: {
		cursor = LoadCursor(NULL, IDC_SIZENESW);
		break;
	}
	case CURSOR_TYPE_RESIZE_TOP_LEFT: {
		cursor = LoadCursor(NULL, IDC_SIZENWSE);
		break;
	}
	case CURSOR_TYPE_MOVE: {
		cursor = LoadCursor(NULL, IDC_SIZEALL);
		break;
	}
	}
	SetCursor(cursor);
}

void get_window_size(HWND window, int *width, int *height)
{
	RECT rect;
	GetClientRect(window, &rect);
	*width = rect.right - rect.left;
	*height = rect.bottom - rect.top;
}

bool create_win32_window(HINSTANCE hinstance, Win32_Window *window, s32 window_width, s32 window_height)
{
	const char CLASS_NAME[] = "Sample Window Class";

	WNDCLASS wc = { };

	wc.lpfnWndProc = Win32_Window::procedure;
	wc.hInstance = hinstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor = LoadCursor(hinstance, MAKEINTRESOURCE(32512));

	RegisterClass(&wc);

	window->handle = CreateWindowEx(0, CLASS_NAME, "Hades Engine", WS_OVERLAPPEDWINDOW, 10, 10, window_width, window_height, NULL, NULL, hinstance, (void *)window);

	if (window->handle) {
		RECT rect;
		GetWindowRect(window->handle, &rect);
		if ((rect.right > rect.left) && (rect.bottom > rect.top)) {
			window->width = rect.right - rect.left;
			window->height = rect.bottom - rect.top;
			return true;
		}
	}
	return false;
}