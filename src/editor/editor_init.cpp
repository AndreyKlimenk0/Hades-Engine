#include "editor.h"

#include "../game/entity.h"
#include "../game/world.h"

void Editor::init()
{
	make_window(WINDOW_LEFT | WINDOW_FULL_HEIGHT | WINDOW_AUTO_WIDTH);

	//make_list_box("Test");
	//add_item("Item 1", 0);
	//add_item("Item 2", 0);
	//add_item("Item 3", 0);

	//make_edit_field("Test", EDIT_DATA_FLOAT);

	//make_list_box("Test2");
	//add_item("Item 1", 0);
	//add_item("Item 2", 0);
	//add_item("Item 3", 0);

	//make_edit_field("Test2", EDIT_DATA_FLOAT);

	make_picked_list_box("Light Type");
	
	make_picked_panel();
	
	make_list_box("Test1");
	add_item("Item 1", 0);
	add_item("Item 2", 0);
	add_item("Item 3", 0);

	make_edit_field("Field1", EDIT_DATA_FLOAT);
	make_edit_field("Field2", EDIT_DATA_FLOAT);
	make_edit_field("Field3", EDIT_DATA_FLOAT);
	
	add_picked_panel("Directional Light", DIRECTIONAL_LIGHT_TYPE);
	
	end_picked_panel();


	make_picked_panel();
	
	make_list_box("Test2");
	add_item("Item 21", 0);
	add_item("Item 22", 0);
	add_item("Item 23", 0);

	make_edit_field("Field21", EDIT_DATA_FLOAT);
	make_edit_field("Field22", EDIT_DATA_FLOAT);
	make_edit_field("Field32", EDIT_DATA_FLOAT);
	
	add_picked_panel("Point Light", POINT_LIGHT_TYPE);
	
	end_picked_panel();




	//make_form();
	//set_form_label("Test2");
	//make_list_box("Test2");
	//add_item("Item 21", 0);
	//add_item("Item 22", 0);
	//add_item("Item 23", 0);
	//
	//end_form();

	//make_form();
	//set_form_label("Test3");
	//make_list_box("Test3");
	//add_item("Item 31", 0);
	//add_item("Item 32", 0);
	//add_item("Item 33", 0);
	//
	//end_form();

	
	
	//make_form();
	//make_list_box("Entity Type");
	//add_item("Default", ENTITY_TYPE_UNKNOWN);
	//add_item("Floor",  ENTITY_TYPE_FLOOR);
	//add_item("Light",  ENTITY_TYPE_LIGHT);
	//add_item("Mutant", ENTITY_TYPE_MUTANT);
	//add_item("Sodier", ENTITY_TYPE_SOLDIER);

	//end_form();

	//set_form_label("Mutant");
	//not_draw_form();
	
	//make_vector3_edit_field();

	//end_form();

	//make_form();
	//set_form_label("Light");
	//not_draw_form();

	//make_list_box("Position");
	//add_item("Form 1", ENTITY_TYPE_UNKNOWN);
	//add_item("Form 2", ENTITY_TYPE_FLOOR);
	//add_item("Form 3", ENTITY_TYPE_LIGHT);
	//add_item("Form 4", ENTITY_TYPE_MUTANT);
	//add_item("Form 5", ENTITY_TYPE_SOLDIER);

	//make_list_box("Position");
	//add_item("Form 1", ENTITY_TYPE_UNKNOWN);
	//add_item("Form 2", ENTITY_TYPE_FLOOR);
	//add_item("Form 3", ENTITY_TYPE_LIGHT);
	//add_item("Form 4", ENTITY_TYPE_MUTANT);
	//add_item("Form 5", ENTITY_TYPE_SOLDIER);

	//end_form();



	//make_list_box("Entity Type");
	//add_item("Text 1", ENTITY_TYPE_UNKNOWN);
	//add_item("Text 2", ENTITY_TYPE_FLOOR);
	//add_item("Text 3", ENTITY_TYPE_LIGHT);
	//add_item("Text 4", ENTITY_TYPE_MUTANT);
	//add_item("Text 5", ENTITY_TYPE_SOLDIER);

	//make_edit_field("Position");
	//make_edit_field("X");
	//make_edit_field("Y");
	//make_edit_field("Z");

	//make_button("Button 1");
	//make_button("Button 2");
	//make_button("Button 3");
	
	//current_window->aligning_input_fields();
	
	make_window(WINDOW_RIGHT | WINDOW_FULL_HEIGHT | WINDOW_AUTO_WIDTH);

	//make_list_box("Position");
	//add_item("Text 1", ENTITY_TYPE_UNKNOWN);
	//add_item("Text 2", ENTITY_TYPE_FLOOR);
	//add_item("Text 3", ENTITY_TYPE_LIGHT);
	//add_item("Text 4", ENTITY_TYPE_MUTANT);
	//add_item("Text 5", ENTITY_TYPE_SOLDIER);

	//make_list_box("Position");
	//add_item("Text 1", ENTITY_TYPE_UNKNOWN);
	//add_item("Text 2", ENTITY_TYPE_FLOOR);
	//add_item("Text 3", ENTITY_TYPE_LIGHT);
	//add_item("Text 4", ENTITY_TYPE_MUTANT);
	//add_item("Text 5", ENTITY_TYPE_SOLDIER);

	//make_edit_field("Position");
	//make_edit_field("X");
	//make_edit_field("Y");
	//make_edit_field("Z");
	
	//make_button("Button 11");
	//make_button("Button 12");
	//make_button("Button 13");

	//current_window->aligning_input_fields();
}
