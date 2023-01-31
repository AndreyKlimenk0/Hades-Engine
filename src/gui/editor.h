#ifndef EDITOR_H
#define EDITOR_H

#include "../libs/str.h"
#include "../libs/ds/array.h"
#include "../game/world.h"
#include "../libs/enum_helper.h"

struct Engine;

typedef Enum_Helper<Entity_Type> Entity_Type_Helper;
typedef Enum_Helper<Light_Type> Light_Type_Helper;
typedef Enum_Helper<Geometry_Type> Geometry_Type_Helper;

struct Editor_Window {
	Editor_Window() {}
	~Editor_Window() {}
	
	Game_World *game_world = NULL;
	Render_World *render_world = NULL;

	void init(Engine *engine);
};

struct Make_Entity_Window : Editor_Window {
	Make_Entity_Window();
	~Make_Entity_Window();
	
	u32 light_index;
	u32 entity_index;
	u32 geometry_index;
	Box box;
	Sphere sphere;
	Vector3 position;
	Vector3 direction;
	Vector3 color;

	Light_Type_Helper *light_type_helper = NULL;
	Entity_Type_Helper *entity_type_helper = NULL;
	Geometry_Type_Helper *geometry_type_helper = NULL;

	Array<String> light_types;
	Array<String> entity_types;
	Array<String> geometry_types;

	void init(Engine *engine);
	void reset_state();
	void draw();
};

struct Editor {
	Editor();
	~Editor();

	bool is_draw_make_entity_window = false;
	Make_Entity_Window make_entity_window;

	void init();
	void render();
};
#endif