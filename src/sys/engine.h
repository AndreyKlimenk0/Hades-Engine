#ifndef ENGINE_H
#define ENGINE_H

#include "vars.h"
#include "file_tracking.h"
#include "global_config.h"
#include "../gui/editor.h"
#include "../game/world.h"
#include "../win32/win_helpers.h"
#include "../render/render_world.h"
#include "../render/render_system.h"
#include "../render/shader_manager.h"
#include "../render/ui_storage.h"

#include "../libs/str.h"
#include "../libs/number_types.h"

struct Engine {
	struct Swap_Chain_Present {
		u32 sync_interval = 0;
		u32 flags = 0;
	} swap_chain_present;

	bool is_initialized = false;
	String current_level_name;
	
	Global_Config global_config;
	Variable_Service var_service;
	File_Tracking_System file_tracking_sys;
	
	Editor editor;
	Game_World game_world;
	//Rendering
	UI_Storage ui_storage;
	Render_System render_sys;
	Render_World render_world;
	Shader_Manager shader_manager;

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
	static Variable_Service *get_variable_service();
	static Global_Config *get_global_config();
};

#endif
