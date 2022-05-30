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
#include "../render/directx.h"
#include "../render/texture.h"
#include "../render/render_system.h"

#include "../editor/editor.h"

#include "test.h"


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
		10, 10, 1900, 980, NULL, NULL, win32.hinstance, NULL
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

template <typename... Args>
void display_text(int x, int y, Args... args)
{
	char *formatted_text = format(args...);
	draw_text(x, y, formatted_text);
	DELETE_ARRAY(formatted_text);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
	win32.hinstance = hInstance;
	create_console();


	create_and_show_window(nCmdShow);

	os_path.init();

	
	directx11.init();
	
	//direct_write.init("Consolas", 11, Color::Red);
	//direct_write.init_characters();
	
	//direct2d.init(directx11.swap_chain);
	
	font.init(11);


	texture_manager.init();


	ShowWindow(win32.window, nCmdShow);

	Key_Input::init();

	editor.init();

	//World world;
	world.init();
	
	View_Info *view_info = make_view_info(1.0f, 10000.0f);
	
	render_sys.init(view_info);
	render_sys.current_render_world = &world;
	render_sys.free_camera = &editor.free_camera;

	Input_Layout::init();

	// Test
	test();

	s64 count_per_s = cpu_ticks_per_second();

	while (1) {
		s64 t = cpu_ticks_counter();
		s64 last = milliseconds_counter();
		s64 l = microseconds_counter();

		pump_events();
		run_event_loop();
		
		editor.update();

		directx11.begin_draw();
		//direct2d.begin_draw();
		
		render_sys.render_frame();	

		//editor.draw();
		
		s64 result = milliseconds_counter() - last;
		s64 r = microseconds_counter() - l;
		s64 x = cpu_ticks_counter() - t;
		s64 fps = cpu_ticks_per_second() / x;
		
		char *s = format("fps{}", fps);
		draw_text(700, 0, s);
		free_string(s);
		//display_text(700, 70, "Fps {} ms", fps);
		//display_text(700, 90, "Microseconds elapsed {} ms", r);
		//display_text(700, 60, "Mouse X {} and Y {}", Mouse_Input::x, Mouse_Input::y);
		
		//draw_text(0, 0, "dotaISNOTGAMEJ");
		//draw_text(10, 70, "ahrgpbqy");
		//draw_text(10, 150, "klimenko");
		
		//direct2d.end_draw();
		directx11.end_draw();
	}

	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
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
			win32.window_width = LOWORD(lParam);
			win32.window_height = HIWORD(lParam);
			
			directx11.resize();
			render_sys.resize();
			
			break;
		}
		case WM_LBUTTONDOWN: {
			SetCapture(win32.window);
			push_event(EVENT_TYPE_KEY, VK_LBUTTON, 1);
			break;
		}
		case WM_LBUTTONUP: {
			ReleaseCapture();
			push_event(EVENT_TYPE_KEY, VK_LBUTTON, 0);
			break;
		}
		case WM_RBUTTONDOWN: {
			SetCapture(win32.window);
			push_event(EVENT_TYPE_KEY, VK_RBUTTON, 1);
			break;
		}
		case WM_RBUTTONUP: {
			ReleaseCapture();
			push_event(EVENT_TYPE_KEY, VK_RBUTTON, 0);
			break;
		}
		case WM_MOUSEMOVE: {
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			push_event(EVENT_TYPE_MOUSE, x, y);
			break;
		}
		case WM_SYSKEYDOWN: {
			push_event(EVENT_TYPE_KEY, wParam, 1);
			break;
		}
		case WM_SYSKEYUP: {
			push_event(EVENT_TYPE_KEY, wParam, 0);
			break;
		}
		case WM_KEYDOWN: {
			push_event(EVENT_TYPE_KEY, wParam, 1);
			break;
		}
		case WM_KEYUP:{
			push_event(EVENT_TYPE_KEY, wParam, 0);
			break;
		}
		case WM_CHAR:{
			char c;
			wcstombs((char *)&c, (wchar_t *)&wParam, sizeof(char));
			push_event(EVENT_TYPE_CHAR, c, 0);
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

