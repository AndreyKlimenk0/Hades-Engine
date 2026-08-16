#include "test_widgets.h"
#include "test_gui.h"
#include "guiv2.h"
#include "../sys/engine.h"
#include "widgets.h"

using namespace imgui;

void draw_test_widgets()
{
	static bool init_gui = true;
	if (init_gui) {
		init_gui = false;
		Engine *engine = Engine::get_instance();
		Render_System *render_sys = &engine->render_sys;
		Size_u32 size = render_sys->get_window_size();
		init_guiv2(size.width, size.height, "Consola", 12, &render_sys->render_2d);

		init_widgets();
	}
	begin_frame();
	set_alignment(ALIGNMENT_CENTER);
	
	begin_ui_element("Main plane");
	set_size(fixed_size(900), fixed_size(600));
	//set_background_color(Color(128, 128, 128));
	set_background_color(Color(45));
	set_padding(Padding(15));
	
	if (button("Button 1")) {
		print("Click by button 1");
	}
	if (button("Button 2")) {
		print("Click by button 2");
	}
	static Array<String> entity_types;
	static bool init_array = false;
	if (!init_array) {
		init_array = true;

		entity_types.push("Entity");
		entity_types.push("Camera");
		entity_types.push("Person");
		entity_types.push("Monster");
		entity_types.push("Hunter");
		entity_types.push("Doom guy");
	}
	static u32 index;
	list_box("Entity Type", entity_types, &index);

	if (button("Button 3")) {
		print("Click by button 2");
	}

	static bool draw_aabb = false;
	check_box("Draw AABB", &draw_aabb);

	static float value0 = 36.0f;
	static float value1 = -36.0f;
	static float value2 = -18.0f;
	static float value3 = 18.0f;
	static float value4 = 0.0f;
	slider("Test 0", -36.0f, 36.0f, &value0);
	slider("Test 1", -36.0f, 36.0f, &value1);
	//slider("Test 2", -36.0f, 36.0f, &value2);
	//slider("Test 3", -36.0f, 36.0f, &value3);
	//slider("Test 4", -36.0f, 36.0f, &value4);

	if (button("Button 4")) {
	}

	static String buffer;
	if (edit_field("Position", &buffer)) {
		print("Update position value:", buffer);
	}

	static String buffer1 = "Scale";
	if (edit_field("Scale", &buffer1)) {
		print("Update scale buffer:", buffer1);
	}

	static float angle = 352.1f;

	if (edit_field("Angle", &angle)) {
		print("Update Angle", angle);
	}

	static int health = 88;
	if (edit_field("health", &health)) {
		print("update health", health);
	}

	static Vector3 entity_position = { 100.0f, 200.0f, 300.0f };
	if (edit_field("Entity Position", &entity_position)) {
		print("Update entity position", &entity_position);
	}

	end_ui_element(); //Main plane

	end_frame();
}
