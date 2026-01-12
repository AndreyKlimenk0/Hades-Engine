#ifndef ENGINE_H
#define ENGINE_H

#include "vars.h"
#include "file_tracking.h"
#include "../gui/editor.h"
#include "../game/world.h"
#include "../win32/win_helpers.h"
#include "../render/font.h"
#include "../render/render_world.h"
#include "../render/render_system.h"
#include "../render/shader_manager.h"

#include "../libs/str.h"
#include "../libs/number_types.h"

struct Engine {
	struct Swap_Chain_Present {
		u32 sync_interval = 0;
		u32 flags = 0;
	} swap_chain_present;

	bool is_initialized = false;
	String current_level_name;
	
	Editor editor;
	Variable_Service var_service;
	File_Tracking_System file_tracking_sys;
	Game_World game_world;
	Render_System render_sys;
	Render_World render_world;
	Font_Manager font_manager;
	Shader_Manager shader_manager;

	void init_base();
	void init(Win32_Window *window);
	void frame();
	void shutdown();

	void set_current_level_name(const String &level_name);

	static void resize_window(u32 window_width, u32 window_height);
	static bool initialized();

	static Engine *get_instance();
	static Game_World *get_game_world();
	static Render_World *get_render_world();
	static Render_System *get_render_system();
	static Font_Manager *get_font_manager();
	static Variable_Service *get_variable_service();
};

#endif
