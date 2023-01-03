#ifndef SYS_ENGINE
#define SYS_ENGINE

#include "../game/world.h"
#include "../libs/os/path.h"
#include "../render/model.h"
#include "../render/font.h"
#include "../render/render_system.h"
#include "../win32/win_local.h"


struct Event_Handler;
struct Game_World;

enum Engine_Mode {
	GAME_MODE,
	EDITING_MODE
};

struct Engine {
	s64 fps = 0;
	s64 frame_time = 0;

	Engine_Mode engine_mode;
	Win32_Info win32_info;
	Font font;
	World world;
	Entity_Manager entity_maanger;
	Render_Model_Manager model_manager;
	Render_System render_sys;

	void init(Win32_Info *_win32_state);
	void frame();
	void shutdown();

	Event_Handler *get_event_handler();
};

u32 get_window_width();
u32 get_window_height();

struct Camera;

struct Engine_Info {
	Engine_Mode state;

	Camera *get_camera();
};
#endif