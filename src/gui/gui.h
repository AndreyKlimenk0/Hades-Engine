#ifndef HADES_GUI_H
#define HADES_GUI_H

#include "../render/render_system.h"

namespace gui {
	bool button(const char *text);

	void init_gui(Render_2D *render_2d, Win32_Info *win32_info, Font *font);
	void shutdown();
	void draw_test_gui();
}
#endif