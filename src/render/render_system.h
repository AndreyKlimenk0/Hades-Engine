#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <stdlib.h>
#include <DirectXMath.h>

#include "directx.h"
#include "effect.h"
#include "../game/world.h"
#include "../libs/math/matrix.h"
#include "../win32/win_types.h"
#include "../libs/os/camera.h"


struct View_Info;


struct Render_System {
	~Render_System();

	World  *current_render_world = NULL;
	
	View_Info *view_info = NULL;
	Free_Camera *free_camera = NULL;

	Matrix4 view_matrix;

	void init(View_Info *_view_info);
	void resize();
	void shutdown();
	
	void render_frame();
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
	Matrix4 orthogonal_matrix;

	void update_projection_matries();
};

inline void View_Info::update_projection_matries()
{
	perspective_matrix = XMMatrixPerspectiveFovLH(fov_y_ratio, window_ratio, near_plane, far_plane);
	orthogonal_matrix = XMMatrixOrthographicLH(window_width, window_height, near_plane, far_plane);
}

View_Info *make_view_info(float near_plane, float far_plane);


void make_outlining(Render_Entity *render_entity);
void free_outlining(Render_Entity *render_entity);
void draw_texture_on_screen(s32 x, s32 y, Texture *texture, float _width = 0.0f, float _height = 0.0f);
#endif