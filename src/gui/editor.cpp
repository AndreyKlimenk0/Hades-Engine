#include "gui.h"
#include "editor.h"
#include "../sys/sys_local.h"
#include "../sys/engine.h"
#include "../libs/geometry_helper.h"

void Editor_Window::init(Engine *engine)
{
	game_world = &engine->game_world;
	render_world = &engine->render_world;
}

Make_Entity_Window::Make_Entity_Window()
{
	reset_state();
}

Make_Entity_Window::~Make_Entity_Window()
{
	DELETE_PTR(entity_type_helper);
	DELETE_PTR(light_type_helper);
	DELETE_PTR(geometry_type_helper);
}

void Make_Entity_Window::init(Engine *engine)
{
	Editor_Window::init(engine);
	
	set_normal_enum_formatting();
	entity_type_helper = MAKE_ENUM_HELPER(Entity_Type, ENTITY_TYPE_COMMON, ENTITY_TYPE_LIGHT, ENTITY_TYPE_GEOMETRY);
	entity_type_helper->get_string_enums(&entity_types);

	light_type_helper = MAKE_ENUM_HELPER(Light_Type, SPOT_LIGHT_TYPE, POINT_LIGHT_TYPE, DIRECTIONAL_LIGHT_TYPE);
	light_type_helper->get_string_enums(&light_types);

	geometry_type_helper = MAKE_ENUM_HELPER(Geometry_Type, GEOMETRY_TYPE_BOX, GEOMETRY_TYPE_GRID, GEOMETRY_TYPE_SPHERE);
	geometry_type_helper->get_string_enums(&geometry_types);
}

void Make_Entity_Window::reset_state()
{
	light_index = 0;
	entity_index = 0;
	geometry_index = 0;
	box;
	position = { 0.0f, 0.0f, 0.0f };
	direction = { 0.2, -1.0, 0.2f };
	color = { 255.0, 255.0, 255.0 };
}

void Make_Entity_Window::draw()
{
	gui::edit_field("Position", &position);
	gui::list_box(&entity_types, &entity_index);

	Entity_Type type = entity_type_helper->from_string(entity_types[entity_index]);

	if (type == ENTITY_TYPE_COMMON) {

	} else if (type == ENTITY_TYPE_LIGHT) {
		gui::list_box(&light_types, &light_index);
		Light_Type light_type = light_type_helper->from_string(light_types[light_index]);

		if (light_type == DIRECTIONAL_LIGHT_TYPE) {
			gui::edit_field("Direction", &direction);
			gui::edit_field("Color", &color, "R", "G", "B");
			if (gui::button("Make")) {
				game_world->make_direction_light(direction, color);
			}
		}

	} else if (type == ENTITY_TYPE_GEOMETRY) {
		gui::list_box(&geometry_types, &geometry_index);
		Geometry_Type geometry_type = geometry_type_helper->from_string(geometry_types[geometry_index]);

		if (geometry_type == GEOMETRY_TYPE_BOX) {
			gui::edit_field("Width", &box.width);
			gui::edit_field("Height", &box.height);
			gui::edit_field("Depth", &box.depth);

			if (gui::button("Make")) {
				Entity_Id entity_idx = game_world->make_geometry_entity(position, geometry_type, (void *)&box);

				Triangle_Mesh mesh;
				make_box_mesh(&box, &mesh);

				char *mesh_name = format(box.width, box.height, box.depth);
				Mesh_Idx mesh_idx = render_world->add_mesh(mesh_name, &mesh);

				render_world->make_render_entity(entity_idx, mesh_idx);

				free_string(mesh_name);
			}
		} else if (geometry_type == GEOMETRY_TYPE_SPHERE) {
			gui::edit_field("Radious", &sphere.radius);
			gui::edit_field("Slice count", (s32 *)&sphere.slice_count);
			gui::edit_field("Stack count", (s32 *)&sphere.stack_count);

			if (gui::button("Make")) {
				Entity_Id entity_idx = game_world->make_geometry_entity(position, geometry_type, (void *)&sphere);

				Triangle_Mesh mesh;
				make_sphere_mesh(&sphere, &mesh);

				char *mesh_name = format(sphere.radius, sphere.slice_count, sphere.stack_count);
				Mesh_Idx mesh_idx = render_world->add_mesh(mesh_name, &mesh);

				render_world->make_render_entity(entity_idx, mesh_idx);

				free_string(mesh_name);
			}
		}
	}
}

Editor::Editor()
{
}

Editor::~Editor()
{
}

void Editor::init()
{
	Engine *engine = Engine::get_instance();
	make_entity_window.init(engine);
}

void Editor::render()
{
	gui::begin_frame();

	if (gui::begin_window("Editor")) {

		if (gui::add_tab("Game World")) {
			make_entity_window.draw();
		}

		if (gui::add_tab("Camera")) {
			Camera *camera = &Engine::get_render_world()->camera;
			gui::text("Camera Type: Free");
			gui::edit_field("Position", &camera->position);
			gui::edit_field("Direction", &camera->target);

		}

		if (gui::add_tab("Entity List")) {
			Gui_Window_Theme win_theme;
			win_theme.shift_element_from_window_side = 0;
			win_theme.outlines_width = 1.0f;
			win_theme.place_between_elements = 0;
			gui::set_theme(&win_theme);

			Gui_Text_Button_Theme theme;
			theme.rect.width = gui::get_window_size().width;
			theme.color = win_theme.background_color;
			theme.aligment |= 0x08;
			gui::set_theme(&theme);

			static bool state = false;
			Game_World *game_world = Engine::get_game_world();
			static u32 index = 0;
			for (u32 i = 0; i < game_world->geometry_entities.count; i++) {
				char *text = format("Geometry", i);
				if (gui::button(text, &state)) {
					state = true;
					index = i;
				}
				free_string(text);
			}

			gui::reset_button_theme();
			gui::reset_window_theme();

			if (state) {
				if (gui::begin_window("Entity Info")) {
					Geometry_Entity *entity = &game_world->geometry_entities[index];
					print("index = ", index);
					gui::edit_field("Position", &entity->position);
					if (entity->geometry_type == GEOMETRY_TYPE_BOX) {
						gui::edit_field("Width", &entity->box.width);
						gui::edit_field("Height", &entity->box.height);
						gui::edit_field("Depth", &entity->box.depth);
					} else if (entity->geometry_type == GEOMETRY_TYPE_SPHERE) {
						gui::edit_field("Radius", &entity->sphere.radius);
						gui::edit_field("Slice Count", (s32 *)&entity->sphere.slice_count);
						gui::edit_field("Stack Count", (s32 *)&entity->sphere.stack_count);
					} else if (entity->geometry_type == GEOMETRY_TYPE_GRID) {
						gui::edit_field("Width", &entity->grid.width);
						gui::edit_field("Depth", &entity->grid.depth);
						gui::edit_field("Rows count", (s32 *)&entity->grid.rows_count);
						gui::edit_field("Columns count", (s32 *)&entity->grid.columns_count);
					}
					gui::end_window();
				}
			}
		}

		gui::end_window();
	}

	gui::end_frame();
}
