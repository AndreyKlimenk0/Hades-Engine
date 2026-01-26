#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

#include "../libs/str.h"
#include "../libs/number_types.h"

struct Variable_Service;

struct Global_Config {
	Global_Config();
	~Global_Config();

	//rendering
	bool windowed;
	bool vsync;
	s32 back_buffer_count;

	//system
	//s32 window_width;
	//s32 window_height;
	bool create_entities_for_meshes;

	String load_level;

	//loading models
	bool scene_logging;
	bool assimp_logging;
	float scaling_value;
	bool use_scaling_value;

	//gui
	String font_name;
	s32 font_size;

	void init(Variable_Service *variable_service);
};

#endif
