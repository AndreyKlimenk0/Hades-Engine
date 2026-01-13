#include "global_config.h"
#include "vars.h"

Global_Config::Global_Config()
{
}

Global_Config::~Global_Config()
{
}

void Global_Config::init(Variable_Service *variable_service)
{
	Variable_Service *rendering = variable_service->find_namespace("rendering");
	ATTACH(rendering, vsync);
	ATTACH(rendering, windowed);
	ATTACH(rendering, back_buffer_count);

	Variable_Service *system = variable_service->find_namespace("system");
	//ATTACH(system, window_width);
	//ATTACH(system, window_height);
	ATTACH(system, level_name);

	Variable_Service *models_loading = variable_service->find_namespace("models_loading");
	ATTACH(models_loading, scene_logging);
	ATTACH(models_loading, assimp_logging);
	ATTACH(models_loading, scaling_value);
	ATTACH(models_loading, use_scaling_value);

	Variable_Service *gui = variable_service->find_namespace("gui");
	ATTACH(gui, font_name);
	ATTACH(gui, font_size);
}