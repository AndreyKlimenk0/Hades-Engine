#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <stdlib.h>
#include <DirectXMath.h>

#include "directx.h"
#include "shader.h"
#include "../game/world.h"
#include "../libs/math/matrix.h"
#include "../win32/win_types.h"
#include "../libs/os/camera.h"
#include "../libs/ds/hash_table.h"
#include "../libs/ds/array.h"


struct View_Info {
	int window_width;
	int window_height;

	float window_ratio;
	float fov_y_ratio;
	float near_plane;
	float far_plane;

	Matrix4 perspective_matrix;
	Matrix4 orthogonal_matrix;

	void update_projection_matries(u32 new_window_width, u32 new_window_height);
};

inline void View_Info::update_projection_matries(u32 new_window_width, u32 new_window_height)
{
	window_width = new_window_width;
	window_height = new_window_height;
	window_ratio = (float)window_width / (float)window_height;
	perspective_matrix = XMMatrixPerspectiveFovLH(fov_y_ratio, window_ratio, near_plane, far_plane);
	orthogonal_matrix =  XMMatrixOrthographicOffCenterLH(0.0f, (float)window_width, (float)window_height, 0.0f, near_plane, far_plane);
}

View_Info *make_view_info(float near_plane, float far_plane);

enum Rect_Side;
typedef ID3D11Buffer Gpu_Buffer;

struct Primitive_2D {
	//Vars is set by Render_2D
	u32 vertex_offset = 0;
	u32 index_offset = 0;

	Array<Vector2> points;
	Array<Vertex_XC> vertices;
	Array<u32> indices;

	void add_point(const Vector2 &point) { points.push(point); }
	void add_rounded_points(int x, int y, int width, int height, Rect_Side rect_side, u32 rounding);
	void make_triangle_polygon(const Color &color);
};

struct Render_Primitive_2D_Info {
	Primitive_2D *primitive = NULL;
	Vector2 position;
	Color color;
};

const u32 ROUND_TOP_LEFT_RECT = 0x1;
const u32 ROUND_TOP_RIGHT_RECT = 0x4;
const u32 ROUND_BOTTOM_LEFT_RECT = 0x2;
const u32 ROUND_BOTTOM_RIGHT_RECT = 0x8;
const u32 ROUND_TOP_RECT = ROUND_TOP_LEFT_RECT | ROUND_TOP_RIGHT_RECT;
const u32 ROUND_BOTTOM_RECT = ROUND_BOTTOM_LEFT_RECT | ROUND_BOTTOM_RIGHT_RECT;
const u32 ROUND_RECT = ROUND_TOP_RECT | ROUND_BOTTOM_RECT;

struct Render_2D {
	~Render_2D();

	Gpu_Buffer *vertex_buffer = NULL;
	Gpu_Buffer *index_buffer = NULL;

	u32 total_vertex_count = 0;
	u32 total_index_count = 0;

	Array<Primitive_2D *> primitives;
	Array<Render_Primitive_2D_Info> render_primitives;
	Hash_Table<String, Primitive_2D *> lookup_table;

	void clear();
	void add_primitive(Primitive_2D *primitive);
	void draw_rect(int x, int y, int width, int height, const Color &color, u32 rounding = 0, u32 flags = ROUND_RECT);
	void draw_primitives();
};

inline void Render_2D::clear()
{
	//Primitive_2D *primitive = NULL;
	//For(primitives, primitive) {
	//	DELETE_PTR(primitive);
	//}
	
	//primitives.clear();

	render_primitives.count = 0;
	
	//total_vertex_count = 0;
	//total_index_count = 0;
}

struct Render_System {
	~Render_System();

	World  *current_render_world = NULL;
	
	View_Info *view_info = NULL;
	Free_Camera *free_camera = NULL;

	Matrix4 view_matrix;

	Render_2D render_2d;

	Shader_Manager shader_manager;

	void init(View_Info *_view_info);
	void resize();
	void shutdown();
	
	void render_frame();

	Shader_Manager *get_shader_manager();
};

inline Shader_Manager *Render_System::get_shader_manager()
{
	return &shader_manager;
}

extern Render_System render_sys;

void make_outlining(Render_Entity *render_entity);
void free_outlining(Render_Entity *render_entity);
void draw_texture_on_screen(s32 x, s32 y, Texture *texture, float _width = 0.0f, float _height = 0.0f);

#endif