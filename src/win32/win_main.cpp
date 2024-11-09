#include <windows.h>
#include <windowsx.h>

#include "test.h"
#include "win_helpers.h"
#include "win_console.h"
#include "../sys/sys.h"
#include "../sys/engine.h"
#include "../libs/os/event.h"

int WINAPI wWinMain(HINSTANCE hinstance, HINSTANCE prev_instance, PWSTR cmd_line, int cmd_show)
{
	if (!create_console(hinstance)) {
		info("Faield to create win32 console.");
	}

	Engine engine;
	engine.init_base();
	Variable_Service *system = engine.var_service.find_namespace("system");

	s32 window_width = CW_USEDEFAULT;
	s32 window_height = CW_USEDEFAULT;
	
	ATTACH(system, window_width);
	ATTACH(system, window_height);

	Win32_Window window;
	if (!create_win32_window(hinstance, &window, window_width, window_height)) {
		error("Failed to create main win32 window.");
	}

	ShowWindow(window.handle, cmd_show);
	set_cursor(CURSOR_TYPE_ARROW);

	engine.init(&window);

	//Test
	test();

	while (1) {
		engine.frame();
	}

	engine.shutdown();
	return 0;
}

LRESULT CALLBACK Win32_Window::procedure(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	Win32_Window *window = NULL;
	if (message == WM_NCCREATE) {
		CREATESTRUCT *cs = (CREATESTRUCT *)lparam;
		window = (Win32_Window *)cs->lpCreateParams;

		SetLastError(0);
		if (SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window) == 0) {
			if (GetLastError() != 0) {
				return FALSE;
			}
		}
	} else {
		window = (Win32_Window *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	}

	switch (message) {
		case  WM_CLOSE: {
			Engine::get_instance()->shutdown();
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
			window->width = LOWORD(lparam);
			window->height = HIWORD(lparam);
			Engine::resize_window(window->width, window->height);
			break;
		}
		case WM_LBUTTONDOWN: {
			SetCapture(window->handle);
			push_event(EVENT_TYPE_KEY, VK_LBUTTON, 1);
			break;
		}
		case WM_LBUTTONUP: {
			ReleaseCapture();
			push_event(EVENT_TYPE_KEY, VK_LBUTTON, 0);
			break;
		}
		case WM_RBUTTONDOWN: {
			SetCapture(window->handle);
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
		case WM_MOUSEWHEEL: {
			push_event(EVENT_TYPE_MOUSE_WHEEL, GET_WHEEL_DELTA_WPARAM(wparam), 0);
			break;
		}
		case WM_SYSKEYDOWN: {
			push_event(EVENT_TYPE_KEY, (int)wparam, 1);
			break;
		}
		case WM_SYSKEYUP: {
			push_event(EVENT_TYPE_KEY, (int)wparam, 0);
			break;
		}
		case WM_KEYDOWN: {
			push_event(EVENT_TYPE_KEY, (int)wparam, 1);
			break;
		}
		case WM_KEYUP: {
			push_event(EVENT_TYPE_KEY, (int)wparam, 0);
			break;
		}
		case WM_CHAR: {
			push_event(EVENT_TYPE_CHAR, (int)wparam, 0);
			break;
		}
		case WM_LBUTTONDBLCLK: {
			push_event(EVENT_TYPE_DOUBLE_CLICK, 1, 0);
			break;
		}
		case WM_RBUTTONDBLCLK: {
			push_event(EVENT_TYPE_DOUBLE_CLICK, 0, 1);
			break;
		}
	}
	return DefWindowProc(hwnd, message, wparam, lparam);
}