#ifndef SYS_ENGINE
#define SYS_ENGINE

#include "../game/world.h"
#include "../libs/os/path.h"
#include "../render/font.h"
#include "../render/model.h"
#include "../render/render_world.h"
#include "../render/render_system.h"
#include "../win32/win_local.h"
#include "../gui/editor.h"


struct Game_World;

enum Engine_Mode {
	GAME_MODE,
	EDITING_MODE
};

struct Engine {
	bool is_initialized = false;
	s64 fps = 0;
	s64 frame_time = 0;
	Engine_Mode engine_mode;

	Font font;
	Editor editor;
	Win32_Info win32_info;
	
	Game_World game_world;
	Render_World render_world;
	
	Render_System render_sys;
	Render_Model_Manager model_manager;

	void init(Win32_Info *_win32_state);
	void frame();
	void shutdown();

	static bool initialized();
	static void resize_window(u32 window_width, u32 window_height);

	static Engine *get_instance();
	static Font *get_font();
	static Game_World *get_game_world();
	static Render_World *get_render_world();
	static Render_System *get_render_system();
};

u32 get_window_width();
u32 get_window_height();

struct Camera;

struct Engine_Info {
	Engine_Mode state;

	Camera *get_camera();
};
#endif