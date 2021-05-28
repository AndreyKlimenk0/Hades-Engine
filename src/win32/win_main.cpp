#include <stdio.h>
#include <windowsx.h>

#include "win_local.h"
#include "win_types.h"

#include "../libs/str.h"
#include "../libs/os/input.h"
#include "../libs/os/event.h"
#include "../libs/os/file.h"
#include "../libs/os/path.h"

#include "../render/effect.h"
#include "../render/directx.h"
#include "../render/render_system.h"

#include "../editor/editor.h"

#include "test.h"

Win32_State win32;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

inline s64 cpu_ticks_counter()
{
	LARGE_INTEGER ticks;
	if (!QueryPerformanceCounter(&ticks)) {
		print("Can't get the information about tichs counter");
		return 0;
	}
	return ticks.QuadPart;
}

inline s64 cpu_ticks_per_second()
{
	LARGE_INTEGER count_ticks_per_second;
	if (!QueryPerformanceFrequency(&count_ticks_per_second)) {
		print("Can't get the information about count ticks per second");
		return 0;
	}
	return count_ticks_per_second.QuadPart;

}

inline u32 microseconds_counter()
{
	s64 ticks = cpu_ticks_counter();
	s64 ticks_per_second = cpu_ticks_per_second();
	return (1000 * 1000 * ticks) / ticks_per_second;
}

inline u32 milliseconds_counter()
{
	s64 ticks = cpu_ticks_counter();
	s64 ticks_per_second = cpu_ticks_per_second();
	return (1000 * ticks) / ticks_per_second;
}

void create_and_show_window(int nCmdShow)
{
	const char CLASS_NAME[] = "Sample Window Class";

	WNDCLASS wc = { };

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = win32.hinstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(0, CLASS_NAME,"Hades Engine", WS_OVERLAPPEDWINDOW,
		10, 10, 900, 850, NULL, NULL, win32.hinstance, NULL
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
	direct2d.draw_text(x, y, formatted_text);
	DELETE_ARRAY(formatted_text);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
	win32.hinstance = hInstance;
	create_console();

	create_and_show_window(nCmdShow);

	os_path.init();

	direct_write.init("Consolas", 12, Color::White);
	directx11.init();
	direct2d.init(directx11.swap_chain);

	ShowWindow(win32.window, nCmdShow);

	fx_shader_manager.init();

	Input_Layout::init();

	Key_Input::init();
	
	test();

	Free_Camera camera;
	camera.init();

	World world;
	world.init(&camera);
	
	View_Info *view_info = make_view_info(1.0f, 10000.0f);
	
	render_sys.init(view_info);
	render_sys.current_render_world = &world;
	render_sys.free_camera = &camera;

	Editor editor;
	editor.init();

	s64 count_per_s = cpu_ticks_per_second();


	while (1) {
		s64 t = cpu_ticks_counter();
		u32 last = milliseconds_counter();
		u32 l = microseconds_counter();

		pump_events();
		run_event_loop();
		editor.handle_event();
		
		camera.update();
		editor.update();

		directx11.begin_draw();
		direct2d.begin_draw();
		
		render_sys.render_frame();	

		editor.draw();

		u32 result = milliseconds_counter() - last;
		u32 r = microseconds_counter() - l;
		s64 x = cpu_ticks_counter() - t;
		s64 fps = cpu_ticks_per_second() / x;
		display_text(700, 5, "Fps {} ms", fps);
		display_text(700, 20, "Microseconds elapsed {} ms", r);
		
		direct2d.end_draw();
		directx11.end_draw();
	}

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
			
			directx11.resize(&direct2d);
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
		case WM_RBUTTONDOWN: {
			SetCapture(win32.window);
			push_event(EVENT_TYPE_KEY, VK_RBUTTON, 1);
			return 0;
		}
		case WM_RBUTTONUP: {
			ReleaseCapture();
			push_event(EVENT_TYPE_KEY, VK_RBUTTON, 0);
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

