#ifndef EDITOR_H
#define EDITOR_H

#include "gui.h"
#include "enum_helper.h"
#include "../game/world.h"
#include "../libs/str.h"
#include "../libs/os/input.h"
#include "../libs/key_binding.h"
#include "../libs/image/image.h"
#include "../libs/structures/array.h"
#include "../libs/math/structures.h"
#include "../render/render_world.h"

struct Editor;
struct Texture;
struct Engine;

typedef Enum_Helper<Entity_Type> Entity_Type_Helper;
typedef Enum_Helper<Light_Type> Light_Type_Helper;
typedef Enum_Helper<Geometry_Type> Geometry_Type_Helper;

struct Editor_Window {
	Editor_Window();
	virtual ~Editor_Window();

	Editor *editor = NULL;
	Game_World *game_world = NULL;
	Render_World *render_world = NULL;
	Render_System *render_system = NULL;

	bool window_open = false;
	Rect_s32 window_rect;
	String name;

	virtual void init(Engine *engine);
	virtual void init(const char *_name, Engine *engine);
	virtual void open();
	virtual void close();
	virtual void draw() = 0;

	void set_position(s32 x, s32 y);
	void set_size(s32 width, s32 height);
};

struct Top_Right_Window : Editor_Window {
	Gui_Window_Theme window_theme;

	void init(const char *_name, Engine *engine);
};

struct Entity_Window : Top_Right_Window {
	void init(Engine *engine);
	void display_light(Light *light);
	void display_sun_earth(u32 earth_radius, u32 sun_radius, u32 orbit_radius, const Point_s32 &position, Light *light, Render_Primitive_List *render_list);
	void draw();
};

struct Entity_Tree_Window : Top_Right_Window {
	Gui_Tree_Theme tree_theme;

	void init(Engine *engine);
	void draw();

	template <typename T>
	void draw_entity_list(Array<T> &entity_list, const char *name);
};

struct Displaying_Command {
	String command_name;
	String str_key_binding;

	bool(*display_info_and_get_command_args)(String *edit_field, Array<String> &command_args, void *context) = NULL;
};

struct Command_Window : Editor_Window {
	Command_Window();
	~Command_Window();

	bool window_just_open = false;
	bool active_edit_field = false;
	Displaying_Command *current_displaying_command = NULL;

	String command_edit_field;
	Rect_s32 command_window_rect;
	Rect_s32 command_window_rect_with_additional_info;

	Gui_List_Theme list_theme;
	Gui_Window_Theme command_window_theme;
	Gui_Edit_Field_Theme command_edit_field_theme;
	Array<Gui_List_Line_State> list_line_states;

	Array<Displaying_Command> displaying_commands;
	Array<Pair<Displaying_Command *, Key_Binding>> command_key_bindings;

	void init(Engine *engine);
	void open();
	void close();

	void displaying_command(const char *command_name, bool(*display_info_and_get_command_args)(String *edit_field, Array<String> &command_args, void *context));
	void displaying_command(const char *command_name, Key modified_key, Key second_key, bool(*display_info_and_get_command_args)(String *edit_field, Array<String> &command_args, void *context));
	//void register_command_key_bindings(Key_Bindings *key_bindings);
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

	Editor_Mode_Type editor_mode = EDITOR_MODE_COMMON;

	Entity_Id picked_entity;
	Entity_Id editor_camera_id;

	Render_System *render_sys = NULL;
	Game_World *game_world = NULL;
	Render_World *render_world = NULL;
	Point_s32 mouse_position;

	struct Settings {
		float camera_speed = 50.0f;
		float camera_rotation_speed = 0.5f;
	} editor_settings;

	struct Left_Bar {
		struct Images {
			Texture *adding = NULL;
			Texture *entity = NULL;
			Texture *entities = NULL;
			Texture *rendering = NULL;
		} textures;
		Gui_Window_Theme window_theme;
		Gui_Image_Button_Theme button_theme;
	} left_bar;

	Key_Bindings key_bindings;
	Key_Command_Bindings key_command_bindings;

	Left_Bar left_buttons;
	Entity_Window entity_window;
	Entity_Tree_Window entities_window;
	Command_Window command_window;

	Array<Editor_Window *> windows;
	Array<Editor_Window *> top_right_windows;

	void init(Engine *engine);
	void init_left_bar();

	void handle_events();
	void update();
	void picking();

	void render();
	void render_menus();
	void render_left_bar();

	void open_or_close_right_window(Editor_Window *window);
	void convert_user_input_events_to_edtior_commands(Array<Editor_Command> *editor_commands);
	void convert_editor_commands_to_entity_commands(Array<Editor_Command> *editor_commands, Array<Entity_Command *> *entity_commands);
};
#endif