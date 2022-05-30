#ifndef SYS_ENGINE
#define SYS_ENGINE

#include "../game/world.h"
#include "../libs/os/path.h"
#include "../render/model.h"
#include "../render/texture.h"
#include "../render/shader.h"
#include "../editor/editor.h"
#include "../render/render_system.h"


struct Event_Handler;

enum Engine_Mode {
	GAME_MODE,
	EDITING_MODE
};

struct Engine {
	Engine_Mode engine_mode;
	
	struct Game {
		World *world = NULL;
		Entity_Manager *entity_maanger = NULL;
	} game;
	
	struct Libs {
		Path *os_path = NULL;
		Editor *editor = NULL;
		Event_Handler *event_handler = NULL;
	} libs;
	
	struct Render {
		Shader_Manager *shader_manager = NULL;
		Texture_Manager *texture_manager = NULL;
		Render_Model_Manager *model_manager = NULL;
		Render_System *render_sys = NULL;
	} render;


	void init();
	void shutdown();

	Event_Handler *get_event_handler();
};

struct Camera;

struct Engine_Info {
	Engine_Mode state;

	Camera *get_camera();
};
#endif