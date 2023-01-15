#ifndef EDITOR_H
#define EDITOR_H

#include "../libs/str.h"
#include "../libs/ds/array.h"
#include "../game/world.h"
#include "../libs/enum_helper.h"

typedef Enum_Helper<Entity_Type> Entity_Type_Helper;
typedef Enum_Helper<Light_Type> Light_Type_Helper;
typedef Enum_Helper<Geometry_Type> Geometry_Type_Helper;

struct Editor {
	Editor();
	~Editor();
	
	bool is_draw_make_entity_window = false;

	Game_World *game_world = NULL;
	Render_World *render_world = NULL;
	
	Light_Type_Helper *light_type_helper = NULL;
	Entity_Type_Helper *entity_type_helper = NULL;
	Geometry_Type_Helper *geometry_type_helper = NULL;

	Array<String> light_types;
	Array<String> entity_types;
	Array<String> geometry_types;
	
	void init();
	void draw_make_entity_window();
	void render();
};
#endif