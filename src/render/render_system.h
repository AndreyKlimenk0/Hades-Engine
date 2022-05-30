#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <stdlib.h>
#include <DirectXMath.h>

#include "directx.h"
#include "shader.h"
#include "font.h"
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

	Matrix4 test_matrix;

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

	Array<Vertex_X2UV> vertices;
	Array<u32> indices;

	void add_point(const Vector2 &point, const Vector2 &uv = Vector2(0.0f, 0.0f)) { vertices.push(Vertex_X2UV(point, uv)); }
	void add_rounded_points(int x, int y, int width, int height, Rect_Side rect_side, u32 rounding);
	void make_triangle_polygon();
};

struct Render_Primitive_2D_Info {
	Primitive_2D *primitive = NULL;
	Texture *texture = NULL;
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

	ID3D11DepthStencilState *depth_test = NULL;
	ID3D11BlendState *blending_test = NULL;

	Gpu_Buffer *constant_buffer = NULL;
	Gpu_Buffer *vertex_buffer = NULL;
	Gpu_Buffer *index_buffer = NULL;

	Gpu_Buffer *font_vertex_buffer = NULL;
	Gpu_Buffer *font_index_buffer = NULL;

	Texture font_atlas;
	Texture default_texture;
	Texture *temp = NULL;

	u32 total_vertex_count = 0;
	u32 total_index_count = 0;

	Matrix4 screen_postion;

	Array<Primitive_2D *> primitives;
	Array<Render_Primitive_2D_Info> render_primitives;
	Hash_Table<String, Primitive_2D *> lookup_table;

	void init();
	void init_font_rendering();
	void init_font_atlas(Font *font, Hash_Table<char, Rect_f32> *font_uvs);
	void clear();
	void add_primitive(Primitive_2D *primitive);
	void draw_primitives();
	
	void draw_text(int x, int y, const char *text);
	void draw_rect(int x, int y, int width, int height, const Color &color, u32 rounding = 0, u32 flags = ROUND_RECT);
	void draw_texture(int x, int y, int width, int height, Texture *texture);
};


inline void Render_2D::clear()
{
	render_primitives.count = 0;
}

struct Render_System {
	~Render_System();

	World  *current_render_world = NULL;
	
	View_Info *view_info = NULL;
	Free_Camera *free_camera = NULL;

	ID3D11SamplerState *sampler = NULL;

	Matrix4 view_matrix;

	Render_2D render_2d;

	Shader_Manager shader_manager;

	void init(View_Info *_view_info);
	void resize();
	void shutdown();
	
	void render_frame();

	Render_2D *get_render_2d();
	Shader_Manager *get_shader_manager();
};

inline Render_2D *Render_System::get_render_2d()
{
	return &render_2d;
}

inline Shader_Manager *Render_System::get_shader_manager()
{
	return &shader_manager;
}

extern Render_System render_sys;

inline Render_2D *get_render_2d()
{
	return render_sys.get_render_2d();
}

void make_outlining(Render_Entity *render_entity);
void free_outlining(Render_Entity *render_entity);
void draw_texture_on_screen(s32 x, s32 y, Texture *texture, float _width = 0.0f, float _height = 0.0f);

Gpu_Buffer *make_gpu_buffer(u32 data_size, u32 data_count, void *data, D3D11_USAGE usage, u32 bind_flags, u32 cpu_access);

inline Gpu_Buffer *make_vertex_buffer(u32 vertex_size, u32 vertex_count, void *vertex_data, D3D11_USAGE usage = D3D11_USAGE_DEFAULT, u32 cpu_access = 0)
{
	return make_gpu_buffer(vertex_size, vertex_count, vertex_data, usage, D3D11_BIND_VERTEX_BUFFER, cpu_access);
}

inline Gpu_Buffer *make_index_buffer(u32 index_count, u32 *index_data, D3D11_USAGE usage = D3D11_USAGE_DEFAULT, u32 cpu_access = 0)
{
	return make_gpu_buffer(sizeof(u32), index_count, index_data, usage, D3D11_BIND_INDEX_BUFFER, cpu_access);
}

inline Gpu_Buffer *make_constant_buffer(u32 buffer_size, void *data = NULL)
{
	return make_gpu_buffer(buffer_size, 1, data, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE);
}

void update_constant_buffer(Gpu_Buffer *buffer, void *data, u32 data_size);
#endif