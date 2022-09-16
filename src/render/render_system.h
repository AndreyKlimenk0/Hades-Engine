#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <stdlib.h>
#include <DirectXMath.h>

#include "directx.h"
#include "shader.h"
#include "font.h"
#include "../game/world.h"
#include "../libs/math/matrix.h"
#include "../libs/math/common.h"
#include "../win32/win_types.h"
#include "../libs/os/camera.h"
#include "../libs/ds/hash_table.h"
#include "../libs/ds/array.h"
#include "../libs/ds/linked_list.h"


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


typedef ID3D11Buffer Gpu_Buffer;

struct Primitive_2D {
	//Vars is set by Render_2D
	u32 vertex_offset = 0;
	u32 index_offset = 0;

	Array<Vertex_X2UV> vertices;
	Array<u32> indices;

	void add_point(const Vector2 &point, const Vector2 &uv = Vector2(0.0f, 0.0f)) { vertices.push(Vertex_X2UV(point, uv)); }
	void add_rounded_points(float x, float y, float width, float height, Rect_Side rect_side, u32 rounding);
	void make_triangle_polygon();
	void make_outline_triangle_polygons();
};

struct Render_Primitive_2D {
	Primitive_2D *primitive = NULL;
	Texture *texture = NULL;
	Vector2 position;
	Rect_s32 clip_rect;
	Color color;
};

typedef Array<Render_Primitive_2D> Render_Primitive_List;


const u32 ROUND_TOP_LEFT_RECT = 0x1;
const u32 ROUND_TOP_RIGHT_RECT = 0x2;
const u32 ROUND_BOTTOM_LEFT_RECT = 0x4;
const u32 ROUND_BOTTOM_RIGHT_RECT = 0x8;
const u32 ROUND_TOP_RECT = ROUND_TOP_LEFT_RECT | ROUND_TOP_RIGHT_RECT;
const u32 ROUND_BOTTOM_RECT = ROUND_BOTTOM_LEFT_RECT | ROUND_BOTTOM_RIGHT_RECT;
const u32 ROUND_RECT = ROUND_TOP_RECT | ROUND_BOTTOM_RECT;

struct Render_2D {
	~Render_2D();

	ID3D11RasterizerState * rasterization = NULL;
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

	Array<Render_Primitive_List> draw_list;

	void new_render_primitive_list();
	void add_render_primitive(Render_Primitive_2D *render_primitive);
	
	Array<Rect_s32> clip_rects;
	Array<Primitive_2D *> primitives;
	Array<Render_Primitive_2D> render_primitives;
	Hash_Table<String, Primitive_2D *> lookup_table;

	void init();
	void init_font_rendering();
	void init_font_atlas(Font *font, Hash_Table<char, Rect_f32> *font_uvs);
	void add_primitive(Primitive_2D *primitive);
	
	void new_frame(); // @Clean up change name 
	void render_frame(); // @Clean up change name 

	//@Note this method must be deleted	
	void push_clip_rect(Rect_s32 *rect);
	void pop_clip_rect();
	void get_clip_rect(Rect_s32 *rect);

	void draw_outlines(int x, int y, int width, int height, const Color &color, float outline_width = 1.0f, u32 rounding = 0, u32 flags = ROUND_RECT);

	void draw_text(Rect_s32 *rect, const char *text);
	void draw_text(int x, int y, const char *text);
	
	template <typename T>
	void draw_rect(Rect<T> *rect, const Color &color, u32 rounding = 0, u32 flags = ROUND_RECT);
	template <typename T>
	void draw_rect(T x, T y, T width, T height, const Color &color, u32 rounding = 0, u32 flags = ROUND_RECT);
	
	void draw_rect(float x, float y, float width, float height, const Color &color, u32 rounding = 0, u32 flags = ROUND_RECT);
	void draw_texture(int x, int y, int width, int height, Texture *texture);
};

inline s32 get_right_size(s32 max_size, s32 min_size)
{
	return math::abs(max_size - math::abs(max_size - min_size));
}

inline void Render_2D::push_clip_rect(Rect_s32 *rect)
{
	//if (clip_rects.count > 0) {
	//	Rect_s32 last = clip_rects.first_item();
	//	//print(&last);
	//	//if (((rect->x > last.right()) && (rect->x > last.x)) || ((rect->y > last.bottom()) && (rect->y > last.y))) {
	//	//	return;
	//	//}
	//	Rect_s32 new_rect;
	//	new_rect.x = rect->x < last.x ? last.x : rect->x;
	//	new_rect.y = rect->y < last.y ? last.y : rect->y;
	//	new_rect.width = rect->right() > last.right() ? last.width : rect->width;
	//	new_rect.height = rect->bottom() > last.bottom() ? last.height : rect->height;
	//	//new_rect.height = rect->bottom() > last.bottom() ? get_right_size(rect->bottom(), last.bottom()) : rect->height;
	//	clip_rects.push(new_rect);
	//	return;
	//}
	clip_rects.push(*rect);
}

inline void Render_2D::pop_clip_rect()
{
	if (clip_rects.count > 0) {
		clip_rects.pop();
	}
}

inline void Render_2D::get_clip_rect(Rect_s32 *rect)
{
	if (clip_rects.count > 0) {
		*rect = clip_rects.last_item();
	} else {
		rect->x = 0;
		rect->y = 0;
		rect->width = win32.window_width;
		rect->height = win32.window_height;
	}
}

template <typename T>
inline void Render_2D::draw_rect(Rect<T> *rect, const Color &color, u32 rounding, u32 flags)
{
	draw_rect(rect->x, rect->y, rect->width, rect->height, color, rounding, flags);
}

template <typename T>
inline void Render_2D::draw_rect(T x, T y, T width, T height, const Color &color, u32 rounding, u32 flags)
{
	float _x = static_cast<float>(x);
	float _y = static_cast<float>(y);
	float _width = static_cast<float>(width);
	float _height = static_cast<float>(height);

	draw_rect(_x, _y, _width, _height, color, rounding, flags);
}

inline void Render_2D::draw_text(Rect_s32 *rect, const char *text)
{
	draw_text((int)rect->x, (int)rect->y, text);
}

inline void Render_2D::new_frame()
{
	clip_rects.clear();
	Render_Primitive_List *list = NULL;
	For(draw_list, list) {
		list->count;
	}
	draw_list.count = 0;
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

	ID3D11ShaderResourceView *shader_resource = NULL;

	void init(View_Info *_view_info);
	void resize();
	void shutdown();
	
	void render_frame();
	void draw_world_entities(World *world);

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
	assert((buffer_size % 16) == 0);
	return make_gpu_buffer(buffer_size, 1, data, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE);
}

void update_constant_buffer(Gpu_Buffer *buffer, void *data, u32 data_size);
#endif