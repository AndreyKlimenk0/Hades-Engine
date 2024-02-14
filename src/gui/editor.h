#ifndef EDITOR_H
#define EDITOR_H

#include "gui.h"
#include "../game/world.h"
#include "../libs/str.h"
#include "../libs/os/input.h"
#include "../libs/ds/array.h"
#include "../libs/enum_helper.h"
#include "../libs/key_binding.h"
#include "../libs/math/structures.h"
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
	s32 window_width_delta;
	s32 world_entities_height;
	s32 entity_info_height;
	Window_Style window_style;

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
	bool show_cascaded_shadow_frustums = false;
	Texture2D shadow_display_texture;
	
	struct Draw_Cascade_Info {
		Entity_Id entity_id;
		u32 cascaded_shadow_map_index;
		u32 shadow_cascade_index;
	};
	Array<Draw_Cascade_Info> frustum_entity_ids;

	void init(Engine *engine);
	void update();
	void draw();
};

struct Drop_Down_Entity_Window : Editor_Window {
	Size_u32 window_size;
	Point_s32 mouse_position; // When happens the right button mouse click by a picked entity, the variable stores current mouse position.
	
	Gui_Window_Theme window_theme;
	Gui_Text_Button_Theme buttons_theme;

	void init(Engine *engine);
	void draw();
};

struct Command_Window : Editor_Window {
	String text;
	Rect_s32 window_rect;
	Gui_Window_Theme window_theme;

	void init(Engine *engine);
	void draw();
};

struct Editor_Command {
	void *additional_info = NULL;
	String command;
};

enum Editor_Mode_Type {
	EDITOR_MODE_COMMON,
	EDITOR_MODE_MOVE_ENTITY,
	EDITOR_MODE_ROTATE_ENTITY,
	EDITOR_MODE_SCALE_ENTITY,
};

struct Editor {
	Editor();
	~Editor();

	bool draw_command_window = true;
	bool draw_make_entity_window = false;
	bool draw_drop_down_entity_window = false;
	Editor_Mode_Type editor_mode = EDITOR_MODE_COMMON;

	Entity_Id picked_entity;
	Entity_Id editor_camera_id;
	
	Render_System *render_sys = NULL;
	Game_World *game_world = NULL;
	Render_World *render_world = NULL;

	Gui_ID game_world_tab_gui_id;
	
	struct Settings {
		float camera_speed = 5.0f;
		float camera_rotation_speed = 0.5f;
	} editor_settings;

	Key_Bindings key_bindings;
	Key_Command_Bindings key_command_bindings;

	Command_Window command_window;
	Make_Entity_Window make_entity_window;
	Game_World_Window game_world_window;
	Render_World_Window render_world_window;
	Drop_Down_Entity_Window drop_down_entity_window;

	void init(Engine *engine);
	void handle_events();
	void update();
	void render();
	void picking();
	
	void convert_user_input_events_to_edtior_commands(Array<Editor_Command> *editor_commands);
	void convert_editor_commands_to_entity_commands(Array<Editor_Command> *editor_commands, Array<Entity_Command *> *entity_commands);
};
#endif