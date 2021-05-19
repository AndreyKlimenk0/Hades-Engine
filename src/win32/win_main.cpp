#include <stdio.h>
#include <windowsx.h>

#include "win_local.h"
#include "../render/effect.h"
#include "../libs/os/input.h"
#include "../libs/os/event.h"
#include "../libs/os/file.h"
#include "../libs/os/path.h"
#include "../render/render_system.h"

#include "test.h"
#include "../libs/os/camera.h"
#include "../game/world.h"

Win32_State win32;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


void create_and_show_window(int nCmdShow)
{
	const char CLASS_NAME[] = "Sample Window Class";

	WNDCLASS wc = { };

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = win32.hinstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(0, CLASS_NAME,"Hades Engine", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, win32.hinstance, NULL
	);

	if (hwnd == NULL) {
		return;
	}
	
	win32.window = hwnd;
	RECT rect;
	GetWindowRect(hwnd, &rect);
	win32.window_height = rect.bottom - rect.top;
	win32.window_width = rect.right - rect.left;

}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
	win32.hinstance = hInstance;
	create_console();

	create_and_show_window(nCmdShow);

	os_path.init();


	//direct3d.init(&win32);
	ShowWindow(win32.window, nCmdShow);



	Key_Input::init();
	test();

	Free_Camera camera;
	camera.init();

	World world;
	
	View_Info *view_info = make_view_info(1.0f, 10000.0f);
	
	render_sys.init(DIRECTX11, view_info);
	render_sys.current_render_world = &world;
	render_sys.free_camera = &camera;
	
	world.init(&camera);
	
	fx_shader_manager.init();

	Input_Layout::init();
	

	while (1) {
		pump_events();
		run_event_loop();
		camera.update();
		render_sys.render_frame();
	}

	//direct3d.shutdown();
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
			render_sys.resize();
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

