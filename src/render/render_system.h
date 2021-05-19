#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <stdlib.h>
#include <DirectXMath.h>

#include "render.h"
#include "../game/world.h"
#include "../libs/math/matrix.h"
#include "../win32/win_types.h"
#include "../libs/os/camera.h"
#include "../render/directx.h"


struct View_Info;


struct Render_System {
	~Render_System();

	Render *render = NULL;
	World  *current_render_world = NULL;
	
	View_Info *view_info = NULL;
	Free_Camera *free_camera = NULL;

	Matrix4 view_matrix;

	void init(Render_API render_api, View_Info *_view_info);
	void resize();
	void shutdown();
	
	void render_frame();
	void draw_world_entities(Entity_Manager *entity_manager);

	DirectX11 *get_directx11() { return (DirectX11 *)render; }
};

extern Render_System render_sys;

struct View_Info {
	int window_width;
	int window_height;

	float window_ratio;
	float fov_y_ratio;
	float near_plane;
	float far_plane;

	Matrix4 perspective_matrix;
	void get_perspective_matrix();
};

View_Info *make_view_info(float near_plane, float far_plane);
inline void View_Info::get_perspective_matrix()
{
	perspective_matrix = XMMatrixPerspectiveFovLH(fov_y_ratio, window_ratio, near_plane, far_plane);
}
#endif