#include <assert.h>

#include "font.h"
#include "render_pass.h"
#include "render_system.h"
#include "render_helpers.h"

#include "../gui/gui.h"
#include "../sys/engine.h"
#include "../sys/sys_local.h"
#include "../win32/win_local.h"
#include "../libs/math/common.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"


const String HLSL_FILE_EXTENSION = "cso";

u32 Render_System::screen_width = 0;
u32 Render_System::screen_height = 0;


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
	if (split(&f_name, "_", &buffer)) {
		for (u32 i = 0; i < (buffer.count - 1); i++) {
			if (i != 0) {
				name.append("_");
			}
			name.append(buffer[i]);
		}
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

static void init_shaders_table(Gpu_Device *gpu_device, Hash_Table<String, Shader *> *shader_table)
{
	print("init_shaders_table: Load hlsl shaders.");

	String path_to_shader_dir;
	get_path_to_data_dir("shader", path_to_shader_dir);

	Array<String> file_names;
	bool success = get_file_names_from_dir(path_to_shader_dir + "\\", &file_names);
	
	if (!success) {
		error("init_shaders_table: has not found compiled shader files.");
	}

	for (u32 i = 0; i < file_names.count; i++) {
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
		shader_name.append(".hlsl");

		Shader *existing_shader = NULL;
		if (shader_table->get(shader_name, existing_shader)) {
			gpu_device->create_shader((u8 *)compiled_shader, file_size, shader_type, existing_shader);

			if (shader_type == VERTEX_SHADER) {
				existing_shader->byte_code = (u8 *)compiled_shader;
				existing_shader->byte_code_size = file_size;
			}
		} else {
			Shader *new_shader = new Shader();
			new_shader->name = shader_name;
			gpu_device->create_shader((u8 *)compiled_shader, file_size, shader_type, new_shader);

			if (shader_type == VERTEX_SHADER) {
				new_shader->byte_code = (u8 *)compiled_shader;
				new_shader->byte_code_size = file_size;
			}

			shader_table->set(shader_name, new_shader);
			print("init_shaders_table: {} was loaded.", shader_name);
		}
	}
}

static int compare_rects_u32(const void *first_rect, const void *second_rect)
{
	assert(first_rect);
	assert(second_rect);

	const Rect_u32 *first = static_cast<const Rect_u32 *>(first_rect);
	const Rect_u32 *second = static_cast<const Rect_u32 *>(second_rect);

	return first->height > second->height;
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
	clip_rects.push(*rect);
}

void Render_Primitive_List::pop_clip_rect()
{
	if (clip_rects.count > 0) {
		clip_rects.pop();
	}
}

void Render_Primitive_List::get_clip_rect(Rect_s32 *rect)
{
	*rect = clip_rects.last_item();
}

void Render_Primitive_List::add_outlines(int x, int y, int width, int height, const Color & color, float outline_width, u32 rounding, u32 flags)
{
	String hash = String((int)width) + String((int)height) + String((int)outline_width) + String("outline");

	Vector2 position = { (float)x, (float)y };
	Matrix4 transform_matrix;
	transform_matrix.translate(&position);

	Primitive_2D *primitive = make_or_find_primitive(transform_matrix, &render_2d->default_texture, color, hash);
	if (!primitive) {
		return;
	}
	bool is_rounded = rounding > 0;
	((flags & ROUND_TOP_LEFT_RECT) && is_rounded) ? primitive->add_rounded_points(-outline_width, -outline_width, (float)width + outline_width * 2.0f, (float)height + outline_width * 2.0f, RECT_SIDE_LEFT_TOP, rounding) : primitive->add_point(Vector2(-outline_width, -outline_width));
	((flags & ROUND_TOP_RIGHT_RECT) && is_rounded) ? primitive->add_rounded_points(-outline_width, -outline_width, (float)width + outline_width * 2.0f, (float)height + outline_width * 2.0f, RECT_SIDE_RIGHT_TOP, rounding) : primitive->add_point(Vector2((float)width + outline_width, -outline_width));
	((flags & ROUND_BOTTOM_RIGHT_RECT) && is_rounded) ? primitive->add_rounded_points(-outline_width, -outline_width, (float)width + outline_width * 2.0f, (float)height + outline_width * 2.0f, RECT_SIDE_RIGHT_BOTTOM, rounding) : primitive->add_point(Vector2((float)width + outline_width, (float)height + outline_width));
	((flags & ROUND_BOTTOM_LEFT_RECT) && is_rounded) ? primitive->add_rounded_points(-outline_width, -outline_width, (float)width + outline_width * 2.0f, (float)height + outline_width * 2.0f, RECT_SIDE_LEFT_BOTTOM, rounding) : primitive->add_point(Vector2(-outline_width, (float)height + outline_width));

	((flags & ROUND_TOP_LEFT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, (float)width, (float)height, RECT_SIDE_LEFT_TOP, rounding) : primitive->add_point(Vector2(0.0f, 0.0f));
	((flags & ROUND_TOP_RIGHT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, (float)width, (float)height, RECT_SIDE_RIGHT_TOP, rounding) : primitive->add_point(Vector2((float)width, 0.0f));
	((flags & ROUND_BOTTOM_RIGHT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, (float)width, (float)height, RECT_SIDE_RIGHT_BOTTOM, rounding) : primitive->add_point(Vector2((float)width, (float)height));
	((flags & ROUND_BOTTOM_LEFT_RECT) && is_rounded) ? primitive->add_rounded_points(0.0f, 0.0f, (float)width, (float)height, RECT_SIDE_LEFT_BOTTOM, rounding) : primitive->add_point(Vector2(0.0f, (float)height));

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

	u32 len = (u32)strlen(text);
	u32 max_height = font->get_text_size(text).height;

	for (u32 i = 0; i < len; i++) {

		u8 c = text[i];
		Font_Char *font_char = font->get_font_char(c);
		u32 x_pos = (u32)x + font_char->bearing.width;
		u32 y_pos = 0;
		if (font_char->size.height >= font_char->bearing.height) {
			y_pos = (u32)y + (max_height - font_char->size.height) + (font_char->size.height - font_char->bearing.height);
		} else {
			y_pos = (u32)y + max_height - font_char->size.height - font_char->bearing.height;
		}
		Vector2 position = Vector2((float)x_pos, (float)y_pos);
		
		Render_Primitive_2D info;
		info.texture = &render_font->font_atlas;
		info.transform_matrix.translate(&position);
		info.color = Color::White;
		info.primitive = render_font->lookup_table[c];
		get_clip_rect(&info.clip_rect);

		x += (font_char->advance_x >> 6);

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

	Primitive_2D *primitive = make_or_find_primitive(transform_matrix, &render_2d->default_texture, color, hash);
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

void Render_Primitive_List::add_texture(int x, int y, int width, int height, Texture2D *resource)
{
	String hash = String(width + height);

	Vector2 position = { (float)x, (float)y };
	Matrix4 transform_matrix;
	transform_matrix.translate(&position);

	Primitive_2D *primitive = make_or_find_primitive(transform_matrix, resource, Color::White, hash);
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
	u32 window_width = Render_System::screen_width;
	u32 window_height = Render_System::screen_height;

	Vector2 position = { (float)first_point->x, (float)first_point->y };
	Matrix4 position_matrix;
	position_matrix.translate(&position);

	Point_s32 converted_point1;
	from_win32_screen_space(window_width, window_height, first_point, &converted_point1);

	Point_s32 converted_point2;
	from_win32_screen_space(window_width, window_height, second_point, &converted_point2);

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

	float line_width = (float)distance(first_point, second_point);
	String hash = String(line_width + thickness);

	Primitive_2D *primitive = make_or_find_primitive(transform_matrix, &render_2d->default_texture, color, hash);
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

Primitive_2D *Render_Primitive_List::make_or_find_primitive(Matrix4 &transform_matrix, Texture2D *texture, const Color &color, String &primitve_hash)
{
	Render_Primitive_2D render_primitive;
	render_primitive.color.value = color.value;
	render_primitive.texture = texture;
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
	Primitive_2D *primitive = NULL;
	For(primitives, primitive) {
		DELETE_PTR(primitive);
	}

	for (u32 i = 0; i < render_fonts.count; i++) {
		DELETE_PTR(render_fonts.get_node(i)->value);
	}
}

void Render_2D::init(Engine *engine)
{
	render_system = &engine->render_sys;
	gpu_device = &render_system->gpu_device;
	render_pipeline = &render_system->render_pipeline;
	render_2d = render_system->get_shader("render_2d.hlsl");
	
	gpu_device->create_constant_buffer(sizeof(CB_Render_2d_Info), &constant_buffer);

	Texture_Desc texture_desc;
	texture_desc.width = 100;
	texture_desc.height = 100;
	texture_desc.mip_levels = 1;
	gpu_device->create_texture_2d(&texture_desc, &default_texture);

	fill_texture_with_value((void *)&Color::White, &default_texture);

	Rasterizer_Desc rasterizer_desc;
	rasterizer_desc.set_sciccor(true);
	
	gpu_device->create_rasterizer_state(&rasterizer_desc, &rasterizer_state);

	Depth_Stencil_State_Desc depth_stencil_test_desc;
	depth_stencil_test_desc.enable_depth_test = false;
	
	gpu_device->create_depth_stencil_state(&depth_stencil_test_desc, &depth_stencil_state);

	Blend_State_Desc blending_test_desc;
	blending_test_desc.enable = true;
	blending_test_desc.src = BLEND_SRC_ALPHA;
	blending_test_desc.dest = BLEND_INV_SRC_ALPHA;
	blending_test_desc.blend_op = BLEND_OP_ADD;
	blending_test_desc.src = BLEND_SRC_ALPHA;
	blending_test_desc.src_alpha = BLEND_ONE;
	blending_test_desc.dest_alpha = BLEND_INV_SRC_ALPHA;
	blending_test_desc.blend_op_alpha = BLEND_OP_ADD;

	gpu_device->create_blend_state(&blending_test_desc, &blend_state);
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
	if (total_vertex_count == 0) {
		return;
	}

	static u32 privious_total_vertex_count;

	if (vertex_buffer.is_empty() || (privious_total_vertex_count != total_vertex_count)) {
		privious_total_vertex_count = total_vertex_count;

		if (!vertex_buffer.is_empty()) {
			vertex_buffer.free();
			index_buffer.free();
		}

		Gpu_Buffer_Desc vertex_buffer_desc = make_vertex_buffer_desc(total_vertex_count, sizeof(Vertex_X2UV), NULL, RESOURCE_USAGE_DYNAMIC, CPU_ACCESS_WRITE);
		Gpu_Buffer_Desc index_buffer_desc = make_index_buffer_desc(total_index_count, NULL, RESOURCE_USAGE_DYNAMIC, CPU_ACCESS_WRITE);
		
		gpu_device->create_gpu_buffer(&vertex_buffer_desc, &vertex_buffer);
		gpu_device->create_gpu_buffer(&index_buffer_desc, &index_buffer);
		
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

	render_pipeline->set_vertex_buffer(&vertex_buffer);
	render_pipeline->set_index_buffer(&index_buffer);

	render_pipeline->set_vertex_shader(render_2d);
	render_pipeline->set_pixel_shader(render_2d);
	render_pipeline->set_pixel_shader_sampler(Render_Pipeline_States::default_sampler_state);

	render_pipeline->set_rasterizer_state(rasterizer_state);
	render_pipeline->set_blend_state(blend_state);
	render_pipeline->set_depth_stencil_state(depth_stencil_state);
	render_pipeline->set_render_target(render_system->render_targes.back_buffer, render_system->render_targes.back_buffer_depth);

	CB_Render_2d_Info cb_render_info;

	Render_Primitive_List *list = NULL;
	For(draw_list, list) {
		Render_Primitive_2D *render_primitive = NULL;
		For(list->render_primitives, render_primitive) {

			render_pipeline->set_scissor(&render_primitive->clip_rect);

			cb_render_info.position_orthographic_matrix = render_primitive->transform_matrix * render_system->view_info.orthogonal_matrix;
			
			cb_render_info.color = render_primitive->color.value;
			
			render_pipeline->update_constant_buffer(&constant_buffer, &cb_render_info);
			render_pipeline->set_vertex_shader_resource(0, constant_buffer);
			render_pipeline->set_pixel_shader_resource(0, constant_buffer);
			if (render_primitive->texture->get_pitch()) {
				render_pipeline->set_pixel_shader_resource(0, render_primitive->texture->view);
			}
			Primitive_2D *primitive = render_primitive->primitive;
			render_pipeline->draw_indexed(primitive->indices.count, primitive->index_offset, primitive->vertex_offset);
		}
	}

	render_pipeline->reset_rasterizer();
	render_pipeline->reset_blending_test();
	render_pipeline->reset_depth_stencil_test();
}

void View_Info::update_projection_matries(u32 width, u32 height, float _near_plane, float _far_plane)
{
	ratio = (float)width / (float)height;
	fov_y_ratio = XMConvertToRadians(45);
	near_plane = _near_plane;
	far_plane = _far_plane;
	perspective_matrix = XMMatrixPerspectiveFovLH(fov_y_ratio, ratio, near_plane, far_plane);
	orthogonal_matrix = XMMatrixOrthographicOffCenterLH(0.0f, (float)width, (float)height, 0.0f, near_plane, far_plane);
}

void Render_System::init(Engine *engine)
{
	Render_System::screen_width = engine->win32_info.window_width;
	Render_System::screen_height = engine->win32_info.window_height;

	win32_info = &engine->win32_info;
	win32_info->render_sys = this;

	screen_view.width = win32_info->window_width;
	screen_view.height = win32_info->window_height;

	view_info.update_projection_matries(win32_info->window_width, win32_info->window_height, 1.0f, 10000.0f);

	init_render_api(&gpu_device, &render_pipeline, win32_info);

	swap_chain.init(&gpu_device, win32_info);
	
	init_shaders_table(&gpu_device, &shader_table);
	
	Render_Pipeline_States::init(&gpu_device);

	render_2d.init(engine);

	gpu_device.create_input_layouts(shader_table);

	init_render_targets(win32_info->window_width, win32_info->window_height);
}

void Render_System::init_render_targets(u32 window_width, u32 window_height)
{	
	gpu_device.create_render_target(&swap_chain.back_buffer, &render_targes.back_buffer);

	Texture_Desc depth_texture_desc;
	depth_texture_desc.width = window_width;
	depth_texture_desc.height = window_height;
	depth_texture_desc.format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depth_texture_desc.mip_levels = 1;
	depth_texture_desc.bind = BIND_DEPTH_STENCIL;

	gpu_device.create_depth_stencil_buffer(&depth_texture_desc, &render_targes.back_buffer_depth);
}

void Render_System::resize(u32 window_width, u32 window_height)
{
	Render_System::screen_width = window_width;
	Render_System::screen_height = window_height;

	if (Engine::initialized()) {
		view_info.update_projection_matries(window_width, window_height, 1.0f, 10000.0f);
		assert(false);
		//swap_chain.resize(window_width, window_height);
	}
}

void Render_System::new_frame()
{
	render_pipeline.dx11_context->ClearRenderTargetView(render_targes.back_buffer.view.Get(), (float *)&Color::LightSteelBlue);
	render_pipeline.dx11_context->ClearDepthStencilView(render_targes.back_buffer_depth.view.Get(), CLEAR_DEPTH_BUFFER | CLEAR_STENCIL_BUFFER, 1.0f, 0);

	render_2d.new_frame();
}

void Render_System::end_frame()
{
	render_2d.render_frame();
	HR(swap_chain.dxgi_swap_chain->Present(0, 0));

#ifdef REPORT_LIVE_OBJECTS
	gpu_device.debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif
}

Shader *Render_System::get_shader(const char *name)
{
	return shader_table[name];
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
		primitive->add_point(Vector2((float)size.width, 0.0f), Vector2(uv.x + uv.width, uv.y));
		primitive->add_point(Vector2((float)size.width, (float)size.height), Vector2(uv.x + uv.width, uv.y + uv.height));
		primitive->add_point(Vector2(0.0f, (float)size.height), Vector2(uv.x, uv.y + uv.height));

		primitive->make_triangle_polygon();
		render_2d->add_primitive(primitive);

		lookup_table.set(c, primitive);
	}
}

void Render_Font::make_font_atlas(Font *font, Hash_Table<char, Rect_f32> *font_uvs)
{
	assert(font);
	assert(font_uvs);

	Texture_Desc texture_desc;
	texture_desc.width = 200;
	texture_desc.height = 200;
	texture_desc.mip_levels = 1;

	Engine::get_render_system()->gpu_device.create_texture_2d(&texture_desc, &font_atlas);

	fill_texture_with_value((void *)&Color::Black, &font_atlas);

	Array<Rect_u32 *> rect_pointers;
	Array<Rect_u32> rects;
	rects.reserve(MAX_CHARACTERS);

	for (u8 c = CONTORL_CHARACTERS; c < (MAX_CHARACTERS - 1); c++) {
		Font_Char *font_char = font->get_font_char(c);
		rects[c] = Rect_u32(font_char->bitmap_size);
		rect_pointers.push(&rects[c]);
	}

	Rect_u32 atlas_rect = Rect_u32(font_atlas.width, font_atlas.height);
	pack_rects_in_rect(&atlas_rect, rect_pointers);

	for (u8 c = CONTORL_CHARACTERS; c < (MAX_CHARACTERS - 1); c++) {		
		Font_Char *font_char = font->get_font_char(c);
		Rect_u32 rect = rects[c];

		Rect_f32 uv;
		uv.x = static_cast<float>(rect.x) / static_cast<float>(font_atlas.width);
		uv.y = static_cast<float>(rect.y) / static_cast<float>(font_atlas.height);
		uv.width = static_cast<float>(rect.width) / static_cast<float>(font_atlas.width);
		uv.height = static_cast<float>(rect.height) / static_cast<float>(font_atlas.height);

		font_uvs->set((char)c, uv);

		if ((rect.width == 0) && (rect.height == 0)) {
			continue;
		}
		Engine::get_render_system()->render_pipeline.update_subresource(&font_atlas, (void *)font_char->bitmap, sizeof(u32) * font_char->size.width, &rect);
	}
}
