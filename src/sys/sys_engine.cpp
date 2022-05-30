#include "sys_engine.h"

void Engine::init()
{
	//game.world = new World();
	//game.world->init();

	//libs.os_path = new Path();
	//libs.os_path->init();

	//render.shader_manager = new Shader_Manager();
	//render.shader_manager->init();

	//render.texture_manager = new Texture_Manager();
	//render.texture_manager->init();

	//render.render_sys = new Render_System();
	//render.render_sys->init();
}

void Engine::shutdown()
{
}

Event_Handler * Engine::get_event_handler()
{
	return nullptr;
}
