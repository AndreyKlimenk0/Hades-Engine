#include "test.h"
#include "../libs/math/matrix.h"
#include "../libs/ds/array.h"
#include "../libs/str.h"
#include "../render/render_api.h"

typedef void (*Job_Function)(void *data);

struct Job {
	void *data = NULL;
	Job_Function *function = NULL;
};

struct Parallel_Job_List {
	Array<Job> jobs;

	void add_job(Job_Function *job_function, void *data);
	void run();
	void wait();
};


struct Parallel_Job_Manager {
	Array<Parallel_Job_List> parallel_jobs;

	void init();
};


//struct Gui_Line_State {
//	enum Data_Type {
//		LIST_ITEM_DATA_TYPE_UNKNOWN,
//		LIST_ITEM_DATA_TYPE_TEXT,
//		LIST_ITEM_DATA_TYPE_IMAGE,
//		LIST_ITEM_DATA_TYPE_IMAGE_BUTTON,
//	};
//
//	struct Item_Data {
//		Data_Type type;
//		union {
//			const char *filter_name = NULL;
//			const char *text;
//			Texture2D texture;
//		};
//		u32 alignment_type;
//	};
//	Array<Item_Data> filter_data_list;
//	struct State {
//		bool selected;
//		bool left_mouse_click;
//		bool right_mouse_click;
//	};
//
//	void add_text(u32 alignment, const char *text, const char *filter_name = NULL);
//	void add_image(u32 alignment, Texture2D *texture2d, const char *filter_name = NULL);
//	void add_image_button(u32 alignment, Texture2D *texture2d, const char *filter_name = NULL);
//};

enum String_Priority_Type {

};

void test()
{
	////const char *str1 = "test10";
	////const char *str2 = "test2";
	//const char *str1 = "test10test";
	//const char *str2 = "test11tes";
	////const char *str1 = "1235A";
	////const char *str2 = "1234";
	//int result = compare_strings_priority(str1, str2);
	//if (result > 0) {
	//	print("First string has more priority");
	//} else if (result < 0) {
	//	print("Second string has more priority");
	//} else {
	//	print("String equal");
	//}

}

void update_test()
{
}
