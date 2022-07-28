#ifndef HADES_GUI_H
#define HADES_GUI_H


namespace gui {
	bool button();
};

bool button(const char *text);

void init_gui();
void shutdown_gui();
void draw_test_gui();

#endif