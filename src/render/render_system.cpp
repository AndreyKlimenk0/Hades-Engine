#include <assert.h>

#include "font.h"
#include "render_system.h"
#include "../gui/gui.h"
#include "../sys/sys_local.h"
#include "../win32/win_local.h"
#include "../libs/math/common.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../sys/engine.h"


const String HLSL_FILE_EXTENSION = "cso";


inline void from_win32_screen_space(u32 screen_width, u32 screen_height, Point_s32 *win32_point, Point_s32 *normal_point)
{
	normal_point->x = win32_point->x - (screen_width / 2);
	normal_point->y = -win32_point->y + (screen_height / 2);
}

inline void to_win32_screen_space(Point_s32 *point, u32 screen_width, u32 screen_height, u32 *screen_x, u32 *screen_y)
{
	*screen_x = point->x + (screen_width / 2);
	*screen_y = -point->y + (screen_height / 2);
}

template< typename T >
inline Vector2 make_vector2(Point_V2<T> *first_point, Point_V2<T> *second_point)
{
	Point_V2<T> result = *first_point - *second_point;
	return Vector2((float)result.x, (float)result.y);
}

inline float get_angle_between_vectors(Vector2 &first_vector, Vector2 &second_vector)
{
	return math::arccos(first_vector.dot(second_vector) / (first_vector.length() * second_vector.length()));
}

inline Vector2 quad(float t, Vector2 p0, Vector2 p1, Vector2 p2)
{
	return (float)pow((1.0f - t), 2.0f) * p0 + 2.0f * (1.0f - t) * t * p1 + (float)pow(t, 2.0f) * p2;
}

static void get_shader_name_from_file(const char *file_name, String &name)
{
	String f_name = file_name;

	Array<String> buffer;
	split(&f_name, "_", &buffer);

	for (int i = 0; i < (buffer.count - 1); i++) {
		if (i != 0) {
			name.append("_");
		}
		name.append(buffer[i]);
	}
}

static bool get_shader_type_from_file_name(const char *file_name, Shader_Type *shader_type)
{
	String name;
	String file_extension;

	extract_file_extension(file_name, file_extension);
	if (file_extension != HLSL_FILE_EXTENSION) {
		print("get_shader_type_from_file: {} has wrong file extension.", file_name);
		return false;
	}

	extract_file_name(file_name, name);

	Array<String> strings;
	bool result = split(&name, "_", &strings);
	if (!result) {
		print("get_shader_type_from_file: can not extract shader type from {}.", file_name);
		return false;
	}

	String type = strings.last_item();

	if (type == "vs") {
		*shader_type = VERTEX_SHADER;
	} else if (type == "gs") {
		*shader_type = GEOMETRY_SHADER;
	} else if (type == "cs") {
		*shader_type = COMPUTE_SHADER;
	} else if (type == "hs") {
		*shader_type = HULL_SHADER;
	} else if (type == "ds") {
		*shader_type = DOMAIN_SHADER;
	} else if (type == "ps") {
		*shader_type = PIXEL_SHADER;
	} else {
		print("get_shader_type_from_file: can not extract shader type from {}.", file_name);
		return false;
	}
	return true;
}


static int compare_rects_u32(const void *first_rect, const void *second_rect)
{
	assert(first_rect);
	assert(second_rect);

	const Rect_u32 *first = static_cast<const Rect_u32 *>(first_rect);
	const Rect_u32 *second = static_cast<const Rect_u32 *>(second_rect);

	return first->height > second->height;
}

static void pack_rects_in_rect(Rect_u32 *main_rect, Array<Rect_u32 *> &rects)
{
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

void Primitive_2D::add_rounded_points(float x, float y, float width, float height, Rect_Side rect_side, float rounding)
{
	add_rounded_points(x, y, width, height, rect_side, rounding, rounding);
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

	s32 points_count_in_rounding = 20;
	s32 temp = 0;
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
	for (int i = 2; i < vertices.count; i++) {
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

Render_Primitive_List::Render_Primitive_List(Render_2D *render_2d) : render_2d(render_2d)
{
}

void Render_Primitive_List::push_clip_rect(Rect_s32 *rect)
{
	clip_rects.push(*rect);
}

void Render_Primitive_List::pop_clip_rect()
{
	if (clip_rects.count > 0) {
		clip_rects.pop();
	}
}

void Render_Primitive_List::get_clip_rect(Rect_s32 * rect)
{
	if (clip_rects.count > 0) {
		*rect = clip_rects.last_item();
	} else {
		rect->x = 0;
		rect->y = 0;
		rect->width = render_2d->render_system->win32_info->window_width;
		rect->height = render_2d->render_system->win32_info->window_height;
	}
}

void Render_Primitive_List::add_outlines(int x, int y, int width, int height, const Color & color, float outline_width, u32 rounding, u32 flags)
{
	String hash = String((int)width) + String((int)height) + String((int)outline_width) + String("outline");

	Vector2 position = { (float)x, (float)y };
	Matrix4 transform_matrix;
	transform_matrix.translate(&position);

	Primitive_2D *primitive = make_or_find_primitive(transform_matrix, render_2d->default_texture, color, hash);
	if (!primitive) {
		return;
	}
	bool is_rounded = rounding > 0;
	((flags & ROUND_TOP_LEFT_RECT) && is_rounded) ? primitive->add_rounded_points(-outline_width, -outline_width, width + outline_width * 2, height + outline_width * 2, RECT_SIDE_LEFT_TOP, rounding) : primitive->add_point(Vector2(outline_width, outline_width));
	((flags & ROUND_TOP_RIGHT_RECT) && is_rounded) ? primitive->add_rounded_points(-outline_width, -outline_width, width + outline_width * 2, height + outline_width * 2, RECT_SIDE_RIGHT_TOP, rounding) : primitive->add_point(Vector2((float)width - outline_width, outline_width));
	((flags & ROUND_BOTTOM_RIGHT_RECT) && is_rounded) ? primitive->add_rounded_points(-outline_width, -outline_width, width + outline_width * 2, height + outline_width * 2, RECT_SIDE_RIGHT_BOTTOM, rounding) : primitive->add_point(Vector2((float)width - outline_width, (float)height - outline_width));
	((flags & ROUND_BOTTOM_LEFT_RECT) && is_rounded) ? primitive->add_rounded_points(-outline_width, -outline_width, width + outline_width * 2, height + outline_width * 2, RECT_SIDE_LEFT_BOTTOM, rounding) : primitive->add_point(Vector2(outline_width, (float)height - outline_width));

	((flags & ROUND_TOP_LEFT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_LEFT_TOP, rounding) : primitive->add_point(Vector2(0.0f, 0.0f));
	((flags & ROUND_TOP_RIGHT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_RIGHT_TOP, rounding) : primitive->add_point(Vector2(width, 0.0f));
	((flags & ROUND_BOTTOM_RIGHT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_RIGHT_BOTTOM, rounding) : primitive->add_point(Vector2(width, height));
	((flags & ROUND_BOTTOM_LEFT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_LEFT_BOTTOM, rounding) : primitive->add_point(Vector2(0.0f, height));

	primitive->make_outline_triangle_polygons();
	render_2d->add_primitive(primitive);
}

void Render_Primitive_List::add_text(Rect_s32 *rect, const char *text)
{
	add_text((int)rect->x, (int)rect->y, text);
}

void Render_Primitive_List::add_text(int x, int y, const char *text)
{
	assert(text);

	u32 len = strlen(text);
	u32 max_height = render_2d->font->get_text_size(text).height;

	for (u32 i = 0; i < len; i++) {

		char c = text[i];
		Font_Char &font_char = render_2d->font->characters[c];
		Vector2 position = Vector2(x + font_char.bearing.width, y + (max_height - font_char.size.height) + (font_char.size.height - font_char.bearing.height));
		
		Render_Primitive_2D info;
		info.gpu_resource = render_2d->font_atlas;
		info.transform_matrix.translate(&position);
		info.color = Color::White;
		info.primitive = render_2d->lookup_table[String(c)];
		get_clip_rect(&info.clip_rect);

		x += (font_char.advance >> 6);

		render_primitives.push(info);
	}
}

void Render_Primitive_List::add_rect(Rect_s32 *rect, const Color &color, u32 rounding, u32 flags)
{
	float _x = static_cast<float>(rect->x);
	float _y = static_cast<float>(rect->y);
	float _width = static_cast<float>(rect->width);
	float _height = static_cast<float>(rect->height);

	add_rect(_x, _y, _width, _height, color, rounding, flags);
}

void Render_Primitive_List::add_rect(s32 x, s32 y, s32 width, s32 height, const Color & color, u32 rounding, u32 flags)
{
	float _x = static_cast<float>(x);
	float _y = static_cast<float>(y);
	float _width = static_cast<float>(width);
	float _height = static_cast<float>(height);

	add_rect(_x, _y, _width, _height, color, rounding, flags);
}

void Render_Primitive_List::add_rect(float x, float y, float width, float height, const Color &color, u32 rounding, u32 flags)
{
	//////////////////////////////////////////////////////////
	// &Note I am not sure that chache works for primitives //
	//////////////////////////////////////////////////////////
	String hash = String((int)width) + String((int)height) + String((int)rounding) + String((int)flags);


	Vector2 position = { (float)x, (float)y };
	Matrix4 transform_matrix;
	transform_matrix.translate(&position);

	Primitive_2D *primitive = make_or_find_primitive(transform_matrix, render_2d->default_texture, color, hash);
	if (!primitive) {
		return;
	}

	primitive->add_point(Vector2(width / 2.0f, height / 2.0f));

	float x_divisor = (!(flags & ROUND_LEFT_RECT) || !(flags & ROUND_RIGHT_RECT)) ? 1.0f : 2.0f;
	float y_divisor = (!(flags & ROUND_TOP_RECT) || !(flags & ROUND_BOTTOM_RECT)) ? 1.0f : 2.0f;
	float x_rounding = math::min((float)rounding, (width / x_divisor));
	float y_rounding = math::min((float)rounding, (height / y_divisor));

	bool is_rounded = rounding > 0;
	((flags & ROUND_TOP_LEFT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_LEFT_TOP, x_rounding, y_rounding) : primitive->add_point(Vector2(0.0f, 0.0f));
	((flags & ROUND_TOP_RIGHT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_RIGHT_TOP, x_rounding, y_rounding) : primitive->add_point(Vector2(width, 0.0f));
	((flags & ROUND_BOTTOM_RIGHT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_RIGHT_BOTTOM, x_rounding, y_rounding) : primitive->add_point(Vector2(width, height));
	((flags & ROUND_BOTTOM_LEFT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_LEFT_BOTTOM, x_rounding, y_rounding) : primitive->add_point(Vector2(0.0f, height));

	primitive->make_triangle_polygon();
	render_2d->add_primitive(primitive);
}

void Render_Primitive_List::add_texture(int x, int y, int width, int height, Texture *gpu_resource)
{
	String hash = String(width + height);

	Vector2 position = { (float)x, (float)y };
	Matrix4 transform_matrix;
	transform_matrix.translate(&position);

	Primitive_2D *primitive = make_or_find_primitive(transform_matrix, gpu_resource, Color::White, hash);
	if (!primitive) {
		return;
	}

	primitive->add_point(Vector2(0.0f, 0.0f), Vector2(0.0f, 0.0f));
	primitive->add_point(Vector2((float)width, 0.0f), Vector2(1.0f, 0.0f));
	primitive->add_point(Vector2((float)width, (float)height), Vector2(1.0f, 1.0f));
	primitive->add_point(Vector2(0.0f, (float)height), Vector2(0.0f, 1.0f));

	primitive->make_triangle_polygon();
	render_2d->add_primitive(primitive);
}

void Render_Primitive_List::add_line(Point_s32 *first_point, Point_s32 *second_point, const Color &color, float thickness)
{
	Vector2 position = { (float)first_point->x, (float)first_point->y };
	Matrix4 position_matrix;
	position_matrix.translate(&position);

	Point_s32 converted_point1;
	from_win32_screen_space(get_window_width(), get_window_height(), first_point, &converted_point1);

	Point_s32 converted_point2;
	from_win32_screen_space(get_window_width(), get_window_height(), second_point, &converted_point2);

	Vector2 line_direction = make_vector2(&converted_point2, &converted_point1);
	line_direction.normalize();

	float angle = get_angle_between_vectors(line_direction, base_x_vec2);

	Matrix4 rotation_matrix;
	if (line_direction.y < 0.0f) {
		angle = math::abs(RADIANS_360 - angle);
	}
	
	rotation_matrix.rotate_about_z(angle);

	Matrix4 transform_matrix;
	transform_matrix = rotation_matrix * position_matrix;

	float line_width = distance(first_point, second_point);
	String hash = String(line_width + thickness);

	Primitive_2D *primitive = make_or_find_primitive(transform_matrix, render_2d->default_texture, color, hash);
	if (!primitive) {
		return;
	}

	primitive->add_point(Vector2(0.0f, 0.0f));
	primitive->add_point(Vector2(line_width, 0.0f));
	primitive->add_point(Vector2(line_width, thickness));
	primitive->add_point(Vector2(0.0f, thickness));

	primitive->make_triangle_polygon();
	render_2d->add_primitive(primitive);
}

Primitive_2D *Render_Primitive_List::make_or_find_primitive(Matrix4 &transform_matrix, Texture *texture, const Color &color, String &primitve_hash)
{
	Render_Primitive_2D render_primitive;
	render_primitive.color.value = color.value;
	render_primitive.gpu_resource = texture;
	render_primitive.transform_matrix = transform_matrix;
	get_clip_rect(&render_primitive.clip_rect);

	// if we found primitve we can just push it in render primitives array
	Primitive_2D *found_primitive = NULL;
	if (render_2d->lookup_table.get(primitve_hash, found_primitive)) {
		render_primitive.primitive = found_primitive;
		render_primitives.push(render_primitive);
		return NULL;
	}

	Primitive_2D *primitive = new Primitive_2D;
	render_2d->lookup_table.set(primitve_hash, primitive);
	render_primitive.primitive = primitive;
	render_primitives.push(render_primitive);
	return primitive;
}


struct CB_Render_2d_Info {
	Matrix4 position_orthographic_matrix;
	Vector4 color;
};

Render_2D::~Render_2D()
{
	free_com_object(depth_test);
	new_frame();
}

void Render_2D::init(Render_System *_render_system, Shader *_render_2d, Font *_font)
{
	render_system = _render_system;
	gpu_device = &render_system->gpu_device;
	render_pipeline = &render_system->render_pipeline;
	render_2d = _render_2d;
	font = _font;
	
	constant_buffer = gpu_device->create_constant_buffer(sizeof(CB_Render_2d_Info));
	default_texture = gpu_device->create_texture_2d(100, 100, NULL, 1);

	u32 *pixel_buffer = create_color_buffer(default_texture->width, default_texture->height, Color::White);
	render_pipeline->update_subresource(default_texture, (void *)pixel_buffer, default_texture->get_row_pitch());
	DELETE_PTR(pixel_buffer);

	Rasterizer_Desc rasterizer_desc;
	rasterizer_desc.set_counter_clockwise(true);
	rasterizer_desc.set_sciccor(true);
	
	rasterizer = gpu_device->create_rasterizer(&rasterizer_desc);

	Depth_Stencil_Test_Desc depth_stencil_test_desc;
	depth_stencil_test_desc.enable_depth_test = false;
	
	depth_test = gpu_device->create_depth_stencil_test(&depth_stencil_test_desc);

	Blending_Test_Desc blending_test_desc;
	blending_test_desc.enable = true;
	blending_test_desc.src = BLEND_SRC_ALPHA;
	blending_test_desc.dest = BLEND_INV_SRC_ALPHA;
	blending_test_desc.blend_op = BLEND_OP_ADD;
	blending_test_desc.src = BLEND_SRC_ALPHA;
	blending_test_desc.src_alpha = BLEND_ONE;
	blending_test_desc.dest_alpha = BLEND_INV_SRC_ALPHA;
	blending_test_desc.blend_op_alpha = BLEND_OP_ADD;

	blending_test = gpu_device->create_blending_test(&blending_test_desc);

	init_font_rendering();
}

void Render_2D::init_font_rendering()
{
	Hash_Table<char, Rect_f32> uvs;
	init_font_atlas(font, &uvs);

	for (char c = 32; c < 127; c++) {

		if (font->characters.key_in_table(c)) {
			Font_Char &font_char = font->characters[c];
			Size_u32 &size = font_char.size;
			Rect_f32 &uv = uvs[c];

			Primitive_2D *primitive = new Primitive_2D();

			primitive->add_point(Vector2(0.0f, 0.0f), Vector2(uv.x, uv.y));
			primitive->add_point(Vector2((float)size.width, 0.0f), Vector2(uv.x + uv.width, uv.y));
			primitive->add_point(Vector2((float)size.width, (float)size.height), Vector2(uv.x + uv.width, uv.y + uv.height));
			primitive->add_point(Vector2(0.0f, (float)size.height), Vector2(uv.x, uv.y + uv.height));

			primitive->make_triangle_polygon();
			add_primitive(primitive);

			lookup_table.set(String(c), primitive);
		}
	}
}

void Render_2D::init_font_atlas(Font *font, Hash_Table<char, Rect_f32> *font_uvs)
{
	font_atlas = gpu_device->create_texture_2d(200, 200, NULL, 1);

	u32 *pixel_buffer = create_color_buffer(font_atlas->width, font_atlas->height, Color::White);
	render_pipeline->update_subresource(font_atlas, (void *)pixel_buffer, font_atlas->get_row_pitch());
	DELETE_PTR(pixel_buffer);

	Array<Rect_u32 *> rects;
	Hash_Table<char, Rect_u32> table;

	for (u32 i = 32; i < 127; i++) {
		if (font->characters.key_in_table((char)i)) {
			Font_Char &font_char = font->characters[(char)i];
			table.set((char)i, Rect_u32(font_char.bitmap_size));
			rects.push(&table[(char)i]);
		}
	}
	
	Rect_u32 main_rect = Rect_u32(font_atlas->width, font_atlas->height);
	pack_rects_in_rect(&main_rect, rects);

	for (u32 i = 32; i < 127; i++) {
		if (font->characters.key_in_table((char)i)) {
			Font_Char &character = font->characters[(char)i];
			
			Rect_u32 &rect = table[(char)i];

			Rect_f32 uv;
			uv.x = static_cast<float>(rect.x) / static_cast<float>(font_atlas->width);
			uv.y = static_cast<float>(rect.y) / static_cast<float>(font_atlas->height);
			uv.width = static_cast<float>(rect.width) / static_cast<float>(font_atlas->width);
			uv.height = static_cast<float>(rect.height) / static_cast<float>(font_atlas->height);

			font_uvs->set((char)i, uv);

			render_pipeline->update_subresource(font_atlas, (void *)character.bitmap, sizeof(u32) * character.size.width, &rect);
		}
	}
}

void Render_2D::add_primitive(Primitive_2D *primitive)
{
	primitive->vertex_offset = total_vertex_count;
	primitive->index_offset = total_index_count;

	total_vertex_count += primitive->vertices.count;
	total_index_count +=  primitive->indices.count;

	primitives.push(primitive);
}

void Render_2D::add_render_primitive_list(Render_Primitive_List *render_primitive_list)
{
	draw_list.push(render_primitive_list);
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
	if (total_vertex_count == 0) {
		return;
	}

	static u32 privious_total_vertex_count;

	if (!vertex_buffer || (privious_total_vertex_count != total_vertex_count)) {
		privious_total_vertex_count = total_vertex_count;

		if (vertex_buffer) {
			free_com_object(vertex_buffer->buffer);
			free_com_object(index_buffer->buffer);
		}

		Vertex_Buffer_Desc vertex_buffer_desc = Vertex_Buffer_Desc(total_vertex_count, sizeof(Vertex_X2UV), NULL, RESOURCE_USAGE_DYNAMIC, CPU_ACCESS_WRITE);
		Index_Buffer_Desc index_buffer_desc = Index_Buffer_Desc(total_index_count, NULL, RESOURCE_USAGE_DYNAMIC, CPU_ACCESS_WRITE);
		
		vertex_buffer = gpu_device->create_gpu_buffer(&vertex_buffer_desc);
		index_buffer = gpu_device->create_gpu_buffer(&index_buffer_desc);
		
		Vertex_X2UV *vertices = (Vertex_X2UV *)render_pipeline->map(vertex_buffer);
		u32 *indices = (u32 *)render_pipeline->map(index_buffer);

		Primitive_2D *primitive = NULL;
		For(primitives, primitive) {

			memcpy((void *)vertices, primitive->vertices.items, primitive->vertices.count * sizeof(Vertex_X2UV));
			memcpy((void *)indices, primitive->indices.items, primitive->indices.count * sizeof(u32));
			vertices += primitive->vertices.count;
			indices += primitive->indices.count;
		}
		
		render_pipeline->unmap(vertex_buffer);
		render_pipeline->unmap(index_buffer);
	}

	render_pipeline->set_input_layout(Gpu_Device::vertex_xuv);
	render_pipeline->set_primitive(RENDER_PRIMITIVE_TRIANGLES);

	render_pipeline->set_vertex_buffer(vertex_buffer);
	render_pipeline->set_index_buffer(index_buffer);

	render_pipeline->set_vertex_shader(render_2d);
	render_pipeline->set_pixel_shader(render_2d);
	render_pipeline->set_pixel_shader_sampler(render_system->sampler);

	render_pipeline->set_rasterizer(rasterizer);
	render_pipeline->set_blending_text(blending_test);
	render_pipeline->set_depth_stencil_test(depth_test);

	CB_Render_2d_Info cb_render_info;


	Render_Primitive_List *list = NULL;
	For(draw_list, list) {
		Render_Primitive_2D *render_primitive = NULL;
		For(list->render_primitives, render_primitive) {

			render_pipeline->set_scissor(&render_primitive->clip_rect);

			cb_render_info.position_orthographic_matrix = render_primitive->transform_matrix * render_system->view_info.orthogonal_matrix;
			
			cb_render_info.color = render_primitive->color.value;
			
			render_pipeline->update_constant_buffer(constant_buffer, &cb_render_info);
			render_pipeline->set_veretex_shader_resource(constant_buffer);
			render_pipeline->set_pixel_shader_resource(constant_buffer);
			if (render_primitive->gpu_resource) {
				render_pipeline->set_pixel_shader_resource(render_primitive->gpu_resource->shader_resource);
			}
			Primitive_2D *primitive = render_primitive->primitive;
			render_pipeline->draw_indexed(primitive->indices.count, primitive->index_offset, primitive->vertex_offset);
		}
	}

	render_pipeline->reset_rasterizer();
	render_pipeline->reset_blending_test();
	render_pipeline->reset_depth_stencil_test();
}

void View_Info::init(u32 width, u32 height, float _near_plane, float _far_plane)
{
	ratio = (float)width / (float)height;
	fov_y_ratio = XMConvertToRadians(45);
	near_plane = _near_plane;
	far_plane = _far_plane;
	perspective_matrix = XMMatrixPerspectiveFovLH(fov_y_ratio, ratio, near_plane, far_plane);
	orthogonal_matrix = XMMatrixOrthographicOffCenterLH(0.0f, (float)width, (float)height, 0.0f, near_plane, far_plane);
}

void View_Info::update_projection_matries(u32 new_window_width, u32 new_window_height)
{
	ratio = (float)new_window_width / (float)new_window_height;
	perspective_matrix = XMMatrixPerspectiveFovLH(fov_y_ratio, ratio, near_plane, far_plane);
	orthogonal_matrix =  XMMatrixOrthographicOffCenterLH(0.0f, (float)new_window_width, (float)new_window_height, 0.0f, near_plane, far_plane);
}


Render_System::~Render_System()
{
	shutdown();
}

void Render_System::init(Win32_Info *_win32_info, Font *font)
{
	win32_info = _win32_info;
	win32_info->render_sys = this;
	
	view_info.init(win32_info->window_width, win32_info->window_height, 1.0f, 10000.0f);

	init_render_api(&gpu_device, &render_pipeline, win32_info);
	init_shaders();

	Shader *render_2d_shader = shaders["render_2d"];
	render_2d.init(this, render_2d_shader, font);

	sampler = gpu_device.create_sampler();

	gpu_device.create_input_layouts(shaders);
}

void Render_System::init_shaders()
{
	Array<String> file_names;

	String path_to_shader_dir;
	get_path_to_data_dir("shader", path_to_shader_dir);

	bool success = get_file_names_from_dir(path_to_shader_dir + "\\", &file_names);
	if (file_names.is_empty() || !success) {
		error("Shader_Manamager::init: has not found compiled shader files.");
	}

	for (int i = 0; i < file_names.count; i++) {

		Shader_Type shader_type;
		if (!get_shader_type_from_file_name(file_names[i].c_str(), &shader_type)) {
			continue;
		}

		String path_to_shader_file;
		build_full_path_to_shader_file(file_names[i], path_to_shader_file);


		int file_size;
		char *compiled_shader = read_entire_file(path_to_shader_file, "rb", &file_size);
		if (!compiled_shader) {
			continue;
		}

		String shader_name;
		get_shader_name_from_file(file_names[i].c_str(), shader_name);


		Shader *existing_shader = NULL;
		if (shaders.get(shader_name, existing_shader)) {
			gpu_device.create_shader((u8 *)compiled_shader, file_size, shader_type, existing_shader);

			if (shader_type == VERTEX_SHADER) {
				existing_shader->byte_code = (u8 *)compiled_shader;
				existing_shader->byte_code_size = file_size;
			}
		} else {
			Shader *new_shader = new Shader();
			new_shader->name = shader_name;
			gpu_device.create_shader((u8 *)compiled_shader, file_size, shader_type, new_shader);

			if (shader_type == VERTEX_SHADER) {
				new_shader->byte_code = (u8 *)compiled_shader;
				new_shader->byte_code_size = file_size;
			}

			shaders.set(shader_name, new_shader);
		}
	}
}

void Render_System::resize()
{
}

void Render_System::shutdown()
{
}

void Render_System::new_frame()
{
	assert(render_pipeline.render_target_view);
	assert(render_pipeline.depth_stencil_view);

	render_pipeline.pipeline->ClearRenderTargetView(render_pipeline.render_target_view, (float *)&Color::LightSteelBlue);
	render_pipeline.pipeline->ClearDepthStencilView(render_pipeline.depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void Render_System::end_frame()
{
	HR(render_pipeline.swap_chain->Present(0, 0));
}

void Render_System::render_frame()
{

	render_2d.new_frame();

	gui::draw_test_gui();

	render_2d.render_frame();
}