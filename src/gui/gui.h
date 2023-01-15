#ifndef HADES_GUI_H
#define HADES_GUI_H

#include "../libs/ds/array.h"
#include "../render/render_system.h"

typedef u32 Window_Style;
const Window_Style NO_WINDOW_STYLE = 0x0;
const Window_Style WINDOW_WITH_HEADER = 0x1;
const Window_Style WINDOW_WITH_OUTLINES = 0x2;
const Window_Style WINDOW_WITH_SCROLL_BAR = 0x4;
const Window_Style WINDOW_STYLE_DEFAULT = WINDOW_WITH_HEADER | WINDOW_WITH_OUTLINES | WINDOW_WITH_SCROLL_BAR;

namespace gui {
	void init_gui(Render_2D *render_2d, Win32_Info *win32_info, Font *font, Gpu_Device *gpu_device);
	void shutdown();
	void draw_test_gui();

	void begin_frame();
	void end_frame();

	bool begin_window(const char *name, Window_Style window_style = WINDOW_STYLE_DEFAULT);
	void end_window();
	void set_next_window_size(s32 width, s32 height);
	void set_next_window_pos(s32 x, s32 y);
	void same_line();
	void next_line();

	bool button(const char *text);
	void radio_button(const char *name, bool *state);

	bool add_tab(const char *tab_name);

	void text(const char *some_text);

	void list_box(Array<String> *array, u32 *item_index);
	
	void image(Texture *texture, s32 width = -1, s32 height = -1);


	void edit_field(const char *name, int *value);
	void edit_field(const char *name, float *value);
	void edit_field(const char *name, String *value);
}
#endif