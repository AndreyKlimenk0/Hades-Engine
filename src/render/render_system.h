#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <stdlib.h>
#include <DirectXMath.h>

#include "font.h"
#include "render_api.h"
#include "vertex.h"
#include "../game/world.h"
#include "../libs/math/matrix.h"
#include "../libs/math/common.h"
#include "../libs/ds/array.h"
#include "../libs/ds/hash_table.h"
#include "../libs/ds/linked_list.h"
#include "../win32/win_types.h"


struct Engine;
struct Render_System;
struct Render_2D;
struct Render_Font;

struct Primitive_2D {
	//Vars is set by Render_2D
	u32 vertex_offset = 0;
	u32 index_offset = 0;

	Array<Vertex_X2UV> vertices;
	Array<u32> indices;

	void add_point(const Vector2 &point, const Vector2 &uv = Vector2(0.0f, 0.0f)) { vertices.push(Vertex_X2UV(point, uv)); }
	void add_rounded_points(float x, float y, float width, float height, Rect_Side rect_side, u32 rounding);
	void add_rounded_points(float x, float y, float width, float height, Rect_Side rect_side, float x_rounding, float y_rounding);

	void make_triangle_polygon();
	void make_outline_triangle_polygons();
};

struct Render_Primitive_2D {
	Primitive_2D *primitive = NULL;
	Texture2D *texture = NULL;
	Color color;
	Rect_s32 clip_rect;
	Matrix4 transform_matrix;
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
	//@Note: default construcor should be deleted. 
	Render_Primitive_List() {}
	Render_Primitive_List(Render_2D *render, Font *font, Render_Font *render_font);
	
	Render_2D *render_2d = NULL;
	Font *font = NULL;
	Render_Font *render_font = NULL;
	
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

	void add_texture(int x, int y, int width, int height, Texture2D *resource);

	void add_line(Point_s32 *first_point, Point_s32 *second_point, const Color &color, float thicknesss = 0.5f);

	Primitive_2D *make_or_find_primitive(Matrix4 &transform_matx, Texture2D *texture, const Color &color, String &primitve_hash);
};

struct Render_Font {
	Texture2D font_atlas;
	Hash_Table<u8, Primitive_2D *> lookup_table;

	void init(Render_2D *render_2d, Font *font);
	void make_font_atlas(Font *font, Hash_Table<char, Rect_f32> *font_uvs);
};

struct Render_2D {
	~Render_2D();

	Texture2D default_texture;
	
	Gpu_Buffer constant_buffer;
	Gpu_Buffer vertex_buffer;
	Gpu_Buffer index_buffer;

	Rasterizer_State rasterizer_state;
	Blend_State blend_state;
	Depth_Stencil_State depth_stencil_state;

	Shader *render_2d = NULL;
	
	Gpu_Device *gpu_device = NULL;
	Render_Pipeline *render_pipeline = NULL;
	Render_System *render_system = NULL;

	u32 total_vertex_count = 0;
	u32 total_index_count = 0;

	Array<Primitive_2D *> primitives;
	Array<Render_Primitive_List *> draw_list;
	Hash_Table<String, Primitive_2D *> lookup_table;
	Hash_Table<String, Render_Font *> render_fonts;

	void init(Engine *engine);
	void add_primitive(Primitive_2D *primitive);
	void add_render_primitive_list(Render_Primitive_List *render_primitive_list);
	Render_Font *get_render_font(Font *font);
	
	void new_frame();
	void render_frame(); // @Clean up change name 
};

struct View {
	float ratio;
	float fov;
	float near_plane;
	float far_plane;

	Matrix4 perspective_matrix;
	Matrix4 orthogonal_matrix;

	void update_projection_matries(u32 width, u32 height, float _near_plane, float _far_plane);
};

struct Projection_Plane {
	u32 width;
	u32 height;
	float ratio;
	float fov_y_ratio;
	float near_plane;
	float far_plane;
};

struct Projection_Matries {
	Matrix4 perspective_matrix;
	Matrix4 orthogonal_matrix;

	void update();
};

struct Render_Pipeline_States {
	Rasterizer_State default_rasterizer_state;
	Depth_Stencil_State default_depth_stencil_state;
	Depth_Stencil_State disabled_depth_test;
	Blend_State default_blend_state;
	Sampler_State default_sampler_state;

	void init(Gpu_Device *gpu_device);
};

struct Render_System {
	static u32 screen_width;
	static u32 screen_height;

	//@Note: Why I need to have this var here ?
	View view;
	//@Note: Why I need to have this var here ?
	Render_2D render_2d;

	Texture2D back_buffer;
	Texture2D depth_back_buffer;

	Swap_Chain swap_chain;
	Gpu_Device gpu_device;
	Render_Pipeline render_pipeline;
	Render_Pipeline_States render_pipeline_states;
	
	Hash_Table<String, Shader *> shader_table;

	void init(Engine *engine);
	void init_render_targets(u32 window_width, u32 window_height);
	
	void resize(u32 window_width, u32 window_height);

	void new_frame();
	void end_frame();

	Shader *get_shader(const char *name);
};
#endif
