#include <stdio.h>
#include <windowsx.h>

#include "win_local.h"
#include "../render/effect.h"
#include "../render/render_frame.h"
#include "../framework/input.h"
#include "../framework/event.h"
#include "../framework/camera.h"
#include "../framework/file.h"

#include "../render/vertex.h"
#include "../libs/math/matrix.h"
#include "../libs/ds/queue.h"
#include "../render/mesh.h"
#include "../libs/fbx_loader.h"

Win32_State win32;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void create_and_show_window(Win32_State *win32, int nCmdShow)
{
	const char CLASS_NAME[] = "Sample Window Class";

	WNDCLASS wc = { };

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = win32->hinstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(0, CLASS_NAME,"Learn to Program Windows", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, win32->hinstance, NULL
	);

	if (hwnd == NULL) {
		return;
	}
	
	win32->window = hwnd;
	RECT rect;
	GetWindowRect(hwnd, &rect);
	win32->window_height = rect.bottom - rect.top;
	win32->window_width = rect.right - rect.left;
}

void set_up_conlose_in_out()
{
	AllocConsole();
	freopen("conin$", "r", stdin);
	freopen("conout$", "w", stdout);
	freopen("conout$", "w", stderr);
	HWND consoleHandle = GetConsoleWindow();
	MoveWindow(consoleHandle, 1, 1, 680, 480, 1);

}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{

	//set_up_conlose_in_out();

	Matrix4 test1 = Matrix4(Vector4(1, 2, 3, 4), Vector4(5, 6, 7, 8), Vector4(9, 10, 11, 12), Vector4(13, 14, 15, 16));
	Matrix4 test2 = Matrix4(Vector4(11, 22, 33, 44), Vector4(55, 66, 77, 88), Vector4(11, 22, 33, 44), Vector4(55, 66, 77, 88));
	Matrix4 test3 = Matrix4(Vector4(1, 0, 0, 1), Vector4(0, 2, 1, 2), Vector4(2, 1, 0, 1), Vector4(2, 0, 1, 4));
	//test3.inverse();
	//Matrix4 result = test1 * test2;
	//print_mat(result);

	win32.hinstance = hInstance;
	
	init_base_path();
	create_and_show_window(&win32, nCmdShow);
	
	direct3d.init(&win32);

	
	ShowWindow(win32.window, nCmdShow);

	Key_Input::init();
	Input_Layout::init();

	Free_Camera camera;
	camera.init();

	create_console();

	Render_World world;
	world.init(&camera);
	
	while (1) {
		pump_events();
		run_event_loop();
		camera.update();
		world.render_world();
	}

	direct3d.shutdown();
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_DESTROY: {
			PostQuitMessage(0);
			return 0;
		}
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
			EndPaint(hwnd, &ps);
			return 0;
		}
		case WM_SIZE: {
			RECT rect;
			GetWindowRect(win32.window, &rect);
			win32.window_height = rect.bottom - rect.top;
			win32.window_width = rect.right - rect.left;
			direct3d.resize(&win32);
			return 0;
		}
		case WM_LBUTTONDOWN: {
			SetCapture(win32.window);
			push_event(EVENT_TYPE_KEY, VK_LBUTTON, 1);
			return 0;
		}
		case WM_LBUTTONUP: {
			ReleaseCapture();
			push_event(EVENT_TYPE_KEY, VK_LBUTTON, 0);
			return 0;
		}
		case WM_MOUSEMOVE: {
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			push_event(EVENT_TYPE_MOUSE, x, y);
			return 0;
		}
		case WM_SYSKEYDOWN: {
			push_event(EVENT_TYPE_KEY, wParam, 1);
			return 0;
		}
		case WM_SYSKEYUP: {
			push_event(EVENT_TYPE_KEY, wParam, 0);
			return 0;
		}
		case WM_KEYDOWN: {
			push_event(EVENT_TYPE_KEY, wParam, 1);
			return 0;
		}
		case WM_KEYUP:{
			push_event(EVENT_TYPE_KEY, wParam, 0);
			return 0;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

