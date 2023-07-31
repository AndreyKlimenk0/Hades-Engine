#ifndef EDITOR_H
#define EDITOR_H

#include "gui.h"
#include "../libs/str.h"
#include "../libs/os/input.h"
#include "../libs/ds/array.h"
#include "../libs/enum_helper.h"
#include "../game/world.h"
#include "../render/render_world.h"

struct Editor;
struct Engine;

typedef Enum_Helper<Entity_Type> Entity_Type_Helper;
typedef Enum_Helper<Light_Type> Light_Type_Helper;
typedef Enum_Helper<Geometry_Type> Geometry_Type_Helper;

struct Editor_Window {
	Editor_Window() {}
	~Editor_Window() {}
	
	Editor *editor = NULL;
	Game_World *game_world = NULL;
	Render_World *render_world = NULL;
	Render_System *render_system = NULL;

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
	// Light entity
	Vector3 position;
	Vector3 direction;
	Vector3 color;

	struct Camera_Fields {
		Vector3 position;
		Vector3 target;
	} camera_fields;

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

struct Game_World_Window : Editor_Window {
	u32 entity_index;
	s32 window_width_delta;
	s32 world_entities_height;
	s32 entity_info_height;
	Window_Style window_style;
	Entity_Type entity_type;

	Gui_Window_Theme world_entities_window_theme;
	Gui_Window_Theme entity_info_window_theme;
	Gui_Text_Button_Theme buttons_theme;

	Hash_Table<u32, bool> draw_AABB_states;
	Hash_Table<u32, bool> draw_frustum_states;
	
	void init(Engine *engine);
	void draw();
	bool draw_entity_list(const char *list_name, u32 list_count, Entity_Type type);
};

struct Render_World_Window : Editor_Window {
	bool debug_cascaded_shadows = false;
	Texture2D shadow_display_texture;

	void init(Engine *engine);
	void update();
	void draw();
};

struct Editor_Command {
	void *additional_info = NULL;
	String command;
};

struct Editor {
	Editor();
	~Editor();

	bool is_draw_make_entity_window = false;
	Game_World *game_world = NULL;
	
	struct Settings {
		float camera_speed = 100.0f;
		float camera_rotation_speed = 0.5f;
	} editor_settings;

	Key_Binding key_command_bindings;
	Entity_Id editor_camera_id;
	Array<Entity_Id> editor_camera_ids;

	Make_Entity_Window make_entity_window;
	Game_World_Window game_world_window;
	Render_World_Window render_world_window;

	void init(Engine *engine);
	void handle_events();
	void render();
	
	void convert_user_input_events_to_edtior_commands(Array<Editor_Command> *editor_commands);
	void convert_editor_commands_to_entity_commands(Array<Editor_Command> *editor_commands, Array<Entity_Command *> *entity_commands);
};
#endif