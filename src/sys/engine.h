#ifndef SYS_ENGINE
#define SYS_ENGINE

#include "file_tracking.h"
#include "../gui/editor.h"
#include "../game/world.h"
#include "../libs/os/path.h"
#include "../render/font.h"
#include "../render/model.h"
#include "../render/render_world.h"
#include "../render/render_system.h"
#include "../render/shader_manager.h"
#include "../win32/win_local.h"

struct Game_World;

enum Engine_Mode {
	GAME_MODE,
	EDITING_MODE
};

struct Engine {
	struct Performance_Displayer {
		Engine *engine = NULL;
		Font *font = NULL;
		Render_Primitive_List render_list;
		
		void init(Engine *_engine);
		void display();
	} performance_displayer;

	bool is_initialized = false;
	s64 fps = 0;
	s64 frame_time = 0;
	Engine_Mode engine_mode;

	Win32_Info win32_info;
	Editor editor;
	Font_Manager font_manager;
	Game_World game_world;
	Render_World render_world;
	Render_System render_sys;
	Shader_Manager shader_manager;
	File_Tracking_System file_tracking_sys;

	void init(Win32_Info *_win32_state);
	void init_from_map();
	void frame();
	void save_to_file();
	void shutdown();

	static bool initialized();
	static void resize_window(u32 window_width, u32 window_height);

	static Engine *get_instance();
	static Game_World *get_game_world();
	static Render_World *get_render_world();
	static Render_System *get_render_system();
	static Font_Manager *get_font_manager();
	static Win32_Info *get_win32_info();
};

struct Camera;

struct Engine_Info {
	Engine_Mode state;

	Camera *get_camera();
};
#endif