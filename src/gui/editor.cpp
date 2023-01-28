#include "gui.h"
#include "editor.h"
#include "../sys/sys_local.h"
#include "../sys/engine.h"
#include "../libs/geometry_generator.h"


inline void reverse_state(bool *state)
{
	if (*state) {
		*state = false;
	} else {
		*state = true;
	}
}

Editor::Editor()
{
}

Editor::~Editor()
{
	DELETE_PTR(entity_type_helper);
	DELETE_PTR(light_type_helper);
}

void Editor::init()
{
	game_world = Engine::get_game_world();
	render_world = Engine::get_render_world();

	set_normal_enum_formatting();
	entity_type_helper = MAKE_ENUM_HELPER(Entity_Type, ENTITY_TYPE_COMMON, ENTITY_TYPE_LIGHT, ENTITY_TYPE_GEOMETRY);
	entity_type_helper->get_string_enums(&entity_types);

	light_type_helper = MAKE_ENUM_HELPER(Light_Type, SPOT_LIGHT_TYPE, POINT_LIGHT_TYPE, DIRECTIONAL_LIGHT_TYPE);
	light_type_helper->get_string_enums(&light_types);

	geometry_type_helper = MAKE_ENUM_HELPER(Geometry_Type, GEOMETRY_TYPE_BOX, GEOMETRY_TYPE_GRID);
	geometry_type_helper->get_string_enums(&geometry_types);
}

void Editor::draw_make_entity_window()
{
	if (gui::begin_window("Make Entity")) {
		
		static u32 index = 0;
		
		gui::list_box(&entity_types, &index);
		Entity_Type type = entity_type_helper->from_string(entity_types[index]);

		if (type == ENTITY_TYPE_COMMON) {
			gui::button("Entity type common");
		} else if (type == ENTITY_TYPE_LIGHT) {
			static u32 light_index = 0;
			gui::list_box(&light_types, &light_index);

			Light_Type light_type = light_type_helper->from_string(light_types[light_index]);
			if (light_type == DIRECTIONAL_LIGHT_TYPE) {
				//game_world->make_direction_light();
				static float x = 0.0f;
				static float y = 0.0f;
				static float z = 0.0f;
				gui::text("Direction");
				gui::same_line();
				gui::edit_field("X", &x);
				gui::edit_field("Y", &y);
				gui::edit_field("Z", &z);
			}
		} else if (type == ENTITY_TYPE_GEOMETRY) {
			static u32 geometry_type_index = 0;
			gui::list_box(&geometry_types, &geometry_type_index);

			Geometry_Type geometry_type = geometry_type_helper->from_string(geometry_types[geometry_type_index]);

			if (geometry_type == GEOMETRY_TYPE_BOX) {
				static Box box;
				gui::edit_field("Width", &box.width);
				gui::edit_field("Height", &box.height);
				gui::edit_field("Depth", &box.depth);

				if (gui::button("Make")) {
					Entity_Id entity_id = game_world->make_geometry_entity(Vector3(0.0f, 0.0f, 0.0f), geometry_type, (void *)&box);

					Triangle_Mesh mesh;
					generate_box(&box, &mesh);
					
					char *mesh_name = format(box.width, box.height, box.depth);
					Mesh_Id mesh_id = render_world->add_mesh(mesh_name, &mesh);
					
					render_world->make_render_entity(entity_id, mesh_id);

					free_string(mesh_name);
				}
			}
			//static Box box;
			//gui::edit_field()

		}

		gui::end_window();
	}
}

void Editor::render()
{
	gui::begin_frame();

	if (gui::begin_window("Editor")) {

		if (gui::add_tab("Game World")) {
			if (gui::button("Make Entity")) {
				reverse_state(&is_draw_make_entity_window);
			}

			if (is_draw_make_entity_window) {
				draw_make_entity_window();
			}
		}

		if (gui::add_tab("Camera")) {
			Camera *camera = &render_world->camera;
			gui::text("Camera Type: Free");
			gui::button("temp1");
			gui::edit_field("Position", &camera->position);
			gui::edit_field("Direction", &camera->target);
			gui::button("temp2");
		}

		gui::end_window();
	}

	gui::end_frame();
}
