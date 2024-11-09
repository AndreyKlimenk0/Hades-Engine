#include <assert.h>

#include "gui.h"
#include "test_gui.h"
#include "../libs/os/event.h"
#include "../libs/os/input.h"
#include "../sys/sys.h"
#include "../libs/str.h"
#include "../libs/image/image.h"
#include "../libs/os/event.h"
#include "../libs/os/input.h"
#include "../libs/math/vector.h"
#include "../libs/structures/array.h"

using namespace gui;

void draw_test_tab_window()
{
	static Vector3 pos = Vector3::zero;
	static Vector3 dir = Vector3::zero;

	bool draw_first_tab = false;
	bool draw_second_tab = false;
	if (begin_window("Test tab window", WINDOW_DEFAULT_STYLE | WINDOW_TAB_BAR)) {
		if (add_tab("First tab")) {
			edit_field("Pos", &pos);
			edit_field("Dir", &dir);

			for (int i = 0; i < 10; i++) {
				char *button_name = format("fbutton {}", i);
				button(button_name);
				free_string(button_name);
			}
			draw_first_tab = true;
		}
		if (add_tab("Second tab")) {
			for (int i = 0; i < 10; i++) {
				char *button_name = format("sbutton {}", i);
				button(button_name);
				free_string(button_name);
			}
			draw_second_tab = true;
		}
		end_window();
	}
	assert(!(draw_first_tab && draw_second_tab));
	assert(!(!draw_first_tab && !draw_second_tab));
}

void set_to_zero(Array<Gui_List_Line_State> &list_line_states)
{
	memset((void *)list_line_states.items, 0, sizeof(Gui_List_Line_State) * list_line_states.size);
}

struct Directory_Entry {
	String name;
	Array<String> files;
	Array<Directory_Entry *> dirs;
};

struct Selected_Directory_Entiry {
	Directory_Entry *directory_entiry = NULL;
	Array<u32> dirs;
	Array<u32> files;
};


void draw_directory_tree(Directory_Entry *dir)
{
	//if (begin_tree_node(dir->name)) {
	//	for (u32 i = 0; i < dir->dirs.count; i++) {
	//		draw_directory_tree(dir->dirs[i]);
	//	}
	//	for (u32 i = 0; i < dir->files.count; i++) {
	//		if (begin_tree_node(dir->files[i])) {
	//			end_tree_node();
	//		}
	//	}
	//	end_tree_node();
	//}
}

static Point_s32 window_pos;
static bool display_option_window = false;
static bool display_option_window_first_frame = false;

void display_window(u32 selected_node_counter, Array<Selected_Directory_Entiry> &selected_entries)
{
	if (!display_option_window) {
		return;
	}
	assert(selected_node_counter > 0);

	if (display_option_window_first_frame) {
		make_next_ui_element_active();
	}
	set_next_window_pos(window_pos.x, window_pos.y);
	set_next_window_size(200, 200);
	if (begin_window("Option window", WINDOW_OUTLINES)) {
		const char *button_text = selected_node_counter > 1 ? "delete files" : "delete file";
		if (button(button_text)) {
			for (u32 i = 0; i < selected_entries.count; i++) {
				Selected_Directory_Entiry *selected_entiry = &selected_entries[i];
				for (u32 j = 0; j < selected_entiry->files.count; j++) {
					u32 index = selected_entiry->files[j];
					if (j == 0) {
						index = selected_entiry->files[j];
					} else {
						index = selected_entiry->files[j] - 1;
					}
					selected_entiry->directory_entiry->files.remove(index);
				}
			}
			display_option_window = false;
			window_pos = Point_s32(0, 0);
		}
		if (!display_option_window_first_frame && !mouse_over_element() && (was_click(KEY_LMOUSE) || was_click(KEY_RMOUSE))) {
			display_option_window = false;
			window_pos = Point_s32(0, 0);
		}
		end_window();
	}
	if (display_option_window_first_frame) {
		display_option_window_first_frame = false;
	}
}

void display_directory(Directory_Entry *directory, u32 *selected_node_counter, Array<Selected_Directory_Entiry> &selected)
{
	if (begin_tree_node(directory->name)) {
		Selected_Directory_Entiry temp;
		temp.directory_entiry = directory;

		for (u32 i = 0; i < directory->dirs.count; i++) {
			display_directory(directory->dirs[i], selected_node_counter, selected);
		}
		for (u32 i = 0; i < directory->files.count; i++) {
			if (begin_tree_node(directory->files[i], GUI_TREE_NODE_FINAL)) {
				if (tree_node_selected()) {
					*selected_node_counter += 1;
					temp.files.push(i);
				}
				if (tree_node_selected() && mouse_over_element() && was_click(KEY_RMOUSE)) {
					display_option_window = true;
					display_option_window_first_frame = true;
					window_pos = Point_s32(Mouse_State::x, Mouse_State::y);
				}
				end_tree_node();
			}
		}
		end_tree_node();
		selected.push(temp);
	}
}
void display_directory_tree(Directory_Entry *directory)
{
	u32 selected_node_counter = 0;
	Array<Selected_Directory_Entiry> selected;
	if (begin_tree("Directory Tree")) {
		display_directory(directory, &selected_node_counter, selected);
		end_tree();
	}
	display_window(selected_node_counter, selected);
}

void draw_test_list_window()
{
	if (begin_window("Test list window")) {
		static bool init = false;
		static Array<Gui_List_Line_State> list_line;
		static Array<String> file_names;
		static Array<String> file_types;
		static Array<String> file_sizes;
		if (!init) {
			init = true;
			list_line.reserve(20);
			set_to_zero(list_line);
			for (int i = 0; i < 20; i++) {
				char *file_name = format("file_name_{}", i);
				char *file_type = format("hlsl_{}", i);
				char *file_size = to_string(i * 1000);

				file_names.push(file_name);
				file_types.push(file_type);
				file_sizes.push(file_size);

				free_string(file_name);
				free_string(file_type);
				free_string(file_size);
			}
		}
		static Gui_List_Column filters[] = { {"File Name", 50}, {"File Type", 25}, {"File Size", 25} };
		if (begin_list("File list", filters, 3)) {
			for (u32 i = 0; i < list_line.count; i++) {
				begin_line(&list_line[i]);
				if (right_mouse_click(list_line[i])) {
					print("Was right mouse click. index", i);
				}
				if (left_mouse_click(list_line[i])) {
					print("Was left mouse click. index", i);
				}
				if (selected(list_line[i])) {
					//print("Selected", i);
				}
				if (enter_key_click(list_line[i])) {
					print("Enter key click index", i);
				}
				begin_column("File Name");
				add_text(file_names[i].c_str(), RECT_LEFT_ALIGNMENT);
				end_column();

				begin_column("File Type");
				add_text(file_types[i].c_str(), RECT_LEFT_ALIGNMENT);
				end_column();

				begin_column("File Size");
				add_text(file_sizes[i].c_str(), RECT_LEFT_ALIGNMENT);
				end_column();

				end_line();
			}
			end_list();
		}

		//static bool init_list = false;
		//static Array<Gui_List_Line_State> list_line_states;
		//static Array<String> file_names2;
		//static Array<String> file_types2;
		//if (!init_list) {
		//	init_list = true;
		//	list_line_states.reserve(20);
		static bool node_list_init = false;
		static Directory_Entry dir_tree;
		static Directory_Entry dota_dir;
		static Directory_Entry cs_dir;
		static Directory_Entry call_of_duty_dir;
		static Directory_Entry game_dir;
		static Directory_Entry doom_3_dir;
		static Directory_Entry imgui_dir;
		static Directory_Entry source_dir;
		static Directory_Entry hades_dir;
		static Directory_Entry dev_dir;

		if (!node_list_init) {
			node_list_init = true;

			dota_dir.name = "dota 2";
			dota_dir.files.push("dota_file_1");
			dota_dir.files.push("dota_file_2");
			dota_dir.files.push("dota_file_3");

			cs_dir.name = "cs 2";
			cs_dir.files.push("cs_file_1");
			cs_dir.files.push("cs_file_2");
			cs_dir.files.push("cs_file_3");
			cs_dir.files.push("cs_file_5");

			call_of_duty_dir.name = "call_of_duty 2";
			call_of_duty_dir.files.push("call_of_duty_file_1");
			call_of_duty_dir.files.push("call_of_duty_file_12");
			call_of_duty_dir.files.push("call_of_duty_file_13");
			call_of_duty_dir.files.push("call_of_duty_file_14");

			game_dir.name = "games";
			game_dir.files.push("game_file_1");
			game_dir.files.push("game_file_2");
			game_dir.files.push("game_file_3");
			game_dir.files.push("game_file_4");
			game_dir.dirs.push(&dota_dir);
			game_dir.dirs.push(&cs_dir);
			game_dir.dirs.push(&call_of_duty_dir);

			doom_3_dir.name = "Doom 3 BFG";
			doom_3_dir.files.push("doom_source_code_1");
			doom_3_dir.files.push("doom_source_code_2");
			doom_3_dir.files.push("doom_source_code_3");
			doom_3_dir.files.push("doom_source_code_4");
			doom_3_dir.files.push("doom_source_code_5");
			doom_3_dir.files.push("doom_source_code_6");
			doom_3_dir.files.push("doom_source_code_7");
			doom_3_dir.files.push("doom_source_code_8");
			doom_3_dir.files.push("doom_source_code_9");

			imgui_dir.name = "imgui";
			imgui_dir.files.push("imgui_source_code_1");
			imgui_dir.files.push("imgui_source_code_2");
			imgui_dir.files.push("imgui_source_code_3");
			imgui_dir.files.push("imgui_source_code_4");

			source_dir.name = "source code";
			source_dir.dirs.push(&doom_3_dir);
			source_dir.dirs.push(&imgui_dir);

			hades_dir.name = "hades engine";
			hades_dir.files.push("hades_source_code 1");
			hades_dir.files.push("hades_source_code 2");
			hades_dir.files.push("hades_source_code 3");
			hades_dir.files.push("hades_source_code 4");

			dev_dir.name = "dev";
			dev_dir.files.push("dev_file_11");
			dev_dir.files.push("dev_file_12");
			dev_dir.files.push("dev_file_13");
			dev_dir.files.push("dev_file_14");
			dev_dir.files.push("dev_file_15");
			dev_dir.files.push("dev_file_16");
			dev_dir.files.push("dev_file_17");
			dev_dir.files.push("dev_file_18");
			dev_dir.files.push("dev_file_19");
			dev_dir.files.push("dev_file_20");
			dev_dir.dirs.push(&source_dir);
			dev_dir.dirs.push(&hades_dir);

			dir_tree.name = "D";
			dir_tree.dirs.push(&dev_dir);
			dir_tree.dirs.push(&game_dir);
		}
		
		display_directory_tree(&dir_tree);

		//if (begin_tree("Test tree")) {
		//	if (begin_tree_node("Node 1")) {
		//		if (begin_tree_node("Node 11")) {
		//			if (begin_tree_node("Node 111", GUI_TREE_NODE_FINAL)) {
		//				end_tree_node();
		//			}
		//			if (begin_tree_node("Node 112", GUI_TREE_NODE_FINAL)) {
		//				end_tree_node();
		//			}
		//			if (begin_tree_node("Node 113", GUI_TREE_NODE_FINAL)) {
		//				end_tree_node();
		//			}
		//			if (begin_tree_node("Node 114", GUI_TREE_NODE_FINAL)) {
		//				end_tree_node();
		//			}
		//			if (begin_tree_node("Node 115", GUI_TREE_NODE_FINAL)) {
		//				end_tree_node();
		//			}
		//			end_tree_node();
		//		}
		//		if (begin_tree_node("Node 12")) {
		//			if (begin_tree_node("Node 121")) {
		//				end_tree_node();
		//			}
		//			if (begin_tree_node("Node 122")) {
		//				end_tree_node();
		//			}
		//			if (begin_tree_node("Node 123")) {
		//				end_tree_node();
		//			}
		//			if (begin_tree_node("Node 124")) {
		//				end_tree_node();
		//			}
		//			if (begin_tree_node("Node 125")) {
		//				end_tree_node();
		//			}
		//			end_tree_node();
		//		}
		//		if (begin_tree_node("Node 13")) {
		//			end_tree_node();
		//		}
		//		if (begin_tree_node("Node 14")) {
		//			end_tree_node();
		//		}
		//		if (begin_tree_node("Node 15")) {
		//			end_tree_node();
		//		}
		//		end_tree_node();
		//	}
		//	if (begin_tree_node("Node 2")) {
		//		end_tree_node();
		//	}
		//	if (begin_tree_node("Node 3")) {
		//		end_tree_node();
		//	}
		//	if (begin_tree_node("Node 4")) {
		//		end_tree_node();
		//	}
		//	if (begin_tree_node("Node 5")) {
		//		end_tree_node();
		//	}
		//	end_tree();
		//}

		//static bool init_list = false;
		//static Array<Gui_List_Line_State> list_line_states;
		//static Array<String> file_names2;
		//static Array<String> file_types2;
		//if (!init_list) {
		//	init_list = true;
		//	list_line_states.reserve(20);

		//	for (u32 i = 0; i < 20; i++) {
		//		list_line_states[i] = 0;
		//	}

		//	for (u32 i = 0; i < 10; i++) {
		//		char *file_name = format("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAArect{}.cpp", i);
		//		file_names2.push(file_name);
		//		file_types2.push("cpp");

		//		free_string(file_name);
		//	}
		//	for (u32 i = 10; i < 20; i++) {
		//		char *file_name = format("draw_rect{}.hlsl", i);

		//		file_names2.push(file_name);
		//		file_types2.push("hlsl");

		//		free_string(file_name);
		//	}
		//}

		//Gui_List_Theme list_theme;
		//list_theme.column_filter = true;

		//set_theme(&list_theme);
		//static Gui_List_Column file_filter[] = { {"File name", 50}, {"File Type", 50} };
		//if (begin_list("Cpp file list", file_filter, 2)) {
		//	for (u32 i = 0; i < 20; i++) {
		//		begin_line(&list_line_states[i]);

		//		if (right_mouse_click(list_line_states[i])) {
		//			print("Was right mouse click. index", i);
		//		}

		//		if (left_mouse_click(list_line_states[i])) {
		//			print("Was left mouse click. index", i);
		//		}

		//		if (selected(list_line_states[i])) {
		//			//print("Selected", i);
		//		}

		//		begin_column("File name");
		//		add_text(file_names2[i].c_str(), RECT_LEFT_ALIGNMENT);
		//		end_column();

		//		begin_column("File Type");
		//		add_text(file_types2[i].c_str(), RECT_LEFT_ALIGNMENT);
		//		end_column();

		//		end_line();
		//	}
		//	end_list();
		//}
		//reset_list_theme();
		end_window();
	}
}

void draw_test_main_window()
{
	if (begin_window("Main test window")) {
		//begin_list("World Entities");
		//for (int i = 0; i < 10; i++) {
		//	item_list("Entity");
		//}
		//end_list();
		static bool state;
		radio_button("Turn on shadows", &state);
		static u32 index = 0;
		Array<String> array;
		array.push("String 1");
		array.push("String 2");
		array.push("String 3");
		list_box(&array, &index);
		static Vector3 position1 = Vector3::zero;
		static Vector3 position2 = Vector3::zero;
		static Vector3 position3 = Vector3::zero;
		//set_theme(&theme);
		//edit_field("Position", &position1);
		//reset_window_theme();
		static float value1;
		static float value2;
		static float value3;
		static float value4;
		static float value5;
		static float value6;
		edit_field("Value1", &value1);
		edit_field("Value2", &value2);
		edit_field("Value3", &value3);
		if (edit_field("Position", &position2)) {
			print("Update position1");
		}
		if (edit_field("Position", &position3)) {
			print("Update position2");
		}
		edit_field("Value4", &value4);
		edit_field("Value5", &value5);
		edit_field("Value6", &value6);

		if (begin_child("Temp child window")) {
			button("Button1");
			button("Button2");
			button("Button3");
			button("Button4");
			button("Button5");
			button("Button6");
			button("Button7");
			button("Button8");
			button("Button9");
			button("Button10");
			end_child();
		}
		if (begin_child("Next widnow")) {
			button("Button1");
			button("Button2");
			button("Button3");
			button("Button4");
			button("Button5");
			button("Button6");
			button("Button7");
			button("Button8");
			button("Button9");
			button("Button10");
			end_child();
		}
		button("Button2345");
		end_window();
	}
}

void test_gui_context_layout()
{
	if (begin_window("Layout window")) {
		//same_line();
		button("Button 11111111111111111111111111");
		button("Button 2");
		button("Button 3");
		button("Button 4");
		button("Button 5");
		same_line();
		button("Button 11");
		button("Button 22");
		button("Button 33");
		button("Button 44");
		button("Button 55");
		same_line();
		button("Button 41");
		button("Button 42");
		button("Button 43");
		button("Button 44");
		button("Button 45");
		next_line();
		button("Button 51");
		button("Button 52");
		button("Button 53");
		button("Button 54");
		button("Button 55");
		same_line();
		button("Button 101");
		button("Button 102");
		button("Button 103");
		button("Button 104");
		button("Button 105");
		button("Button 106");
		end_window();
	}
}

void test_window()
{
	static Image dir_image;
	static Image file_image;
	static bool init_local = false;
	if (!init_local) {
		init_local = true;
		dir_image.init_from_file("folder.png", "editor");
		file_image.init_from_file("google-docs.png", "editor");
	}

	Array<String> temp;
	temp.push("String 1");
	temp.push("String 2");
	temp.push("String 3");
	temp.push("String 4");
	temp.push("String 5");
	temp.push("String 6");
	static u32 index = 0;
	if (begin_window("Window 23451")) {
		list_box(&temp, &index);
		//button("ButtonW11");
		//button("ButtonW12");
		//button("ButtonW13");
		//button("ButtonW14");

		if (begin_tree("Layout  tree")) {
			if (begin_tree_node("Node 1")) {
				if (begin_tree_node("Node 11", GUI_TREE_NODE_NOT_DISPLAY_NAME)) {
					image(&dir_image.texture, 16, 16);
					
					text("Text 1");
					text("Text 2");
					text("Text 3");

					Gui_Text_Button_Theme theme;
					theme.rect.set_size(40, 16);
					theme.color = Color(32, 32, 32);
					set_theme(&theme);
					button("B");
					reset_button_theme();

					end_tree_node();
				}
				end_tree_node();
			}
			if (begin_tree_node("Node 2")) {
				if (begin_tree_node("Node 22", GUI_TREE_NODE_NOT_DISPLAY_NAME)) {
					image(&file_image.texture, 18, 18);
					text("Color.png");
					end_tree_node();
				}
				if (begin_tree_node("Node 23", GUI_TREE_NODE_NOT_DISPLAY_NAME | GUI_TREE_NODE_FINAL)) {
					image(&file_image.texture, 18, 18);
					text("Code.png");
					end_tree_node();
				}
				if (begin_tree_node("Node 43", GUI_TREE_NODE_FINAL)) {
					end_tree_node();
				}
				if (begin_tree_node("Node 53")) {
					end_tree_node();
				}
				end_tree_node();
			}
			end_tree();
		}

		end_window();
	}
	//if (begin_window("Window 2")) {
	//	button("ButtonW21");
	//	button("ButtonW22");
	//	button("ButtonW23");
	//	button("ButtonW24");
	//	end_window();
	//}
	//if (begin_window("Window 3")) {
	//	button("ButtonW31");
	//	button("ButtonW32");
	//	button("ButtonW33");
	//	button("ButtonW34");
	//	end_window();
	//}
}

void test_window_auto_size()
{
	if (begin_window("Check window")) {
		if (button("Open menu")) {
			open_menu("Test menu");
		}
		if (button("Close menu")) {
			close_menu("Test menu");
		}
		if (button("Open menu2")) {
			open_menu("Test menu2");
		}
		if (button("Close menu2")) {
			close_menu("Test menu2");
		}
		end_window();
	}
	static bool init = false;
	static Image up;
	if (!init) {
		init = true;
		up.init_from_file("icons8-sun-22.png", "editor");
	}
	if (gui::begin_menu("Test menu")) {
		gui::menu_item(&up, "Add entity");
		gui::menu_item("Delete entity");
		gui::menu_item("Copy entity");
		gui::segment();
		gui::menu_item("Move");
		gui::menu_item("Animate");
		gui::menu_item("Add animation");
		gui::segment();
		gui::menu_item("Select");
		gui::menu_item("Convert to");
		gui::menu_item("Set origin");
		gui::end_menu();
	}

	set_next_window_pos(500, 100);
	if (gui::begin_menu("Test menu2")) {
		gui::menu_item(&up, "Light");
		gui::menu_item("Shape");
		gui::menu_item("Box");
		gui::segment();
		gui::menu_item("Move");
		gui::menu_item("Animate");
		gui::menu_item("Add animation");
		gui::segment();
		gui::menu_item("Ground");
		gui::menu_item("Go");
		gui::menu_item("Set origin");
		gui::end_menu();
	}
}

void draw_test_gui()
{
	begin_frame();
	test_window_auto_size();
	test_window();
	//test_gui_context_layout();
	//draw_test_tab_window();
	draw_test_list_window();
	//draw_test_main_window();

	gui::end_frame();
}