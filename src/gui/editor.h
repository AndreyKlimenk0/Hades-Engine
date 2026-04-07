#ifndef EDITOR_H
#define EDITOR_H

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

struct Displaying_Command {
	String command_name;
	String str_key_binding;

	bool(*display_info_and_get_command_args)(String *edit_field, Array<String> &command_args, void *context) = NULL;
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
		float camera_speed = 0.5f;
		float camera_rotation_speed = 0.5f;
	} editor_settings;

	struct Left_Bar {
		struct Images {
			Texture *adding = NULL;
			Texture *entity = NULL;
			Texture *entities = NULL;
			Texture *rendering = NULL;
		} textures;
	} left_bar;

	Key_Bindings key_bindings;
	Key_Command_Bindings key_command_bindings;

	Left_Bar left_buttons;

	Array<Editor_Window *> windows;
	Array<Editor_Window *> top_right_windows;

	void init(Engine *engine);

	void handle_events();
	void update();
	void picking();

	void render();
	void render_menus();
	void render_left_bar();

	void convert_user_input_events_to_edtior_commands(Array<Editor_Command> *editor_commands);
	void convert_editor_commands_to_entity_commands(Array<Editor_Command> *editor_commands, Array<Entity_Command *> *entity_commands);
};
#endif