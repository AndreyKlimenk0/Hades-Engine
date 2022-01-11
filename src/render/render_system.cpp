#include <assert.h>
#include "directx.h"
#include "render_system.h"
#include "../win32/win_local.h"
#include "../sys/sys_local.h"


Render_System render_sys;

const u32 ROUND_TOP_LEFT_RECT = 0x1;
const u32 ROUND_TOP_RIGHT_RECT = 0x4;
const u32 ROUND_BOTTOM_LEFT_RECT = 0x2;
const u32 ROUND_BOTTOM_RIGHT_RECT = 0x8;
const u32 ROUND_TOP_RECT = ROUND_TOP_LEFT_RECT | ROUND_TOP_RIGHT_RECT;
const u32 ROUND_BOTTOM_RECT = ROUND_BOTTOM_LEFT_RECT | ROUND_BOTTOM_RIGHT_RECT;
const u32 ROUND_RECT = ROUND_TOP_RECT | ROUND_BOTTOM_RECT;

void draw_rect(int x, int y, int width, int height, const Color &color, u32 rounding = 0, u32 flags = ROUND_RECT);

Matrix4 make_world_view_perspective_matrix(Entity *entity)
{
	Matrix4 world;
	entity->get_world_matrix(world);
	return world * render_sys.view_matrix * render_sys.view_info->perspective_matrix;
}

static void draw_indexed_mesh(Triangle_Mesh *mesh)
{
	assert(mesh->index_buffer);
	assert(mesh->vertex_buffer);

	directx11.device_context->IASetInputLayout(Input_Layout::vertex);
	directx11.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	directx11.device_context->IASetVertexBuffers(0, 1, &mesh->vertex_buffer, &stride, &offset);
	directx11.device_context->IASetIndexBuffer(mesh->index_buffer, DXGI_FORMAT_R32_UINT, 0);

	directx11.device_context->DrawIndexed(mesh->index_count, 0, 0);
}

static void draw_not_indexed_mesh(Triangle_Mesh *mesh)
{
	directx11.device_context->IASetInputLayout(Input_Layout::vertex);
	directx11.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	directx11.device_context->IASetVertexBuffers(0, 1, &mesh->vertex_buffer, &stride, &offset);

	directx11.device_context->Draw(mesh->vertex_count, 0);
}

static void draw_mesh(Triangle_Mesh *mesh)
{
	assert(mesh);
	assert(mesh->vertex_buffer);

	if (mesh->is_indexed) {
		draw_indexed_mesh(mesh);
	} else {
		draw_not_indexed_mesh(mesh);
	}
}

static void draw_world_entities(World *world)
{
	Entity_Manager *entity_manager = &world->entity_manager;

	Fx_Shader *light = fx_shader_manager.get_shader("forward_light");

	bind_light(light, &entity_manager->lights);

	light->bind_per_frame_info(render_sys.free_camera);

	Render_Entity *render_entity = NULL;
	For(world->render_entities, render_entity)
	{

		if (render_entity->stencil_test) {
			enable_stencil_test(render_entity->stencil_test, render_entity->stencil_ref_value);
		}

		if (render_entity->call_before_drawing_entity) {
			render_entity->call_before_drawing_entity(render_entity);
		}

		bind(light, render_entity->entity);

		Render_Mesh *render_mesh = NULL;
		For(render_entity->render_model->render_meshes, render_mesh)
		{
			bind(light, render_mesh);
			light->attach("draw");
			draw_mesh(&render_mesh->mesh);
		}

		if (render_entity->call_after_drawn_entity) {
			render_entity->call_after_drawn_entity(render_entity);
		}

		if (render_entity->stencil_test) {
			disable_stencil_test();
		}

	}
}

Render_System::~Render_System()
{
	shutdown();
}

void Render_System::init(View_Info *view_info)
{
	this->view_info = view_info;
}

void Render_System::shutdown()
{
	DELETE_PTR(view_info);
}

void Render_System::resize()
{
	if (view_info) { view_info->update_projection_matries(); }
}

#include "font.h"

void Render_System::render_frame()
{
	//Texture *texture = texture_manager.get_texture("cross.png");
	//draw_texture_on_screen(700, 400, texture, 25.0f, 25.0f);
	//view_matrix = free_camera->get_view_matrix();

	//Render_Entity *render_entity = NULL;
	//For(current_render_world->render_entities, render_entity)
	//{
	//	make_outlining(render_entity);
	//}

	//draw_world_entities(current_render_world);

	//For(current_render_world->render_entities, render_entity)
	//{
	//	free_outlining(render_entity);
	//}

	//draw_texture_on_screen(0, 0, texture_manager.get_texture("Lion_Albedo.png"), 250.0f, 250.0f);

	//draw_text(0.5, 0.5, "AAAAAAAAAAAAAAAAAAAAA");
	//draw_rect(500, 150, 500, 500, Color::Green);
	draw_rect(800, 450, 300, 300, Color::Blue, 50);
	direct2d.draw_rounded_rect(10, 10, 300, 300, 50, 50, Color::Blue);
}

View_Info *make_view_info(float near_plane, float far_plane)
{
	View_Info *view_info = new View_Info();
	view_info->window_width = win32.window_width;
	view_info->window_height = win32.window_height;
	view_info->window_ratio = (float)view_info->window_width / (float)view_info->window_height;
	view_info->fov_y_ratio = XMConvertToRadians(45);
	view_info->near_plane = near_plane;
	view_info->far_plane = far_plane;
	view_info->perspective_matrix = XMMatrixPerspectiveFovLH(view_info->fov_y_ratio, view_info->window_ratio, near_plane, far_plane);
	view_info->orthogonal_matrix = XMMatrixOrthographicOffCenterLH(0.0f, view_info->window_width, 0.0f, view_info->window_height, near_plane, far_plane);
	return view_info;
}

inline Stencil_Test *make_outlining_stencil_test()
{
	return make_stecil_test(D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_REPLACE, D3D11_COMPARISON_ALWAYS);
}

void draw_outlining(Render_Entity *render_entity)
{
	Fx_Shader *color = fx_shader_manager.get_shader("base");

	Matrix4 world = render_entity->entity->get_world_matrix();
	Matrix4 wvp_projection = make_world_view_perspective_matrix(render_entity->entity);

	color->bind("world", &world);
	color->bind("world_view_projection", &wvp_projection);

	color->attach("draw_outlining");

	Stencil_Test *second_test = make_stecil_test(D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_REPLACE, D3D11_COMPARISON_NOT_EQUAL, 0x0, 0xff);
	enable_stencil_test(second_test, render_entity->entity->id);

	draw_mesh(render_entity->render_model->get_triangle_mesh());

	disable_stencil_test();
	free_com_object(second_test);
}

void make_outlining(Render_Entity *render_entity)
{
	render_entity->stencil_ref_value = render_entity->entity->id;
	render_entity->stencil_test = make_outlining_stencil_test();
	render_entity->call_after_drawn_entity = draw_outlining;
}

void free_outlining(Render_Entity *render_entity)
{
	render_entity->stencil_ref_value = 0;
	free_com_object(render_entity->stencil_test);
	render_entity->call_after_drawn_entity = NULL;
}

inline float x_to_screen_space(float x)
{
	return 2 * x / (win32.window_width - 0) - (win32.window_width + 0) / (win32.window_width - 0);
}

inline float y_to_screen_space(float y)
{
	return 2 * y / (0 - win32.window_height) - (0 + win32.window_height) / (0 - win32.window_height);
}

typedef ID3D11Buffer Gpu_Buffer;

Gpu_Buffer *make_gpu_buffer(D3D11_USAGE usage, u32 bind_flags, u32 data_size, u32 data_count, void *data)
{
	assert(data);

	Gpu_Buffer *buffer = NULL;

	D3D11_BUFFER_DESC buffer_desc;
	ZeroMemory(&buffer_desc, sizeof(D3D11_BUFFER_DESC));
	buffer_desc.Usage = usage;
	buffer_desc.BindFlags = bind_flags;
	buffer_desc.ByteWidth = data_size * data_count;

	D3D11_SUBRESOURCE_DATA resource_data_desc;
	ZeroMemory(&resource_data_desc, sizeof(D3D11_SUBRESOURCE_DATA));
	resource_data_desc.pSysMem = (void *)data;

	HR(directx11.device->CreateBuffer(&buffer_desc, &resource_data_desc, &buffer));

	return buffer;
}

inline Gpu_Buffer *make_vertex_buffer(u32 vertex_size, u32 vertex_count, void *vertex_data, D3D11_USAGE usage = D3D11_USAGE_DEFAULT)
{
	return make_gpu_buffer(usage, D3D11_BIND_VERTEX_BUFFER, vertex_size, vertex_count, vertex_data);
}

inline Gpu_Buffer *make_index_buffer(u32 index_count, u32 *index_data, D3D11_USAGE usage = D3D11_USAGE_DEFAULT)
{
	return make_gpu_buffer(usage, D3D11_BIND_INDEX_BUFFER, sizeof(u32), index_count, index_data);
}

static inline void draw_indexed_traingles(Gpu_Buffer *vertices, u32 vertex_size, const char *vertex_name, Gpu_Buffer *indices, u32 index_count)
{
	assert(vertices);
	assert(indices && (index_count > 2));

	directx11.device_context->IASetInputLayout(Input_Layout::table[vertex_name]);
	directx11.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = vertex_size;
	UINT offset = 0;
	directx11.device_context->IASetVertexBuffers(0, 1, &vertices, &stride, &offset);
	directx11.device_context->IASetIndexBuffer(indices, DXGI_FORMAT_R32_UINT, 0);

	directx11.device_context->DrawIndexed(index_count, 0, 0);
}

static inline void draw_not_indexed_traingles(Gpu_Buffer *vertices, u32 vertex_size, const char *vertex_name, u32 vertex_count)
{
	assert(vertices);

	directx11.device_context->IASetInputLayout(Input_Layout::table[vertex_name]);
	directx11.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = vertex_size;
	UINT offset = 0;
	directx11.device_context->IASetVertexBuffers(0, 1, &vertices, &stride, &offset);

	directx11.device_context->Draw(vertex_count, 0);
}

void draw_texture_on_screen(s32 x, s32 y, Texture *texture, float _width, float _height)
{
	float xpos = x;
	float ypos = y;

	float width = _width == 0.0f ? texture->width : _width;
	float height = _height == 0.0f ? texture->height : _height;

	Vertex vertices[4] = {
		Vertex(Vector3(x_to_screen_space(xpos), y_to_screen_space(ypos + height), 0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 1.0f)),
		Vertex(Vector3(x_to_screen_space(xpos), y_to_screen_space(ypos), 0.0f),  Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 0.0f)),
		Vertex(Vector3(x_to_screen_space(xpos + width), y_to_screen_space(ypos), 0.0f),Vector3(0.0f, 0.0f, 1.0f),  Vector2(1.0f, 0.0f)),
		Vertex(Vector3(x_to_screen_space(xpos + width), y_to_screen_space(ypos + height), 0.0f),Vector3(0.0f, 0.0f, 1.0f),  Vector2(1.0f, 1.0f)),
	};

	u32 indices[6] = {
		0, 1, 2,
		0, 2, 3
	};

	Fx_Shader *color = fx_shader_manager.get_shader("base");
	color->bind("texture_map", texture->shader_resource);
	color->attach("draw_texture");

	Gpu_Buffer *vertex_buffer = make_vertex_buffer(sizeof(Vertex), 4, (void *)vertices);
	Gpu_Buffer *index_buffer = make_index_buffer(6, indices);

	draw_indexed_traingles(vertex_buffer, sizeof(Vertex), "vertex", index_buffer, 6);

	free_com_object(vertex_buffer);
	free_com_object(index_buffer);
}

#include <math.h>
const float PI = 3.14;
static inline float degrees_to_radian(u32 degrees)
{
	return degrees * PI / 180.0f;
}

Vector2 quad(float t, Vector2 p0, Vector2 p1, Vector2 p2)
{
	return (float)pow((1 - t), 2) * p0 + 2 * (1 - t) * t * p1 + (float)pow(t, 2) * p2;
}

enum Rect_Side {
	RECT_SIDE_TOP_LEFT,
	RECT_SIDE_TOP_RIGHT,
	RECT_SIDE_BOTTOM_LEFT,
	RECT_SIDE_BOTTOM_RIGHT,
};

struct Primitive_2D {
	~Primitive_2D();
	
	Gpu_Buffer *vertex_buffer = NULL;
	Gpu_Buffer *index_buffer = NULL;

	Array<Vector2> points;
	Array<Vertex_XC> vertices;
	Array<u32> indices;

	void allocate_gpu_buffer();
	void add_point(const Vector2 &point) { points.push(point); }
	//void add_rounded_points(const Vector2 &point0, const Vector2 &point1, const Vector2 &point2);
	void add_rounded_points(int x, int y, int width, int height, Rect_Side rect_side, u32 rounding);
	void make_triangle_polygon(const Color &color);
};

Primitive_2D::~Primitive_2D()
{
	free_com_object(vertex_buffer);
	free_com_object(index_buffer);
}


void Primitive_2D::add_rounded_points(int x, int y, int width, int height, Rect_Side rect_side, u32 rounding)
{
	Vector2 point0, point1, point2;
	if (rect_side == RECT_SIDE_TOP_LEFT) {
		point0 = Vector2(x, y + rounding);
		point1 = Vector2(x, y);
		point2 = Vector2(x + rounding, y);
	
	} else if (rect_side == RECT_SIDE_TOP_RIGHT) {
		point0 = Vector2(x + width - rounding, y); 
		point1 = Vector2(x + width, y); 
		point2 = Vector2(x + width, y + rounding);
	
	} else if (rect_side == RECT_SIDE_BOTTOM_LEFT) {
		point0 = Vector2(x + rounding, y + height); 
		point1 = Vector2(x, y + height); 
		point2 = Vector2(x, y + height - rounding);
	
	} else if (rect_side == RECT_SIDE_BOTTOM_RIGHT) {
		point0 = Vector2(x + width, y + height - rounding);
		point1 = Vector2(x + width, y + height); 
		point2 = Vector2(x + width - rounding, y + height);
	}

	float s = 0.0f;
	for (int i = 0; i < 10; i++) {
		Vector2 point = quad(s, point0, point1, point2);
		points.push(point);
		s += 0.1f;
	}
}

void Primitive_2D::allocate_gpu_buffer()
{
	vertex_buffer = make_vertex_buffer(sizeof(Vertex_XC), vertices.count, (void *)vertices.items);
	index_buffer = make_index_buffer(indices.count, indices.items);
}

void Primitive_2D::make_triangle_polygon(const Color &color)
{
	assert(points.count > 2);

	vertices.resize(points.count);
	for (int i = 0; i < points.count; i++) {
		vertices.push(Vertex_XC(points[i], color.value));
	}

	for (int i = 2; i < points.count; i++) {
		indices.push(i);
		indices.push(i - 1);
		indices.push(0);
	}
}

struct Render_2D {
	Array<Primitive_2D *> primitives;

	void add_primitive(Primitive_2D *primitive) { primitives.push(primitive); }
	
	void draw_rect(int x, int y, int width, int height, const Color &color, u32 rounding, u32 flags);
	void draw_primitives();
};

void draw_rect(int x, int y, int width, int height, const Color &color, u32 rounding, u32 flags)
{
	//Primitive_2D *primitive = new Primitive_2D();
	Primitive_2D p;

	if (rounding > 0) {
		(flags & ROUND_TOP_LEFT_RECT) ? p.add_rounded_points(x, y, width, height, RECT_SIDE_TOP_LEFT, rounding) : p.add_point(Vector2(x, y));
		(flags & ROUND_TOP_RIGHT_RECT) ? p.add_rounded_points(x, y, width, height, RECT_SIDE_TOP_RIGHT, rounding) : p.add_point(Vector2(x + width, y));
		(flags & ROUND_BOTTOM_RIGHT_RECT) ? p.add_rounded_points(x, y, width, height, RECT_SIDE_BOTTOM_RIGHT, rounding) : p.add_point(Vector2(x + width, y + height));
		(flags & ROUND_BOTTOM_LEFT_RECT) ? p.add_rounded_points(x, y, width, height, RECT_SIDE_BOTTOM_LEFT, rounding) : p.add_point(Vector2(x, y + height));
	
	} else {
		p.add_point(Vector2(x, y));
		p.add_point(Vector2(x + width, y));
		p.add_point(Vector2(x + width, y + height));
		p.add_point(Vector2(x, y + height));
	}

	p.make_triangle_polygon(color);

	p.allocate_gpu_buffer();


	Fx_Shader *shader = fx_shader_manager.get_shader("color");
	shader->bind("world_view_projection", &render_sys.view_info->orthogonal_matrix);
	shader->attach("draw_vertex_on_screen");

	draw_indexed_traingles(p.vertex_buffer, sizeof(Vertex_XC), "vertex_color", p.index_buffer, p.indices.count);
}
