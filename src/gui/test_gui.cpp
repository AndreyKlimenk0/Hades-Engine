#include <assert.h>

#include "gui.h"
#include "test_gui.h"
#include "../sys/sys_local.h"
#include "../libs/str.h"
#include "../libs/ds/array.h"
#include "../libs/math/vector.h"

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
				char *button_name = format("First tab Button {}", i);
				button(button_name);
				free_string(button_name);
			}
			draw_first_tab = true;
		}
		if (add_tab("Second tab")) {
			for (int i = 0; i < 10; i++) {
				char *button_name = format("Second tab Button {}", i);
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
		//list_theme.column_filter = false;

		//set_theme(&list_theme);
		//static Gui_List_Column file_filter[] = { {"File name", 50}, {"File Type", 25} };
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
		edit_field("Position", &position2);
		edit_field("Position", &position3);
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
		button("Button2345");
		end_window();
	}
}


void draw_test_gui()
{
	begin_frame();

	draw_test_tab_window();
	draw_test_list_window();
	draw_test_main_window();

	gui::end_frame();
}