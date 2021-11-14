#ifndef SYS_ENGINE
#define SYS_ENGINE

enum Engine_Mode {
	GAME_MODE,
	EDITING_MODE
};

struct Camera;

struct Engine_Info {
	Engine_Mode state;

	Camera *get_camera();
};
#endif