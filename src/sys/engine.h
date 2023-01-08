#ifndef SYS_ENGINE
#define SYS_ENGINE

#include "../game/world.h"
#include "../libs/os/path.h"
#include "../render/font.h"
#include "../render/model.h"
#include "../render/render_world.h"
#include "../render/render_system.h"
#include "../win32/win_local.h"

struct Game_World;

enum Engine_Mode {
	GAME_MODE,
	EDITING_MODE
};

struct Engine {
	s64 fps = 0;
	s64 frame_time = 0;
	Engine_Mode engine_mode;

	Font font;
	Win32_Info win32_info;
	
	Game_World game_world;
	Render_World render_world;
	
	Render_System render_sys;
	Render_Model_Manager model_manager;

	void init(Win32_Info *_win32_state);
	void frame();
	void shutdown();

	static Font *get_font();
	static Render_World *get_render_world();
};

u32 get_window_width();
u32 get_window_height();

struct Camera;

struct Engine_Info {
	Engine_Mode state;

	Camera *get_camera();
};
#endif