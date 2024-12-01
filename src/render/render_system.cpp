#include <assert.h>

#include "hlsl.h"
#include "font.h"
#include "render_system.h"
#include "render_helpers.h"

#include "../sys/sys.h"
#include "../sys/engine.h"
#include "../sys/profiling.h"
#include "../libs/utils.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/math/functions.h"
#include "../libs/math/structures.h"

#define TURN_ON_RECT_CLIPPING 1

u32 Render_System::screen_width = 0;
u32 Render_System::screen_height = 0;

inline void from_win32_screen_space(u32 screen_width, u32 screen_height, const Point_s32 &win32_point, Point_s32 &normal_point)
{
	normal_point.x = win32_point.x - (screen_width / 2);
	normal_point.y = -win32_point.y + (screen_height / 2);
}

inline void to_win32_screen_space(const Point_s32 &point, u32 screen_width, u32 screen_height, u32 *screen_x, u32 *screen_y)
{
	*screen_x = point.x + (screen_width / 2);
	*screen_y = -point.y + (screen_height / 2);
}

template< typename T >
inline Vector2 make_vector2(const Point3D<T> &first_point, const Point3D<T> &second_point)
{
	Point3D<T> result = first_point - second_point;
	return result.to_vector2();;
}

inline Vector2 quad(float t, Vector2 p0, Vector2 p1, Vector2 p2)
{
	return (float)pow((1.0f - t), 2.0f) * p0 + 2.0f * (1.0f - t) * t * p1 + (float)pow(t, 2.0f) * p2;
}

static int compare_rects_u32(const void *first_rect, const void *second_rect)
{
	assert(first_rect);
	assert(second_rect);

	const Rect_u32 *first = static_cast<const Rect_u32 *>(first_rect);
	const Rect_u32 *second = static_cast<const Rect_u32 *>(second_rect);

	return first->height > second->height;
}

inline void pack_rects_in_rect(Rect_u32 *main_rect, Array<Rect_u32 *> &rects)
{
	assert(main_rect);

	qsort(rects.items, rects.count, sizeof(rects[0]), compare_rects_u32);

	u32 x_pos = 0;
	u32 y_pos = 0;
	u32 large = 0;

	Rect_u32 *rect = NULL;
	For(rects, rect) {

		if ((rect->width + x_pos) > main_rect->width) {
			y_pos += large;
			x_pos = 0;
			large = 0;
		}

		if ((y_pos + rect->height) > main_rect->height) {
			break;
		}

		rect->x = x_pos;
		rect->y = y_pos;

		x_pos += rect->width;

		if (rect->height > large) {
			large = rect->height;
		}
	}
}

void Primitive_2D::add_rounded_points(float x, float y, float width, float height, Rect_Side rect_side, u32 rounding)
{
	add_rounded_points(x, y, width, height, rect_side, (float)rounding, (float)rounding);
}

void Primitive_2D::add_rounded_points(float x, float y, float width, float height, Rect_Side rect_side, float x_rounding, float y_rounding)
{
	Vector2 point0, point1, point2;
	if (rect_side == RECT_SIDE_LEFT_TOP) {
		point0 = Vector2(x, y + y_rounding);
		point1 = Vector2(x, y);
		point2 = Vector2(x + x_rounding, y);

	} else if (rect_side == RECT_SIDE_RIGHT_TOP) {
		point0 = Vector2(x + width - x_rounding, y);
		point1 = Vector2(x + width, y);
		point2 = Vector2(x + width, y + y_rounding);

	} else if (rect_side == RECT_SIDE_LEFT_BOTTOM) {
		point0 = Vector2(x + x_rounding, y + height);
		point1 = Vector2(x, y + height);
		point2 = Vector2(x, y + height - y_rounding);

	} else if (rect_side == RECT_SIDE_RIGHT_BOTTOM) {
		point0 = Vector2(x + width, y + height - y_rounding);
		point1 = Vector2(x + width, y + height);
		point2 = Vector2(x + width - x_rounding, y + height);
	}

	u32 points_count_in_rounding = 20;
	float point_count = 1.0f / (float)points_count_in_rounding;
	float point_position = 0.0f;

	for (u32 i = 0; i < points_count_in_rounding; i++) {
		Vector2 point = quad(point_position, point0, point1, point2);
		add_point(point);
		point_position += point_count;
	}
}

void Primitive_2D::make_triangle_polygon()
{
	for (u32 i = 2; i < vertices.count; i++) {
		indices.push(0);
		indices.push(i - 1);
		indices.push(i);
		//indices.push(i - 1);
		//indices.push(i);
		//indices.push(0);
	}
	if ((vertices.count % 2) != 0) {
		indices.push(0);
		indices.push(vertices.count - 1);
		indices.push(1);
	}
}

void Primitive_2D::make_outline_triangle_polygons()
{
	assert((vertices.count % 2) == 0);

	u32 count = vertices.count / 2;
	for (u32 i = 0; i < count; i++) {
		indices.push(i);
		indices.push(i + 1);
		indices.push(i + count);

		if (i != (count - 1)) {
			indices.push(i + count);
			indices.push(i + 1);
			indices.push(i + count + 1);
		} else {
			indices.push(count - 1);
			indices.push(0);
			indices.push(count);
		}
	}
}

Render_Primitive_List::Render_Primitive_List(Render_2D *render_2d, Font *font, Render_Font *render_font) : render_2d(render_2d), font(font), render_font(render_font)
{
	Rect_s32 rect;
	rect.set_size(Render_System::screen_width, Render_System::screen_height);
	clip_rects.push(rect);
}

void Render_Primitive_List::push_clip_rect(Rect_s32 *rect)
{
#if TURN_ON_RECT_CLIPPING
	clip_rects.push(*rect);
#endif
}

void Render_Primitive_List::pop_clip_rect()
{
#if TURN_ON_RECT_CLIPPING
	if (clip_rects.count > 0) {
		clip_rects.pop();
	}
#endif
}

void Render_Primitive_List::get_clip_rect(Rect_s32 *rect)
{
	*rect = clip_rects.last();
}

void Render_Primitive_List::add_outlines(int x, int y, int width, int height, const Color &color, float outline_width, u32 rounding, u32 flags)
{
	//String hash = String((int)width) + String((int)height) + String((int)outline_width) + String("outline");

	//Vector2 position = { (float)x, (float)y };
	//Matrix4 transform_matrix = make_translation_matrix(&position);

	//Primitive_2D *primitive = make_or_find_primitive(transform_matrix, &render_2d->default_texture, color, hash);
	//if (!primitive) {
	//	return;
	//}
	//bool is_rounded = rounding > 0;
	//((flags & ROUND_TOP_LEFT_RECT) && is_rounded) ? primitive->add_rounded_points(-outline_width, -outline_width, (float)width + outline_width * 2.0f, (float)height + outline_width * 2.0f, RECT_SIDE_LEFT_TOP, rounding) : primitive->add_point(Vector2(-outline_width, -outline_width));
	//((flags & ROUND_TOP_RIGHT_RECT) && is_rounded) ? primitive->add_rounded_points(-outline_width, -outline_width, (float)width + outline_width * 2.0f, (float)height + outline_width * 2.0f, RECT_SIDE_RIGHT_TOP, rounding) : primitive->add_point(Vector2((float)width + outline_width, -outline_width));
	//((flags & ROUND_BOTTOM_RIGHT_RECT) && is_rounded) ? primitive->add_rounded_points(-outline_width, -outline_width, (float)width + outline_width * 2.0f, (float)height + outline_width * 2.0f, RECT_SIDE_RIGHT_BOTTOM, rounding) : primitive->add_point(Vector2((float)width + outline_width, (float)height + outline_width));
	//((flags & ROUND_BOTTOM_LEFT_RECT) && is_rounded) ? primitive->add_rounded_points(-outline_width, -outline_width, (float)width + outline_width * 2.0f, (float)height + outline_width * 2.0f, RECT_SIDE_LEFT_BOTTOM, rounding) : primitive->add_point(Vector2(-outline_width, (float)height + outline_width));

	//((flags & ROUND_TOP_LEFT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, (float)width, (float)height, RECT_SIDE_LEFT_TOP, rounding) : primitive->add_point(Vector2(0.0f, 0.0f));
	//((flags & ROUND_TOP_RIGHT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, (float)width, (float)height, RECT_SIDE_RIGHT_TOP, rounding) : primitive->add_point(Vector2((float)width, 0.0f));
	//((flags & ROUND_BOTTOM_RIGHT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, (float)width, (float)height, RECT_SIDE_RIGHT_BOTTOM, rounding) : primitive->add_point(Vector2((float)width, (float)height));
	//((flags & ROUND_BOTTOM_LEFT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, (float)width, (float)height, RECT_SIDE_LEFT_BOTTOM, rounding) : primitive->add_point(Vector2(0.0f, (float)height));

	//primitive->make_outline_triangle_polygons();
	//render_2d->add_primitive(primitive);
}

void Render_Primitive_List::add_text(Rect_s32 *rect, const char *text, Text_Alignment text_alignmnet)
{
	add_text((int)rect->x, (int)rect->y, text, text_alignmnet);
}

void Render_Primitive_List::add_text(int x, int y, const char *text, Text_Alignment text_alignment)
{
	//assert(text);

	//u32 len = (u32)strlen(text);
	//if (len == 0) {
	//	return;
	//}

	//u32 max_height = font->get_text_size(text, text_alignment).height;
	//Font_Char *font_char = font->get_font_char(text[0]);
	//Vector2 position = { (float)x, (float)(y + (max_height - font_char->size.height) + (font_char->size.height - font_char->bearing.height)) };

	//Render_Primitive_2D info;
	//info.texture = render_font->font_atlas;
	//info.transform_matrix = make_translation_matrix(&position);
	//info.color = Color::White;
	//info.primitive = render_font->lookup_table[text[0]];
	//get_clip_rect(&info.clip_rect);

	//x += font_char->advance;
	//render_primitives.push(info);

	//for (u32 i = 1; i < len; i++) {
	//	Font_Char *font_char = font->get_font_char(text[i]);
	//	Vector2 position = { (float)x + font_char->bearing.width, (float)(y + (max_height - font_char->size.height) + (font_char->size.height - font_char->bearing.height)) };

	//	Render_Primitive_2D info;
	//	info.texture = render_font->font_atlas;
	//	info.transform_matrix = make_translation_matrix(&position);
	//	info.color = Color::White;
	//	info.primitive = render_font->lookup_table[text[i]];
	//	get_clip_rect(&info.clip_rect);

	//	x += font_char->advance;
	//	render_primitives.push(info);
	//}
}

void Render_Primitive_List::add_rect(Rect_s32 *rect, const Color &color, u32 rounding, u32 flags)
{
	float _x = static_cast<float>(rect->x);
	float _y = static_cast<float>(rect->y);
	float _width = static_cast<float>(rect->width);
	float _height = static_cast<float>(rect->height);

	add_rect(_x, _y, _width, _height, color, rounding, flags);
}

void Render_Primitive_List::add_rect(s32 x, s32 y, s32 width, s32 height, const Color &color, u32 rounding, u32 flags)
{
	float _x = static_cast<float>(x);
	float _y = static_cast<float>(y);
	float _width = static_cast<float>(width);
	float _height = static_cast<float>(height);

	add_rect(_x, _y, _width, _height, color, rounding, flags);
}

void Render_Primitive_List::add_rect(float x, float y, float width, float height, const Color &color, u32 rounding, u32 flags)
{
	////////////////////////////////////////////////////////////
	//// &Note I am not sure that chache works for primitives //
	////////////////////////////////////////////////////////////
	//String hash = String((int)width) + String((int)height) + String((int)rounding) + String((int)flags);


	//Vector2 position = { (float)x, (float)y };
	//Matrix4 transform_matrix = make_translation_matrix(&position);

	//Primitive_2D *primitive = make_or_find_primitive(transform_matrix, &render_2d->default_texture, color, hash);
	//if (!primitive) {
	//	return;
	//}

	//primitive->add_point(Vector2(width / 2.0f, height / 2.0f));

	//float x_divisor = (!(flags & ROUND_LEFT_RECT) || !(flags & ROUND_RIGHT_RECT)) ? 1.0f : 2.0f;
	//float y_divisor = (!(flags & ROUND_TOP_RECT) || !(flags & ROUND_BOTTOM_RECT)) ? 1.0f : 2.0f;
	//float x_rounding = math::min((float)rounding, (width / x_divisor));
	//float y_rounding = math::min((float)rounding, (height / y_divisor));

	//bool is_rounded = rounding > 0;
	//((flags & ROUND_TOP_LEFT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_LEFT_TOP, x_rounding, y_rounding) : primitive->add_point(Vector2(0.0f, 0.0f));
	//((flags & ROUND_TOP_RIGHT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_RIGHT_TOP, x_rounding, y_rounding) : primitive->add_point(Vector2(width, 0.0f));
	//((flags & ROUND_BOTTOM_RIGHT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_RIGHT_BOTTOM, x_rounding, y_rounding) : primitive->add_point(Vector2(width, height));
	//((flags & ROUND_BOTTOM_LEFT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_LEFT_BOTTOM, x_rounding, y_rounding) : primitive->add_point(Vector2(0.0f, height));

	//primitive->make_triangle_polygon();
	//render_2d->add_primitive(primitive);
}

//void Render_Primitive_List::add_texture(Rect_s32 *rect, Texture2D *resource)
//{
//	add_texture(rect->x, rect->y, rect->width, rect->height, resource);
//}
//
//void Render_Primitive_List::add_texture(int x, int y, int width, int height, Texture2D *resource)
//{
//	String hash = String(width + height);
//
//	Vector2 position = { (float)x, (float)y };
//	Matrix4 transform_matrix = make_translation_matrix(&position);
//
//	Primitive_2D *primitive = make_or_find_primitive(transform_matrix, resource, Color::White, hash);
//	if (!primitive) {
//		return;
//	}
//
//	primitive->add_point(Vector2(0.0f, 0.0f), Vector2(0.0f, 0.0f));
//	primitive->add_point(Vector2((float)width, 0.0f), Vector2(1.0f, 0.0f));
//	primitive->add_point(Vector2((float)width, (float)height), Vector2(1.0f, 1.0f));
//	primitive->add_point(Vector2(0.0f, (float)height), Vector2(0.0f, 1.0f));
//
//	primitive->make_triangle_polygon();
//	render_2d->add_primitive(primitive);
//}

void Render_Primitive_List::add_line(const Point_s32 &first_point, const Point_s32 &second_point, const Color &color, float thickness)
{
	//u32 window_width = Render_System::screen_width;
	//u32 window_height = Render_System::screen_height;

	//Point_s32 converted_point1;
	//from_win32_screen_space(window_width, window_height, first_point, converted_point1);

	//Point_s32 converted_point2;
	//from_win32_screen_space(window_width, window_height, second_point, converted_point2);

	//Vector2 temp = make_vector2(converted_point2, converted_point1);
	//Vector2 line_direction = normalize(&temp);

	//float angle = get_angle(&line_direction, &Vector2::base_x);

	//Matrix4 rotation_matrix;
	//if (line_direction.y < 0.0f) {
	//	angle = math::abs(RADIANS_360 - angle);
	//}

	//Vector2 position = first_point.to_vector2();
	//Matrix4 transform_matrix;
	//transform_matrix = rotate_about_z(-angle) * make_translation_matrix(&position);

	//Vector2 temp1 = first_point.to_vector2();
	//Vector2 temp2 = second_point.to_vector2();
	//float line_width = (float)find_distance(&temp1, &temp2);
	//String hash = String(line_width + thickness);

	//Primitive_2D *primitive = make_or_find_primitive(transform_matrix, &render_2d->default_texture, color, hash);
	//if (!primitive) {
	//	return;
	//}

	//primitive->add_point(Vector2(0.0f, 0.0f));
	//primitive->add_point(Vector2(line_width, 0.0f));
	//primitive->add_point(Vector2(line_width, thickness));
	//primitive->add_point(Vector2(0.0f, thickness));

	//primitive->make_triangle_polygon();
	//render_2d->add_primitive(primitive);
}

//Primitive_2D *Render_Primitive_List::make_or_find_primitive(Matrix4 &transform_matrix, Texture2D *texture, const Color &color, String &primitve_hash)
//{
	//Render_Primitive_2D render_primitive;
	//render_primitive.color.value = color.value;
	//render_primitive.texture = *texture;
	//render_primitive.transform_matrix = transform_matrix;
	//get_clip_rect(&render_primitive.clip_rect);

	//// if we found primitve we can just push it in render primitives array
	//Primitive_2D *found_primitive = NULL;
	//if (render_2d->lookup_table.get(primitve_hash, found_primitive)) {
	//	render_primitive.primitive = found_primitive;
	//	render_primitives.push(render_primitive);
	//	return NULL;
	//}

	//Primitive_2D *primitive = new Primitive_2D;
	//render_2d->lookup_table.set(primitve_hash, primitive);
	//render_primitive.primitive = primitive;
	//render_primitives.push(render_primitive);
	//return primitive;
//}

Render_2D::~Render_2D()
{
	Primitive_2D *primitive = NULL;
	For(primitives, primitive) {
		DELETE_PTR(primitive);
	}

	for (u32 i = 0; i < render_fonts.count; i++) {
		DELETE_PTR(render_fonts.get_node(i)->value);
	}
}

void Render_2D::init(Render_System *render_sys, Shader_Manager *shader_manager)
{
	//render_system = render_sys;
	//gpu_device = &render_system->gpu_device;
	//render_pipeline = &render_system->render_pipeline;

	//render_2d = GET_SHADER(shader_manager, render_2d);
	//if (!is_valid(render_2d, VALIDATE_RENDERING_SHADER)) {
	//	print("Render_2D::init: Failed to initialize Render_2D. {} is not valid.", render_2d->file_name);
	//	return;
	//}

	//gpu_device->create_constant_buffer(sizeof(CB_Render_2d_Info), &constant_buffer);

	//Texture2D_Desc texture_desc;
	//texture_desc.width = 100;
	//texture_desc.height = 100;
	//texture_desc.mip_levels = 1;
	//gpu_device->create_texture_2d(&texture_desc, &default_texture);
	//gpu_device->create_shader_resource_view(&texture_desc, &default_texture);

	//fill_texture((void *)&Color::White, &default_texture);

	//Rasterizer_Desc rasterizer_desc;
	//rasterizer_desc.set_sciccor(true);

	//gpu_device->create_rasterizer_state(&rasterizer_desc, &rasterizer_state);

	//Depth_Stencil_State_Desc depth_stencil_test_desc;
	//depth_stencil_test_desc.enable_depth_test = true;
	//depth_stencil_test_desc.depth_compare_func = COMPARISON_ALWAYS;

	//gpu_device->create_depth_stencil_state(&depth_stencil_test_desc, &depth_stencil_state);

	//Blend_State_Desc blending_test_desc;
	//blending_test_desc.enable = true;
	//blending_test_desc.src = BLEND_SRC_ALPHA;
	//blending_test_desc.dest = BLEND_INV_SRC_ALPHA;
	//blending_test_desc.blend_op = BLEND_OP_ADD;
	//blending_test_desc.src = BLEND_SRC_ALPHA;
	//blending_test_desc.src_alpha = BLEND_ONE;
	//blending_test_desc.dest_alpha = BLEND_INV_SRC_ALPHA;
	//blending_test_desc.blend_op_alpha = BLEND_OP_ADD;

	//gpu_device->create_blend_state(&blending_test_desc, &blend_state);

	//initialized = true;
}

void Render_2D::add_primitive(Primitive_2D *primitive)
{
	primitive->vertex_offset = total_vertex_count;
	primitive->index_offset = total_index_count;

	total_vertex_count += primitive->vertices.count;
	total_index_count += primitive->indices.count;

	primitives.push(primitive);
}

void Render_2D::add_render_primitive_list(Render_Primitive_List *render_primitive_list)
{
	draw_list.push(render_primitive_list);
}

Render_Font *Render_2D::get_render_font(Font *font)
{
	Render_Font *render_font = NULL;
	if (!render_fonts.get(font->name, &render_font)) {
		render_font = new Render_Font();
		render_font->init(this, font);
		render_fonts.set(font->name, render_font);
	}
	return render_font;
}

void Render_2D::new_frame()
{
	Render_Primitive_List *list = NULL;
	For(draw_list, list) {
		list->render_primitives.count = 0;
	}
	draw_list.count = 0;
}

void Render_2D::render_frame()
{
	//begin_mark_rendering_event(L"2D Rendering");

	//if ((total_vertex_count == 0) || !initialized) {
	//	return;
	//}

	//static u32 privious_total_vertex_count;

	//if (vertex_buffer.is_empty() || (privious_total_vertex_count != total_vertex_count)) {
	//	privious_total_vertex_count = total_vertex_count;

	//	if (!vertex_buffer.is_empty()) {
	//		vertex_buffer.free();
	//		index_buffer.free();
	//	}

	//	Gpu_Buffer_Desc vertex_buffer_desc;
	//	vertex_buffer_desc.data_count = total_vertex_count;
	//	vertex_buffer_desc.data_size = sizeof(Vertex_X2UV);
	//	vertex_buffer_desc.usage = RESOURCE_USAGE_DYNAMIC;
	//	vertex_buffer_desc.bind_flags = BIND_VERTEX_BUFFER;
	//	vertex_buffer_desc.cpu_access = CPU_ACCESS_WRITE;

	//	gpu_device->create_gpu_buffer(&vertex_buffer_desc, &vertex_buffer);

	//	Gpu_Buffer_Desc index_buffer_desc;
	//	index_buffer_desc.data_count = total_index_count;
	//	index_buffer_desc.data_size = sizeof(u32);
	//	index_buffer_desc.usage = RESOURCE_USAGE_DYNAMIC;
	//	index_buffer_desc.bind_flags = BIND_INDEX_BUFFER;
	//	index_buffer_desc.cpu_access = CPU_ACCESS_WRITE;

	//	gpu_device->create_gpu_buffer(&index_buffer_desc, &index_buffer);

	//	Vertex_X2UV *vertices = (Vertex_X2UV *)render_pipeline->map(vertex_buffer);
	//	u32 *indices = (u32 *)render_pipeline->map(index_buffer);

	//	Primitive_2D *primitive = NULL;
	//	For(primitives, primitive) {

	//		memcpy((void *)vertices, primitive->vertices.items, primitive->vertices.count * sizeof(Vertex_X2UV));
	//		memcpy((void *)indices, primitive->indices.items, primitive->indices.count * sizeof(u32));
	//		vertices += primitive->vertices.count;
	//		indices += primitive->indices.count;
	//	}

	//	render_pipeline->unmap(vertex_buffer);
	//	render_pipeline->unmap(index_buffer);
	//}

	//render_pipeline->set_input_layout(render_system->input_layouts.vertex_P2UV2);
	//render_pipeline->set_primitive(RENDER_PRIMITIVE_TRIANGLES);

	//render_pipeline->set_vertex_buffer(&vertex_buffer);
	//render_pipeline->set_index_buffer(&index_buffer);

	//render_pipeline->set_vertex_shader(render_2d);
	//render_pipeline->set_pixel_shader(render_2d);
	//render_pipeline->set_pixel_shader_sampler(POINT_SAMPLING_REGISTER, render_system->render_pipeline_states.point_sampling);

	//Viewport viewport;
	//viewport.width = Render_System::screen_width;
	//viewport.height = Render_System::screen_height;
	//render_pipeline->set_viewport(&viewport);
	//render_pipeline->set_rasterizer_state(rasterizer_state);
	//render_pipeline->set_blend_state(blend_state);
	//render_pipeline->set_depth_stencil_state(depth_stencil_state);
	//render_pipeline->set_render_target(render_system->multisampling_back_buffer_texture.rtv, render_system->multisampling_depth_stencil_texture.dsv);

	//CB_Render_2d_Info cb_render_info;

	//Render_Primitive_List *list = NULL;
	//For(draw_list, list) {
	//	Render_Primitive_2D *render_primitive = NULL;
	//	For(list->render_primitives, render_primitive) {

	//		render_pipeline->set_scissor(&render_primitive->clip_rect);

	//		cb_render_info.position_orthographic_matrix = render_primitive->transform_matrix * render_system->view.orthogonal_matrix;

	//		cb_render_info.color = render_primitive->color.value;

	//		render_pipeline->update_constant_buffer(&constant_buffer, &cb_render_info);
	//		render_pipeline->set_vertex_shader_resource(CB_RENDER_2D_INFO_REGISTER, constant_buffer);
	//		render_pipeline->set_pixel_shader_resource(CB_RENDER_2D_INFO_REGISTER, constant_buffer);
	//		render_pipeline->set_pixel_shader_resource(0, render_primitive->texture.srv);

	//		Primitive_2D *primitive = render_primitive->primitive;
	//		render_pipeline->draw_indexed(primitive->indices.count, primitive->index_offset, primitive->vertex_offset);
	//	}
	//}

	//render_pipeline->reset_rasterizer();
	//render_pipeline->reset_blending_state();
	//render_pipeline->reset_depth_stencil_state();

	//end_mark_rendering_event();
}

void View::update_projection_matries(u32 fov_in_degrees, u32 width, u32 height, float _near_plane, float _far_plane)
{
	ratio = (float)width / (float)height;
	fov = degrees_to_radians(60.0f);
	near_plane = _near_plane;
	far_plane = _far_plane;
	perspective_matrix = XMMatrixPerspectiveFovLH(fov, ratio, near_plane, far_plane);
	orthogonal_matrix = XMMatrixOrthographicOffCenterLH(0.0f, (float)width, (float)height, 0.0f, near_plane, far_plane);
}

void Render_System::init(Win32_Window *window)
{
	assert(window);
	assert(window->width > 0);
	assert(window->height > 0);

	Render_System::screen_width = window->width;
	Render_System::screen_height = window->height;


	print("Rendering system info:");
	//print("  Window resolution {}x{}.", Render_System::screen_width, Render_System::screen_height);
	print("  FOV {} degrees.", 60);
	print("  Render API based on Directx 11.");
}

void Render_System::init_render_targets(u32 window_width, u32 window_height)
{

}

void Render_System::init_input_layouts(Shader_Manager *shader_manager)
{
	//Input_Layout_Elements position_uv_input_layout;
	//position_uv_input_layout.add("POSITION", DXGI_FORMAT_R32G32_FLOAT);
	//position_uv_input_layout.add("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT);

	//Input_Layout_Elements position_input_layout;
	//position_input_layout.add("POSITION", DXGI_FORMAT_R32G32B32_FLOAT);

	//Extend_Shader *render_2d = GET_SHADER(shader_manager, render_2d);
	//Extend_Shader *draw_vertices = GET_SHADER(shader_manager, draw_vertices);
	//
	//gpu_device.create_input_layout((void *)render_2d->bytecode, render_2d->bytecode_size, &position_uv_input_layout, input_layouts.vertex_P2UV2);
	//gpu_device.create_input_layout((void *)draw_vertices->bytecode, draw_vertices->bytecode_size, &position_input_layout, input_layouts.vertex_P3);
}

void Render_System::resize(u32 window_width, u32 window_height)
{
	Render_System::screen_width = window_width;
	Render_System::screen_height = window_height;

	if (Engine::initialized()) {
	}
}

void Render_System::new_frame()
{

}

void Render_System::end_frame()
{
}

void Render_Font::init(Render_2D *render_2d, Font *font)
{
	assert(font);
	assert(render_2d);

	Hash_Table<char, Rect_f32> uvs;
	make_font_atlas(font, &uvs);

	// Exp (MAX_CHARACTERS - 1) skips DEL char
	for (u8 c = CONTORL_CHARACTERS; c < (MAX_CHARACTERS - 1); c++) {

		Size_u32 size = font->get_font_char(c)->size;
		Rect_f32 &uv = uvs[c];

		Primitive_2D *primitive = new Primitive_2D();

		primitive->add_point(Vector2(0.0f, 0.0f), Vector2(uv.x, uv.y));
		primitive->add_point(Vector2((float)size.width, 0.0f), Vector2(uv.right(), uv.y));
		primitive->add_point(Vector2((float)size.width, (float)size.height), Vector2(uv.right(), uv.bottom()));
		primitive->add_point(Vector2(0.0f, (float)size.height), Vector2(uv.x, uv.bottom()));

		primitive->make_triangle_polygon();
		//render_2d->add_primitive(primitive);

		lookup_table.set(c, primitive);
	}
}

void Render_Font::make_font_atlas(Font *font, Hash_Table<char, Rect_f32> *font_uvs)
{
	//assert(font);
	//assert(font_uvs);

	//Texture2D_Desc texture_atlas_desc;
	//texture_atlas_desc.width = 200;
	//texture_atlas_desc.height = 200;
	//texture_atlas_desc.mip_levels = 1;

	//Engine::get_render_system()->gpu_device.create_texture_2d(&texture_atlas_desc, &font_atlas);
	//Engine::get_render_system()->gpu_device.create_shader_resource_view(&texture_atlas_desc, &font_atlas);

	//fill_texture((void *)&Color::Black, &font_atlas);

	//Array<Rect_u32 *> rect_pointers;
	//Array<Rect_u32> rects;
	//rects.reserve(MAX_CHARACTERS);

	//for (u8 c = CONTORL_CHARACTERS; c < (MAX_CHARACTERS - 1); c++) {
	//	Font_Char *font_char = font->get_font_char(c);
	//	rects[c] = Rect_u32(font_char->size);
	//	rect_pointers.push(&rects[c]);
	//}

	//Rect_u32 atlas_rect;
	//atlas_rect.set_size(texture_atlas_desc.width, texture_atlas_desc.height);

	//pack_rects_in_rect(&atlas_rect, rect_pointers);

	//for (u8 c = CONTORL_CHARACTERS; c < (MAX_CHARACTERS - 1); c++) {
	//	Font_Char *font_char = font->get_font_char(c);
	//	Rect_u32 rect = rects[c];

	//	Rect_f32 uv;
	//	uv.x = static_cast<float>(rect.x) / static_cast<float>(texture_atlas_desc.width);
	//	uv.y = static_cast<float>(rect.y) / static_cast<float>(texture_atlas_desc.height);
	//	uv.width = static_cast<float>(rect.width) / static_cast<float>(texture_atlas_desc.width);
	//	uv.height = static_cast<float>(rect.height) / static_cast<float>(texture_atlas_desc.height);

	//	font_uvs->set((char)c, uv);

	//	if ((rect.width == 0) && (rect.height == 0)) {
	//		continue;
	//	}
	//	Engine::get_render_system()->render_pipeline.update_subresource(&font_atlas, (void *)font_char->bitmap, sizeof(u32) * font_char->size.width, &rect);
	//}
}

void Render_3D::init(Render_System *_render_system, Shader_Manager *shader_manager)
{
	//assert(_render_system);
	//assert(shader_manager);

	//render_system = _render_system;
	//draw_vertices = GET_SHADER(shader_manager, draw_vertices);
	//gpu_device = &render_system->gpu_device;
	//render_pipeline = &render_system->render_pipeline;

	//gpu_device->create_constant_buffer(sizeof(Draw_Info), &draw_info_cbuffer);
}

void Render_3D::shutdown()
{
	//vertex_buffer.free();
	//index_buffer.free();
	//draw_info_cbuffer.free();
}

void Render_3D::set_mesh(Vertex_Mesh *mesh)
{
	assert(mesh);

	if (mesh->empty()) {
		return;
	}
	//index_count = mesh->indices.count;
	//vertex_buffer.free();
	//index_buffer.free();

	//Gpu_Buffer_Desc vertex_buffer_desc;
	//vertex_buffer_desc.usage = RESOURCE_USAGE_DEFAULT;
	//vertex_buffer_desc.bind_flags = BIND_VERTEX_BUFFER;
	//vertex_buffer_desc.data = (void *)mesh->vertices.items;
	//vertex_buffer_desc.data_size = sizeof(Vector3);
	//vertex_buffer_desc.data_count = mesh->vertices.count;

	//gpu_device->create_gpu_buffer(&vertex_buffer_desc, &vertex_buffer);

	//Gpu_Buffer_Desc index_buffer_desc;
	//index_buffer_desc.usage = RESOURCE_USAGE_DEFAULT;
	//index_buffer_desc.bind_flags = BIND_INDEX_BUFFER;
	//index_buffer_desc.data = (void *)mesh->indices.items;
	//index_buffer_desc.data_size = sizeof(u32);
	//index_buffer_desc.data_count = mesh->indices.get_size();

	//gpu_device->create_gpu_buffer(&index_buffer_desc, &index_buffer);
}

void Render_3D::reset_mesh()
{
	//index_count = 0;
	//vertex_buffer.free();
	//index_buffer.free();
}

void Render_3D::draw_lines(const Vector3 &position, const Color &mesh_color)
{
	//render_pipeline->set_primitive(RENDER_PRIMITIVE_LINES);
	//draw(position, mesh_color);
}

void Render_3D::draw(const Vector3 &position, const Color &mesh_color)
{
	//render_pipeline->set_input_layout(render_system->input_layouts.vertex_P3);
	//render_pipeline->set_vertex_buffer(&vertex_buffer);
	//render_pipeline->set_index_buffer(&index_buffer);

	//render_pipeline->set_vertex_shader(draw_vertices);

	//render_pipeline->set_blend_state(render_system->render_pipeline_states.default_blend_state);
	//render_pipeline->set_depth_stencil_state(render_system->render_pipeline_states.default_depth_stencil_state);
	//render_pipeline->set_rasterizer_state(render_system->render_pipeline_states.default_rasterizer_state);

	//Viewport viewport;
	//viewport.width = Render_System::screen_width;
	//viewport.height = Render_System::screen_height;
	//
	//render_pipeline->set_viewport(&viewport);
	//render_pipeline->set_pixel_shader(draw_vertices);

	//render_pipeline->set_render_target(render_system->multisampling_back_buffer_texture.rtv, render_system->multisampling_depth_stencil_texture.dsv);

	//Draw_Info draw_info;
	//draw_info.color = mesh_color;
	//draw_info.world_matrix = make_translation_matrix((Vector3 *)&position);
	//render_pipeline->update_constant_buffer(&draw_info_cbuffer, (void *)&draw_info);

	//render_pipeline->set_vertex_shader_resource(0, draw_info_cbuffer);
	//render_pipeline->set_pixel_shader_resource(0, draw_info_cbuffer);

	//render_pipeline->draw_indexed(index_count, index_offset, vertex_offset);
}

void Render_3D::draw_triangles(const Vector3 &position, const Color &mesh_color)
{
	//render_pipeline->set_primitive(RENDER_PRIMITIVE_TRIANGLES);
	//draw(position, mesh_color);
}
