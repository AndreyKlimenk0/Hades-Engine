#include <stdio.h>
#include <stdlib.h>
#include <windowsx.h>

#include "win_time.h"
#include "win_local.h"
#include "win_types.h"

#include "../libs/str.h"
#include "../libs/os/input.h"
#include "../libs/os/event.h"
#include "../libs/os/file.h"
#include "../libs/os/path.h"

#include "../render/font.h"
#include "../render/render_system.h"

#include "../gui/gui.h"
#include "../sys/engine.h"

#include "test.h"

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
	}
	SetCursor(cursor);
}

bool create_win32_window(Win32_Info *win32_state)
{
	const char CLASS_NAME[] = "Sample Window Class";

	WNDCLASS wc = { };

	wc.lpfnWndProc = Win32_Info::win32_procedure;
	wc.hInstance = win32_state->hinstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor = LoadCursor(win32_state->hinstance, MAKEINTRESOURCE(32512));

	RegisterClass(&wc);

	win32_state->window = CreateWindowEx(0, CLASS_NAME,"Hades Engine", WS_OVERLAPPEDWINDOW, 10, 10, 1900, 980, NULL, NULL, win32_state->hinstance, (void *)win32_state);

	if (!win32_state->window) {
		return false;
	}
	
	RECT rect;
	GetWindowRect(win32_state->window, &rect);
	win32_state->window_width = rect.right - rect.left;
	win32_state->window_height = rect.bottom - rect.top;
	return true;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE prev_instance, PWSTR cmd_line, int cmd_show)
{
	Win32_Info win32_info;
	win32_info.hinstance = hInstance;

	if (!create_console(&win32_info)) {
		info("Faield to create win32 console.");
	}
	
	if (!create_win32_window(&win32_info)) {
		error("Failed to create main win32 window.");
	}
	
	ShowWindow(win32_info.window, cmd_show);
	set_cursor(CURSOR_TYPE_ARROW);

	Engine engine;
	engine.init(&win32_info);

	while (1) {
		engine.frame();
	}

	engine.shutdown();
	return 0;
}

LRESULT CALLBACK Win32_Info::win32_procedure(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{	
	Win32_Info *win32_info = NULL;
	if (message == WM_NCCREATE) {
		CREATESTRUCT *cs = (CREATESTRUCT*)lparam;
		win32_info = (Win32_Info *)cs->lpCreateParams;

		SetLastError(0);
		if (SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)win32_info) == 0) {
			if (GetLastError() != 0) {
				return FALSE;
			}
		}
	} else {
		win32_info = (Win32_Info *)GetWindowLongPtr(hwnd, GWL_USERDATA);
	}

	switch (message) {
		case  WM_CLOSE: {
			gui::shutdown();
			ExitProcess(0);
			break;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);
			break;
		}
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
			EndPaint(hwnd, &ps);
			break;
		}
		case WM_SIZE: {
			win32_info->window_width = LOWORD(lparam);
			win32_info->window_height = HIWORD(lparam);
			break;
		}
		case WM_LBUTTONDOWN: {
			SetCapture(win32_info->window);
			push_event(EVENT_TYPE_KEY, VK_LBUTTON, 1);
			break;
		}
		case WM_LBUTTONUP: {
			ReleaseCapture();
			push_event(EVENT_TYPE_KEY, VK_LBUTTON, 0);
			break;
		}
		case WM_RBUTTONDOWN: {
			SetCapture(win32_info->window);
			push_event(EVENT_TYPE_KEY, VK_RBUTTON, 1);
			break;
		}
		case WM_RBUTTONUP: {
			ReleaseCapture();
			push_event(EVENT_TYPE_KEY, VK_RBUTTON, 0);
			break;
		}
		case WM_MOUSEMOVE: {
			int x = GET_X_LPARAM(lparam);
			int y = GET_Y_LPARAM(lparam);

			push_event(EVENT_TYPE_MOUSE, x, y);
			break;
		}
		case WM_SYSKEYDOWN: {
			push_event(EVENT_TYPE_KEY, wparam, 1);
			break;
		}
		case WM_SYSKEYUP: {
			push_event(EVENT_TYPE_KEY, wparam, 0);
			break;
		}
		case WM_KEYDOWN: {
			push_event(EVENT_TYPE_KEY, wparam, 1);
			break;
		}
		case WM_KEYUP:{
			push_event(EVENT_TYPE_KEY, wparam, 0);
			break;
		}
		case WM_CHAR:{
			char c;
			wcstombs((char *)&c, (wchar_t *)&wparam, sizeof(char));
			push_event(EVENT_TYPE_CHAR, c, 0);
			break;
		}
	}
	return DefWindowProc(hwnd, message, wparam, lparam);
}