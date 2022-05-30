#include "editor.h"

#include "../game/world.h"



void Editor::init()
{
	make_window(100, 200, 500, 500, WINDOW_WITH_HEADER);
	set_window_name("Temp1");
	current_window->set_element_place(PLACE_HORIZONTALLY_AND_IN_MIDDLE);
	make_button("Test1");
	make_button("Test2");
	make_button("Test3");
	make_button("Test5");

	make_window(400, 200, 500, 500, WINDOW_WITH_HEADER);
	set_window_name("Temp2");

	make_button("Test1");
	make_button("Test2");
	make_button("Test3");
	make_button("Test5");

	make_window(600, 200, 500, 500, WINDOW_WITH_HEADER);
	set_window_name("Temp3");

	make_list_box("Test List box");
	add_item("Text 1", 0);
	add_item("Text 1", 1);
	add_item("Text 1", 2);
	add_item("Text 1", 3);

	make_button("Test1");
	make_button("Test2");
	make_button("Test3");
	make_button("Test5");

	make_window(0, 0, win32.window_width, 40, 0);
	current_window->aligment = LEFT_ALIGNMENT;
	current_window->set_element_place(PLACE_HORIZONTALLY_AND_IN_MIDDLE);
	set_window_name("Temp");

	make_window_button("Create Entity", find_window("Temp1"));
	make_button("Create Entity");
	
	make_window_button("Temp2", find_window("Temp2"));
	make_window_button("Temp3", find_window("Temp3"));
	make_button("Camera");

	make_button("Button 3");
	
	make_button("Button 4");

	make_button("Button 4");
	make_button("Button 4");
	make_button("Button 4");
	make_button("Button 4");
	make_button("Button 4");
	make_button("Button 4");
	make_button("Button 4");
	make_button("Button 4");
	make_button("Button 5");
	
	make_window(300, 200, 500, 500, WINDOW_WITH_HEADER);
	current_window->name = "Temp1";

	make_window(1000, 200, 500, 500, WINDOW_WITH_HEADER);
	current_window->name = "Temp2";


	make_window(0, 0, win32.window_width, 50, 0);
	current_window->place = PLACE_HORIZONTALLY;
	current_window->set_alignment(LEFT_ALIGNMENT);

	bind_window("Temp", "Temp1");
	bind_window("Temp", "Temp2");
	make_window_button("Button 1", find_window("Temp"));
	
	make_button("Button 1");
	
	make_window(700, 600, WINDOW_CENTER | WINDOW_WITH_HEADER);
	current_window->place = PLACE_HORIZONTALLY;
	current_window->set_alignment(RIGHT_ALIGNMENT);

	make_button("Button 1");
	
	make_button("Button 2");

	make_button("Button 3");
	
	make_button("Button 4");
	//
	//make_button("Button 4");
	//make_button("Button 4");
	//make_button("Button 4");
	//make_button("Button 4");
	//make_button("Button 4");
	//make_button("Button 4");
	//make_button("Button 4");
	//make_button("Button 4");
	//make_button("Button 5");
	//make_button("Button 5");
	//make_button("Button 5");
	//make_button("Button 5");
	//make_button("Button 5");
	//make_button("Button 5");
	//make_button("Button 5");
	//make_button("Button 5");
	//make_button("Button 5");
	//make_button("Button 5");
	//make_button("Button 5");
	//make_button("Button 5");
	//make_button("Button 5");
	//make_button("Button 5");
	//make_button("Button 5");
	//make_button("Button 5");
}



//void Editor::init()
//{
//	make_window(WINDOW_LEFT | WINDOW_FULL_HEIGHT | WINDOW_AUTO_WIDTH);
//
//	//make_edit_field("Test", EDIT_DATA_FLOAT);
//	//make_vector3_edit_field("Position");
//	//make_button("Ttss");
//	
//	make_form();
//	
//	make_picked_list_box("Entity Type");
//
//	make_vector3_edit_field("Position");
//	
//	make_picked_panel();
//	
//	make_edit_field("radius", EDIT_DATA_INT);
//	
//	make_edit_field("stack slice", EDIT_DATA_INT);
//	make_edit_field("stack count", EDIT_DATA_INT);
//	
//	add_picked_panel("Sphere", ENTITY_TYPE_SPHERE);
//	end_picked_panel();
//
//	make_picked_panel();
//
//	make_edit_field("width", EDIT_DATA_FLOAT);
//	make_edit_field("depth", EDIT_DATA_FLOAT);
//	make_edit_field("cell width", EDIT_DATA_INT);
//	make_edit_field("cell depth", EDIT_DATA_INT);
//	
//	add_picked_panel("Grid", ENTITY_TYPE_GRID); 
//	end_picked_panel();
//
//	make_picked_panel();
//
//	make_list_box("Light Type");
//	
//	add_item("Spot", SPOT_LIGHT_TYPE);
//	add_item("Point", POINT_LIGHT_TYPE);
//	add_item("Directional", DIRECTIONAL_LIGHT_TYPE);
//
//	make_edit_field("range", EDIT_DATA_FLOAT);
//	make_edit_field("radius", EDIT_DATA_FLOAT);
//
//	make_vector3_edit_field("Color");
//	make_vector3_edit_field("Direction");
//
//	add_picked_panel("Light", ENTITY_TYPE_LIGHT);
//	end_picked_panel();
//
//	make_end_picked_list_box();
//	
//	end_form();
//
//	make_window(WINDOW_RIGHT | WINDOW_FULL_HEIGHT | WINDOW_AUTO_WIDTH);
//
//	//make_edit_field("Test", EDIT_DATA_FLOAT);
//
//	make_edit_field("x", &free_camera.position.x);
//	make_edit_field("y", &free_camera.position.y);
//	make_edit_field("z", &free_camera.position.z);
//
//	make_vector3_edit_field("Position", &free_camera.position);
//
//	//make_edit_field("x", &free_camera.forward.x);
//	//make_edit_field("y", &free_camera.forward.y);
//	//make_edit_field("z", &free_camera.forward.z);
//
//
//	//make_picked_list_box("Light Type");
//	//
//	//make_picked_panel();
//	//
//	//make_list_box("Test1");
//	//add_item("Item 1", 0);
//	//add_item("Item 2", 0);
//	//add_item("Item 3", 0);
//
//	//make_edit_field("Field1", EDIT_DATA_FLOAT);
//	//make_edit_field("Field2", EDIT_DATA_FLOAT);
//	//make_edit_field("Field3", EDIT_DATA_FLOAT);
//	//
//	//add_picked_panel("Sphere", ENTI);
//	//
//	//end_picked_panel();
//
//
//	//make_picked_panel();
//	//
//	//make_list_box("Test2");
//	//add_item("Item 21", 0);
//	//add_item("Item 22", 0);
//	//add_item("Item 23", 0);
//
//	//make_edit_field("Field21", EDIT_DATA_FLOAT);
//	//make_edit_field("Field22", EDIT_DATA_FLOAT);
//	//make_edit_field("Field32", EDIT_DATA_FLOAT);
//	//
//	//add_picked_panel("Point Light", POINT_LIGHT_TYPE);
//	//
//	//end_picked_panel();
//
//
//	//make_list_box("AAAA");
//	//add_item("Item A1", 0);
//	//add_item("Item A2", 0);
//	//add_item("Item A3", 0);
//
//
//	//make_form();
//	//set_form_label("Test2");
//	//make_list_box("Test2");
//	//add_item("Item 21", 0);
//	//add_item("Item 22", 0);
//	//add_item("Item 23", 0);
//	//
//	//end_form();
//
//	//make_form();
//	//set_form_label("Test3");
//	//make_list_box("Test3");
//	//add_item("Item 31", 0);
//	//add_item("Item 32", 0);
//	//add_item("Item 33", 0);
//	//
//	//end_form();
//
//	
//	
//	//make_form();
//	//make_list_box("Entity Type");
//	//add_item("Default", ENTITY_TYPE_UNKNOWN);
//	//add_item("Floor",  ENTITY_TYPE_FLOOR);
//	//add_item("Light",  ENTITY_TYPE_LIGHT);
//	//add_item("Mutant", ENTITY_TYPE_MUTANT);
//	//add_item("Sodier", ENTITY_TYPE_SOLDIER);
//
//	//end_form();
//
//	//set_form_label("Mutant");
//	//not_draw_form();
//	
//	//make_vector3_edit_field();
//
//	//end_form();
//
//	//make_form();
//	//set_form_label("Light");
//	//not_draw_form();
//
//	//make_list_box("Position");
//	//add_item("Form 1", ENTITY_TYPE_UNKNOWN);
//	//add_item("Form 2", ENTITY_TYPE_FLOOR);
//	//add_item("Form 3", ENTITY_TYPE_LIGHT);
//	//add_item("Form 4", ENTITY_TYPE_MUTANT);
//	//add_item("Form 5", ENTITY_TYPE_SOLDIER);
//
//	//make_list_box("Position");
//	//add_item("Form 1", ENTITY_TYPE_UNKNOWN);
//	//add_item("Form 2", ENTITY_TYPE_FLOOR);
//	//add_item("Form 3", ENTITY_TYPE_LIGHT);
//	//add_item("Form 4", ENTITY_TYPE_MUTANT);
//	//add_item("Form 5", ENTITY_TYPE_SOLDIER);
//
//	//end_form();
//
//
//
//	//make_list_box("Entity Type");
//	//add_item("Text 1", ENTITY_TYPE_UNKNOWN);
//	//add_item("Text 2", ENTITY_TYPE_FLOOR);
//	//add_item("Text 3", ENTITY_TYPE_LIGHT);
//	//add_item("Text 4", ENTITY_TYPE_MUTANT);
//	//add_item("Text 5", ENTITY_TYPE_SOLDIER);
//
//	//make_edit_field("Position");
//	//make_edit_field("X");
//	//make_edit_field("Y");
//	//make_edit_field("Z");
//
//	//make_button("Button 1");
//	//make_button("Button 2");
//	//make_button("Button 3");
//	
//	//current_window->aligning_input_fields();
//	
//	//make_window(WINDOW_RIGHT | WINDOW_FULL_HEIGHT | WINDOW_AUTO_WIDTH);
//
//	//make_list_box("Position");
//	//add_item("Text 1", ENTITY_TYPE_UNKNOWN);
//	//add_item("Text 2", ENTITY_TYPE_FLOOR);
//	//add_item("Text 3", ENTITY_TYPE_LIGHT);
//	//add_item("Text 4", ENTITY_TYPE_MUTANT);
//	//add_item("Text 5", ENTITY_TYPE_SOLDIER);
//
//	//make_list_box("Position");
//	//add_item("Text 1", ENTITY_TYPE_UNKNOWN);
//	//add_item("Text 2", ENTITY_TYPE_FLOOR);
//	//add_item("Text 3", ENTITY_TYPE_LIGHT);
//	//add_item("Text 4", ENTITY_TYPE_MUTANT);
//	//add_item("Text 5", ENTITY_TYPE_SOLDIER);
//
//	//make_edit_field("Position");
//	//make_edit_field("X");
//	//make_edit_field("Y");
//	//make_edit_field("Z");
//	
//	//make_button("Button 11");
//	//make_button("Button 12");
//	//make_button("Button 13");
//
//	//current_window->aligning_input_fields();
//}
