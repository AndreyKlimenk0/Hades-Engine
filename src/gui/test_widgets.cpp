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
		init_guiv2(size.width, size.height, "FiraCode-Regular", 12, &render_sys->render_2d);

		init_widgets();
	}
	begin_frame();
	set_alignment(ALIGNMENT_CENTER);
	
	begin_ui_element("Main plane");
	set_size(fixed_size(1400), fixed_size(800));
	set_background_color(Color(128, 128, 128));
	set_padding(Padding(15));
	
	if (button("Button 1")) {
		print("Click by button 1");
	}
	if (button("Button 2")) {
		print("Click by button 2");
	}

	Array<String> entity_types;
	entity_types.push("Entity");
	entity_types.push("Camera");
	entity_types.push("Person");
	static u32 index;
	list_box("Entity Type", entity_types, &index);

	text("Entity Type");
	end_ui_element(); //Main plane

	end_frame();
}
