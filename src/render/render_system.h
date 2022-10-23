#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <stdlib.h>
#include <DirectXMath.h>

#include "font.h"
#include "../game/world.h"
#include "../libs/math/matrix.h"
#include "../libs/math/common.h"
#include "../win32/win_types.h"
#include "../libs/os/camera.h"
#include "../libs/ds/hash_table.h"
#include "../libs/ds/array.h"
#include "../libs/ds/linked_list.h"
#include "render_api.h"


struct Render_System;
struct Render_2D;


struct Primitive_2D {
	//Vars is set by Render_2D
	u32 vertex_offset = 0;
	u32 index_offset = 0;

	Array<Vertex_X2UV> vertices;
	Array<u32> indices;

	void add_point(const Vector2 &point, const Vector2 &uv = Vector2(0.0f, 0.0f)) { vertices.push(Vertex_X2UV(point, uv)); }
	void add_rounded_points(float x, float y, float width, float height, Rect_Side rect_side, float rounding);
	void add_rounded_points(float x, float y, float width, float height, Rect_Side rect_side, float x_rounding, float y_rounding);

	void make_triangle_polygon();
	void make_outline_triangle_polygons();
};

struct Render_Primitive_2D {
	Primitive_2D *primitive = NULL;
	Texture *gpu_resource = NULL;
	Vector2 position;
	Rect_s32 clip_rect;
	Color color;
};

const u32 ROUND_TOP_LEFT_RECT = 0x1;
const u32 ROUND_TOP_RIGHT_RECT = 0x2;
const u32 ROUND_BOTTOM_LEFT_RECT = 0x4;
const u32 ROUND_BOTTOM_RIGHT_RECT = 0x8;
const u32 ROUND_RIGHT_RECT = ROUND_TOP_RIGHT_RECT | ROUND_BOTTOM_RIGHT_RECT;
const u32 ROUND_LEFT_RECT = ROUND_TOP_LEFT_RECT | ROUND_BOTTOM_LEFT_RECT;
const u32 ROUND_TOP_RECT = ROUND_TOP_LEFT_RECT | ROUND_TOP_RIGHT_RECT;
const u32 ROUND_BOTTOM_RECT = ROUND_BOTTOM_LEFT_RECT | ROUND_BOTTOM_RIGHT_RECT;
const u32 ROUND_RECT = ROUND_TOP_RECT | ROUND_BOTTOM_RECT;

struct Render_Primitive_List {	
	Render_Primitive_List() {}
	Render_Primitive_List(Render_2D *render_2d);
	
	Render_2D *render_2d = NULL;
	
	Array<Rect_s32> clip_rects;
	Array<Render_Primitive_2D> render_primitives;

	void push_clip_rect(Rect_s32 *rect);
	void pop_clip_rect();
	void get_clip_rect(Rect_s32 *rect);

	void add_outlines(int x, int y, int width, int height, const Color &color, float outline_width = 1.0f, u32 rounding = 0, u32 flags = ROUND_RECT);

	void add_text(Rect_s32 *rect, const char *text);
	void add_text(int x, int y, const char *text);
	
	void add_rect(Rect_s32 *rect, const Color &color, u32 rounding = 0, u32 flags = ROUND_RECT);
	void add_rect(s32 x, s32 y, s32 width, s32 height, const Color &color, u32 rounding = 0, u32 flags = ROUND_RECT);
	void add_rect(float x, float y, float width, float height, const Color &color, u32 rounding = 0, u32 flags = ROUND_RECT);

	void add_texture(int x, int y, int width, int height, Texture *gpu_resource);

	Primitive_2D *make_or_find_primitive(float x, float y, Texture *texture, const Color &color, String &primitve_hash);
};

struct Render_2D {
	~Render_2D();

	Texture *font_atlas = NULL;
	Texture *default_texture = NULL;
	Texture *temp = NULL;
	
	Gpu_Buffer *constant_buffer = NULL;
	Gpu_Buffer *vertex_buffer = NULL;
	Gpu_Buffer *index_buffer = NULL;

	Gpu_Buffer *font_vertex_buffer = NULL;
	Gpu_Buffer *font_index_buffer = NULL;

	Rasterizer *rasterizer = NULL;
	Blending_Test *blending_test = NULL;
	Depth_Stencil_Test *depth_test = NULL;

	Shader *render_2d = NULL;
	
	Gpu_Device *gpu_device = NULL;
	Render_Pipeline *render_pipeline = NULL;
	Render_System *render_system = NULL;

	u32 total_vertex_count = 0;
	u32 total_index_count = 0;

	Matrix4 screen_postion;

	Array<Primitive_2D *> primitives;
	Array<Render_Primitive_List *> draw_list;
	Hash_Table<String, Primitive_2D *> lookup_table;

	void init(Render_System *_render_system, Shader *_render_2d);
	void init_font_rendering();
	void init_font_atlas(Font *font, Hash_Table<char, Rect_f32> *font_uvs);
	void add_primitive(Primitive_2D *primitive);
	void add_render_primitive_list(Render_Primitive_List *render_primitive_list);
	
	void new_frame();
	void render_frame(); // @Clean up change name 
};

struct View_Info {
	u32 width;
	u32 height;

	float ratio;
	float fov_y_ratio;
	float near_plane;
	float far_plane;

	Matrix4 perspective_matrix;
	Matrix4 orthogonal_matrix;

	void init(u32 _width, u32 _height, float _near_plane, float _far_plane);
	void update_projection_matries(u32 new_window_width, u32 new_window_height);
};

struct Render_System {
	~Render_System();

	World  *current_render_world = NULL;
	
	View_Info view_info;
	Free_Camera *free_camera = NULL;

	Texture_Sampler *sampler = NULL;

	Matrix4 view_matrix;

	Render_2D render_2d;

	ID3D11ShaderResourceView *shader_resource = NULL;

	Hash_Table<String, Shader *> shaders;

	Gpu_Device gpu_device;
	Render_Pipeline render_pipeline;

	void init(Win32_State *win32_state);
	void init_shaders();
	void resize();
	void shutdown();

	void new_frame();
	void end_frame();
	
	void render_frame();
};
#endif
