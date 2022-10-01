#include <assert.h>

#include "font.h"
#include "directx.h"
#include "render_system.h"
#include "../sys/sys_local.h"
#include "../win32/win_local.h"
#include "../libs/math/common.h"


Render_System render_sys;


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

struct CB_Light {
	Vector4 position;
	Vector4 direction;
	Vector4 color;
	u32 light_type;
	float radius;
	float range;
	float pad;
};

struct World_Info {
	Vector3 camera_position;
	int pad;
	Vector3 camera_direction;
	int pad2;
	u32 light_count;
	Vector3 pad3;
};

struct Entity_Info {
	Matrix4 world_matrix;
	Matrix4 wvp_matrix;
};


void create_structured_buffer(u32 size, u32 count, void *data, Gpu_Buffer **buffer, ID3D11ShaderResourceView **shader_resource)
{
	assert((size % 16) == 0);

	D3D11_BUFFER_DESC buffer_desc;
	ZeroMemory(&buffer_desc, sizeof(buffer_desc));
	buffer_desc.ByteWidth = (size * count);
	buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.StructureByteStride = size;

	D3D11_SUBRESOURCE_DATA resource;
	ZeroMemory(&resource, sizeof(resource));
	resource.pSysMem = data;

	HR(directx11.device->CreateBuffer(&buffer_desc, &resource, buffer));

	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_desc;
	ZeroMemory(&shader_resource_desc, sizeof(shader_resource_desc));
	shader_resource_desc.Format = DXGI_FORMAT_UNKNOWN;
	shader_resource_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	shader_resource_desc.Buffer.FirstElement = 0;
	shader_resource_desc.Buffer.NumElements = count;

	HR(directx11.device->CreateShaderResourceView(*buffer, &shader_resource_desc, shader_resource));
}

void bind_lights(Entity_Manager *entity_manager, Array<CB_Light> *lights)
{
	Light *light = NULL;
	For(entity_manager->lights, light) {
		CB_Light cb_light;
		cb_light.position = light->position;
		cb_light.direction = light->direction;
		cb_light.color = light->color;
		cb_light.radius = light->radius;
		cb_light.range = light->range;
		cb_light.light_type = light->light_type;
		lights->push(cb_light);
	}
}

void bind_world_info()
{

}


void Render_System::draw_world_entities(World *world)
{
	static Gpu_Buffer *world_info_bufffer = make_constant_buffer(sizeof(World_Info));
	static Gpu_Buffer *entity_buffer = make_constant_buffer(sizeof(Entity_Info));
	static Gpu_Buffer *material_buffer = make_constant_buffer(sizeof(Material));

	Entity_Manager *entity_manager = &world->entity_manager;

	Shader_Manager *shader_manager = render_sys.get_shader_manager();

	Shader *light = shader_manager->get_shader("forward_light");

	assert(shader_resource);

	directx11.device_context->VSSetShader(light->vertex_shader, NULL, 0);
	directx11.device_context->PSSetShader(light->pixel_shader, NULL, 0);

	directx11.device_context->PSSetSamplers(0, 1, &render_sys.sampler);
	directx11.device_context->PSSetShaderResources(1, 1, &shader_resource);

	World_Info world_info;
	world_info.camera_direction = render_sys.free_camera->target;
	world_info.camera_position = render_sys.free_camera->position;
	world_info.light_count = entity_manager->lights.count;

	update_constant_buffer(world_info_bufffer, (void *)&world_info, sizeof(World_Info));
	directx11.device_context->PSSetConstantBuffers(2, 1, &world_info_bufffer);

	Render_Entity *render_entity = NULL;
	For(world->render_entities, render_entity) {

		if (render_entity->stencil_test) {
			//enable_stencil_test(render_entity->stencil_test, render_entity->stencil_ref_value);
		}

		if (render_entity->call_before_drawing_entity) {
			render_entity->call_before_drawing_entity(render_entity);
		}

		Entity_Info entity_info;
		entity_info.wvp_matrix = make_world_view_perspective_matrix(render_entity->entity);
		entity_info.world_matrix = render_entity->entity->get_world_matrix();
		update_constant_buffer(entity_buffer, (void *)&entity_info, sizeof(Entity_Info));
		
		directx11.device_context->VSSetConstantBuffers(1, 1, &entity_buffer);

		Render_Mesh *render_mesh = NULL;
		For(render_entity->render_model->render_meshes, render_mesh) {

			update_constant_buffer(material_buffer, (void *)&render_mesh->material, sizeof(Material));
			directx11.device_context->PSSetConstantBuffers(3, 1, &material_buffer);
			directx11.device_context->PSSetShaderResources(0, 1, &render_mesh->diffuse_texture->shader_resource);
			//directx11.device_context->PSSetShaderResources(1, 1, &shader_resource);
			
			draw_mesh(&render_mesh->mesh);
		}

		if (render_entity->call_after_drawn_entity) {
			render_entity->call_after_drawn_entity(render_entity);
		}

		if (render_entity->stencil_test) {
			//disable_stencil_test();
		}
	}

	//free_com_object(shader_resource);
	//free_com_object(world_info_bufffer);
}

Render_System::~Render_System()
{
	shutdown();
}

void Render_System::init(View_Info *view_info)
{
	this->view_info = view_info;

	shader_manager.init();
	render_2d.init();

	D3D11_SAMPLER_DESC sampler_desc;
	ZeroMemory(&sampler_desc, sizeof(sampler_desc));
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

	HR(directx11.device->CreateSamplerState(&sampler_desc, &sampler));

	Gpu_Buffer *buffer = NULL;
	Entity_Manager *entity_manager = &world.entity_manager;
	Array<CB_Light> cb_light;
	bind_lights(entity_manager, &cb_light);
	create_structured_buffer(sizeof(CB_Light), cb_light.count, (void *)&cb_light.items[0], &buffer, &shader_resource);
}

void Render_System::shutdown()
{
	DELETE_PTR(view_info);
}

void Render_System::resize()
{
	if (view_info) { 
		view_info->update_projection_matries(win32.window_width, win32.window_height); 
	}
}

#include "font.h"

#include "../editor/editor.h"

struct rect_t {
	rect_t() {};
	rect_t(int x, int y, int w, int h, const Color &c) : x(x), y(y), width(w), height(h), color(c) {}
	int x;
	int y;
	int width;
	int height;
	Color color;
};

static int compare_rects(const void *first_rect, const void *second_rect)
{
	assert(first_rect);
	assert(second_rect);

	const rect_t *first = static_cast<const rect_t *>(first_rect);
	const rect_t *second = static_cast<const rect_t *>(second_rect);

	return first->height > second->height;
}

void pack_rects(rect_t *main_rect, Array<rect_t> *rects)
{
	qsort(rects->items, rects->count, sizeof((*rects)[0]), compare_rects);

	u32 x_pos = 0;
	u32 y_pos = 0;
	u32 large = 0;

	rect_t *rect = NULL;
	For((*rects), rect) {

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

static int compare_rects_u32(const void *first_rect, const void *second_rect)
{
	assert(first_rect);
	assert(second_rect);

	const Rect_u32 *first = static_cast<const Rect_u32 *>(first_rect);
	const Rect_u32 *second = static_cast<const Rect_u32 *>(second_rect);

	return first->height > second->height;
}

void pack_rects(Rect_u32 *main_rect, Array<Rect_u32 *> &rects)
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

#include "../gui/gui.h"

void Render_System::render_frame()
{
	//Array<rect_t> rects;
	//rects.push(rect_t(0, 0, 50, 110, Color::Silver));
	//rects.push(rect_t(50, 0, 70, 120, Color::Red));
	//rects.push(rect_t(0, 100, 50, 100, Color::Blue));
	//rects.push(rect_t(200, 200, 50, 100, Color::Silver));
	//rects.push(rect_t(300, 0, 150, 177, Color::LightSteelBlue));
	//rects.push(rect_t(500, 0, 400, 120, Color::Magenta));
	//rects.push(rect_t(500, 0, 150, 130, Color::Cyan));
	//rects.push(rect_t(400, 0, 50, 100, Color::Yellow));
	//rects.push(rect_t(0, 200, 220, 120, Color::Red));
	//rects.push(rect_t(0, 300, 120, 100, Color::Blue));
	//rects.push(rect_t(0, 400, 320, 150, Color::Blue));
	//rects.push(rect_t(0, 500, 220, 122, Color::Green));
	//rects.push(rect_t(0, 600, 150, 233, Color::Green));
	//rects.push(rect_t(700, 700, 50, 154, Color::Magenta));
	//rects.push(rect_t(200, 300, 50, 122, Color::Silver));
	//rects.push(rect_t(0, 500, 220, 122, Color::Green));
	//rects.push(rect_t(0, 600, 150, 233, Color::Green));
	//rects.push(rect_t(700, 700, 50, 154, Color::Magenta));
	//rects.push(rect_t(200, 300, 50, 122, Color::Silver));
	//rects.push(rect_t(0, 0, 50, 110, Color::Silver));
	//rects.push(rect_t(50, 0, 70, 120, Color::Red));
	//rects.push(rect_t(0, 100, 50, 100, Color::Blue));
	//rects.push(rect_t(200, 200, 50, 100, Color::Silver));

	//rect_t r = rect_t(0, 0, 900, 900, Color::Red);
	//pack_rects(&r, &rects);

	render_2d.new_frame();


	//render_2d.draw_texture(0, 0, 500, 500, render_2d.temp);
//	render_2d.draw_texture(600, 0, 400, 400, &render_2d.font_atlas);
	//const char *text = "ANDREYKLIENMKOQWEZ";
	//Size_u32 s = font.get_text_size(text);
	//render_2d.draw_rect(10, 10, s.width, s.height, Color::Red);
	//render_2d.draw_text(10, 10, text);

	//render_2d.draw_rect(0, 0, 900, 300, Color::Black);
	//render_2d.draw_rect(0, 400, 200, 200, Color::Red, 50);
	
	//rect_t *rect = NULL;
	//For(rects, rect) {
	//	render_2d.draw_rect(rect->x, rect->y, rect->width, rect->height, rect->color);
	//}

	//view_matrix = free_camera->get_view_matrix();
	//draw_world_entities(current_render_world);
	
	//editor.draw();

	draw_test_gui();

	//render_2d.new_render_primitive_list();
	//render_2d.draw_outlines(100, 100, 200, 300, Color(92, 100, 107), 10.0f);
	
	//render_2d.new_render_primitive_list();
	//render_2d.draw_rect(100, 100, 100, 100, Color::Red, 120, ROUND_TOP_RIGHT_RECT | ROUND_BOTTOM_RIGHT_RECT);
	//render_2d.draw_rect(100, 300, 100, 100, Color::Red, 120);


	render_2d.render_frame();
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
	view_info->orthogonal_matrix = XMMatrixOrthographicOffCenterLH(0.0f, (float)view_info->window_width, (float)view_info->window_height, 0.0f, near_plane, far_plane);

	return view_info;
}

inline Stencil_Test *make_outlining_stencil_test()
{
	return make_stecil_test(D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_REPLACE, D3D11_COMPARISON_ALWAYS);
}

void draw_outlining(Render_Entity *render_entity)
{
	//Fx_Shader *color = fx_shader_manager.get_shader("base");

	//Matrix4 world = render_entity->entity->get_world_matrix();
	//Matrix4 wvp_projection = make_world_view_perspective_matrix(render_entity->entity);

	//color->bind("world", &world);
	//color->bind("world_view_projection", &wvp_projection);

	//color->attach("draw_outlining");

	//Stencil_Test *second_test = make_stecil_test(D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_REPLACE, D3D11_COMPARISON_NOT_EQUAL, 0x0, 0xff);
	//enable_stencil_test(second_test, render_entity->entity->id);

	//draw_mesh(render_entity->render_model->get_triangle_mesh());

	//disable_stencil_test();
	//free_com_object(second_test);
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

Gpu_Buffer *make_gpu_buffer(u32 data_size, u32 data_count, void *data, D3D11_USAGE usage, u32 bind_flags, u32 cpu_access)
{
	Gpu_Buffer *buffer = NULL;

	D3D11_BUFFER_DESC buffer_desc;
	ZeroMemory(&buffer_desc, sizeof(D3D11_BUFFER_DESC));
	buffer_desc.Usage = usage;
	buffer_desc.BindFlags = bind_flags;
	buffer_desc.ByteWidth = data_size * data_count;
	buffer_desc.CPUAccessFlags = cpu_access;

	if (data) {
		D3D11_SUBRESOURCE_DATA resource_data_desc;
		ZeroMemory(&resource_data_desc, sizeof(D3D11_SUBRESOURCE_DATA));
		resource_data_desc.pSysMem = (void *)data;
		HR(directx11.device->CreateBuffer(&buffer_desc, &resource_data_desc, &buffer));
	} else {
		HR(directx11.device->CreateBuffer(&buffer_desc, NULL, &buffer));
	}

	return buffer;
}

void update_constant_buffer(Gpu_Buffer *buffer, void *data, u32 data_size)
{
assert(buffer);
assert(data);

D3D11_MAPPED_SUBRESOURCE mapped_resource;
ZeroMemory(&mapped_resource, sizeof(D3D11_MAPPED_SUBRESOURCE));

HR(directx11.device_context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource));

memcpy(mapped_resource.pData, data, data_size);

directx11.device_context->Unmap(buffer, 0);
}

inline void draw_indexed_traingles(Gpu_Buffer *vertices, u32 vertex_size, const char *vertex_name, Gpu_Buffer *indices, u32 index_count, u32 vertex_offset = 0, u32 index_offset = 0)
{
	assert(vertices);
	assert(indices && (index_count > 2));

	directx11.device_context->IASetInputLayout(Input_Layout::table[vertex_name]);
	directx11.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = vertex_size;
	UINT offset = 0;
	directx11.device_context->IASetVertexBuffers(0, 1, &vertices, &stride, &offset);
	directx11.device_context->IASetIndexBuffer(indices, DXGI_FORMAT_R32_UINT, 0);

	directx11.device_context->DrawIndexed(index_count, index_offset, vertex_offset);
}

inline void draw_not_indexed_traingles(Gpu_Buffer *vertices, u32 vertex_size, const char *vertex_name, u32 vertex_count)
{
	assert(vertices);

	directx11.device_context->IASetInputLayout(Input_Layout::table[vertex_name]);
	directx11.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = vertex_size;
	UINT offset = 0;
	directx11.device_context->IASetVertexBuffers(0, 1, &vertices, &stride, &offset);

	directx11.device_context->Draw(vertex_count, 0);
}

#include <math.h>
const float PI = 3.14;
static inline float degrees_to_radian(u32 degrees)
{
	return degrees * PI / 180.0f;
}

Vector2 quad(float t, Vector2 p0, Vector2 p1, Vector2 p2)
{
	return (float)pow((1.0f - t), 2.0f) * p0 + 2.0f * (1.0f - t) * t * p1 + (float)pow(t, 2.0f) * p2;
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

	u32 i = 0;
	s32 points_count_in_rounding = 20;
	s32 temp = 0;
	float point_count = 1.0f / (float)points_count_in_rounding;
	float point_position = 0.0f;
	int l = points_count_in_rounding;

	if (x_rounding >= width) {
		float r = (width / 2.0f) / x_rounding;
		temp = (float)points_count_in_rounding * r;
		s32 a = temp;
		temp = math::abs(points_count_in_rounding - temp - 2);
		//if ((temp != 0) && (rect_side == RECT_SIDE_LEFT_TOP) || (RECT_SIDE_RIGHT_TOP)) {
		//if ((rect_side == RECT_SIDE_LEFT_TOP) || (rect_side == RECT_SIDE_LEFT_BOTTOM)) {
		//if ((rect_side == RECT_SIDE_RIGHT_TOP) || (rect_side == RECT_SIDE_RIGHT_BOTTOM)) {
		//if ((rect_side == RECT_SIDE_RIGHT_TOP)) {
		if ((rect_side == RECT_SIDE_LEFT_BOTTOM) || (rect_side == RECT_SIDE_RIGHT_TOP)) {
			//point_position = point_count * math::abs(points_count_in_rounding - temp);
			point_position = point_count * a;
		}

		if (temp != 0) {
			l = temp;
		}
	}

	for (; i <= l; i++) {
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
	indices.push(0);
	indices.push(vertices.count - 1);
	indices.push(1);
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

Render_2D::~Render_2D()
{
	free_com_object(depth_test);
	free_com_object(constant_buffer);
	free_com_object(vertex_buffer);
	free_com_object(index_buffer);
	new_frame();
}

struct CB_Render_2d_Info {
	Matrix4 position_orthographic_matrix;
	Vector4 color;
};

void Render_2D::add_render_primitive_list(Render_Primitive_List *render_primitive_list)
{
	draw_list.push(render_primitive_list);
}

void Render_2D::init()
{
	D3D11_DEPTH_STENCIL_DESC depth_test_desc;
	ZeroMemory(&depth_test_desc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	depth_test_desc.DepthEnable = false;
	depth_test_desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	depth_test_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depth_test_desc.StencilEnable = false;

	HR(directx11.device->CreateDepthStencilState(&depth_test_desc, &depth_test));

	D3D11_BLEND_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.AlphaToCoverageEnable = false;
	desc.RenderTarget[0].BlendEnable = true;
	desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	
	HR(directx11.device->CreateBlendState(&desc, &blending_test));

	D3D11_RASTERIZER_DESC rasterizer_desc;
	rasterizer_desc.FillMode = D3D11_FILL_SOLID;
	rasterizer_desc.CullMode = D3D11_CULL_FRONT;
	rasterizer_desc.FrontCounterClockwise = true;
	rasterizer_desc.DepthBias = false;
	rasterizer_desc.DepthBiasClamp = 0;
	rasterizer_desc.SlopeScaledDepthBias = 0;
	rasterizer_desc.DepthClipEnable = true;
	rasterizer_desc.ScissorEnable = true;
	rasterizer_desc.MultisampleEnable = false;
	rasterizer_desc.AntialiasedLineEnable = false;
	
	HR(directx11.device->CreateRasterizerState(&rasterizer_desc, &rasterization));

	constant_buffer = make_constant_buffer(sizeof(CB_Render_2d_Info));
	constant_buffer = make_constant_buffer(sizeof(CB_Render_2d_Info));
	init_font_rendering();

	default_texture.init(100, 100);
	default_texture.set_color(Color::White);
}

void Render_2D::init_font_rendering()
{
	Hash_Table<char, Rect_f32> uvs;
	init_font_atlas(&font, &uvs);

	for (char c = 32; c < 127; c++) {

		if (font.characters.key_in_table(c)) {
			Font_Char &font_char = font.characters[c];
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
	font_atlas.init(200, 200);
	font_atlas.set_color(Color::White);

	Array<Rect_u32 *> rects;
	Hash_Table<char, Rect_u32> table;

	for (u32 i = 32; i < 127; i++) {
		if (font->characters.key_in_table((char)i)) {
			Font_Char &font_char = font->characters[(char)i];
			table.set((char)i, Rect_u32(font_char.bitmap_size));
			rects.push(&table[(char)i]);
		}
	}
	Rect_u32 main_rect = Rect_u32(font_atlas.width, font_atlas.height);
	pack_rects(&main_rect, rects);

	for (u32 i = 32; i < 127; i++) {
		if (font->characters.key_in_table((char)i)) {
			Font_Char &character = font->characters[(char)i];
			
			Rect_u32 &rect = table[(char)i];

			Rect_f32 uv;
			uv.x = static_cast<float>(rect.x) / static_cast<float>(font_atlas.width);
			uv.y = static_cast<float>(rect.y) / static_cast<float>(font_atlas.height);
			uv.width = static_cast<float>(rect.width) / static_cast<float>(font_atlas.width);
			uv.height = static_cast<float>(rect.height) / static_cast<float>(font_atlas.height);

			font_uvs->set((char)i, uv);

			
			font_atlas.update(&rect, character.bitmap, sizeof(u32) * character.size.width);
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

void Render_2D::render_frame()
{
	if (total_vertex_count == 0) {
		return;
	}

	static u32 privious_total_vertex_count;

	if (!vertex_buffer || (privious_total_vertex_count != total_vertex_count)) {
		privious_total_vertex_count = total_vertex_count;

		free_com_object(vertex_buffer);
		free_com_object(index_buffer);

		vertex_buffer = make_vertex_buffer(sizeof(Vertex_X2UV), total_vertex_count, NULL, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		index_buffer = make_index_buffer(total_index_count, NULL, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

		D3D11_MAPPED_SUBRESOURCE buffer;
		D3D11_MAPPED_SUBRESOURCE i_buffer;
		
		ZeroMemory(&buffer, sizeof(D3D11_MAPPED_SUBRESOURCE));
		ZeroMemory(&i_buffer, sizeof(D3D11_MAPPED_SUBRESOURCE));

		HR(directx11.device_context->Map(vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &buffer));
		HR(directx11.device_context->Map(index_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &i_buffer));

		Vertex_X2UV *p1 = (Vertex_X2UV *)buffer.pData;
		u32 *p2 = (u32 *)i_buffer.pData;

		Primitive_2D *primitive = NULL;
		For(primitives, primitive) {

			memcpy((void *)p1, primitive->vertices.items, primitive->vertices.count * sizeof(Vertex_X2UV));
			memcpy((void *)p2, primitive->indices.items, primitive->indices.count * sizeof(u32));
			p1 += primitive->vertices.count;
			p2 += primitive->indices.count;
		}
		
		directx11.device_context->Unmap(index_buffer, 0);
		directx11.device_context->Unmap(vertex_buffer, 0);
	}

	CB_Render_2d_Info cb_render_info;
	
	Shader_Manager *sm = render_sys.get_shader_manager();
	Shader *render_2d = sm->get_shader("render_2d");

	directx11.device_context->IASetInputLayout(Input_Layout::table["vertex_xuv"]);
	directx11.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex_X2UV);
	UINT offset = 0;
	directx11.device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
	directx11.device_context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R32_UINT, 0);

	directx11.device_context->VSSetShader(render_2d->vertex_shader, NULL, 0);
	directx11.device_context->PSSetShader(render_2d->pixel_shader, NULL, 0);

	float b[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	directx11.device_context->OMSetBlendState(blending_test, b, 0xffffffff);

	directx11.device_context->OMSetDepthStencilState(depth_test, 0);

	directx11.device_context->RSSetState(rasterization);


	Render_Primitive_List *list = NULL;
	//For(draw_list, list) {
	for (int i = 0; i < draw_list.count; i++) {
		list = draw_list[i];
		Render_Primitive_2D *render_primitive = NULL;
		for (int j = 0; j < list->render_primitives.count; j++) {
			render_primitive = &list->render_primitives[j];
		//For((*list), render_primitive) {
			//Rect_s32 clip_rect = clip_rects[index++];

			//D3D11_RECT rects[1];
			//rects[0].left = clip_rect.x;
			//rects[0].right = clip_rect.right();
			//rects[0].top = clip_rect.y;
			//rects[0].bottom = clip_rect.bottom();

			D3D11_RECT rects[1];
			rects[0].left = render_primitive->clip_rect.x;
			rects[0].right = render_primitive->clip_rect.right();
			rects[0].top = render_primitive->clip_rect.y;
			rects[0].bottom = render_primitive->clip_rect.bottom();

			directx11.device_context->RSSetScissorRects(1, rects);

			screen_postion.translate(&render_primitive->position);

			cb_render_info.position_orthographic_matrix = screen_postion * render_sys.view_info->orthogonal_matrix;
			cb_render_info.color = render_primitive->color.value;
			update_constant_buffer(constant_buffer, (void *)&cb_render_info, sizeof(CB_Render_2d_Info));

			directx11.device_context->VSSetConstantBuffers(0, 1, &constant_buffer);
			directx11.device_context->PSSetConstantBuffers(0, 1, &constant_buffer);
			if (render_primitive->texture) {
				directx11.device_context->PSSetShaderResources(0, 1, &render_primitive->texture->shader_resource);
			}
			Primitive_2D *primitive = render_primitive->primitive;
			directx11.device_context->DrawIndexed(primitive->indices.count, primitive->index_offset, primitive->vertex_offset);
		}
	}

	directx11.device_context->OMSetBlendState(0, b, 0);
	directx11.device_context->RSSetState(0);
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
		rect->width = win32.window_width;
		rect->height = win32.window_height;
	}
}

void Render_Primitive_List::add_outlines(int x, int y, int width, int height, const Color & color, float outline_width, u32 rounding, u32 flags)
{
	String hash = String((int)width) + String((int)height) + String((int)outline_width) + String("outline");

	Render_Primitive_2D render_primitive;
	render_primitive.position.x = x;
	render_primitive.position.y = y;
	render_primitive.color.value = color.value;
	render_primitive.texture = &render_2d->default_texture;
	get_clip_rect(&render_primitive.clip_rect);

	Primitive_2D *found_primitive = NULL;
	if (render_2d->lookup_table.get(hash, found_primitive)) {
		render_primitive.primitive = found_primitive;
		render_primitives.push(render_primitive);
		return;
	}

	Primitive_2D *primitive = new Primitive_2D;
	render_2d->lookup_table.set(hash, primitive);
	render_primitive.primitive = primitive;
	render_primitives.push(render_primitive);

	if (rounding > 0) {
		(flags & ROUND_TOP_LEFT_RECT) ? primitive->add_rounded_points(-outline_width, -outline_width, width + outline_width * 2, height + outline_width * 2, RECT_SIDE_LEFT_TOP, rounding) : primitive->add_point(Vector2(outline_width, outline_width));
		(flags & ROUND_TOP_RIGHT_RECT) ? primitive->add_rounded_points(-outline_width, -outline_width, width + outline_width * 2, height + outline_width * 2, RECT_SIDE_RIGHT_TOP, rounding) : primitive->add_point(Vector2((float)width - outline_width, outline_width));
		(flags & ROUND_BOTTOM_RIGHT_RECT) ? primitive->add_rounded_points(-outline_width, -outline_width, width + outline_width * 2, height + outline_width * 2, RECT_SIDE_RIGHT_BOTTOM, rounding) : primitive->add_point(Vector2((float)width - outline_width, (float)height - outline_width));
		(flags & ROUND_BOTTOM_LEFT_RECT) ? primitive->add_rounded_points(-outline_width, -outline_width, width + outline_width * 2, height + outline_width * 2, RECT_SIDE_LEFT_BOTTOM, rounding) : primitive->add_point(Vector2(outline_width, (float)height - outline_width));

		(flags & ROUND_TOP_LEFT_RECT) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_LEFT_TOP, rounding) : primitive->add_point(Vector2(0.0f, 0.0f));
		(flags & ROUND_TOP_RIGHT_RECT) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_RIGHT_TOP, rounding) : primitive->add_point(Vector2(width, 0.0f));
		(flags & ROUND_BOTTOM_RIGHT_RECT) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_RIGHT_BOTTOM, rounding) : primitive->add_point(Vector2(width, height));
		(flags & ROUND_BOTTOM_LEFT_RECT) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_LEFT_BOTTOM, rounding) : primitive->add_point(Vector2(0.0f, height));
	} else {
		primitive->add_point(Vector2((float)-outline_width, (float)-outline_width));
		primitive->add_point(Vector2((float)width + outline_width, (float)-outline_width));
		primitive->add_point(Vector2((float)width + outline_width, (float)height + outline_width));
		primitive->add_point(Vector2(-outline_width, (float)height + outline_width));

		primitive->add_point(Vector2(0.0f, 0.0f));
		primitive->add_point(Vector2((float)width, 0.0f));
		primitive->add_point(Vector2((float)width, (float)height));
		primitive->add_point(Vector2(0.0f, (float)height));
	}

	primitive->make_outline_triangle_polygons();
	render_2d->add_primitive(primitive);
}

void Render_Primitive_List::add_text(Rect_s32 *rect, const char *text)
{
	add_text((int)rect->x, (int)rect->y, text);
}

void Render_Primitive_List::add_text(int x, int y, const char *text)
{
	u32 len = strlen(text);
	u32 max_height = font.get_text_size(text).height;

	for (u32 i = 0; i < len; i++) {

		char c = text[i];
		Font_Char &font_char = font.characters[c];

		Render_Primitive_2D info;
		info.texture = &render_2d->font_atlas;
		info.position = Vector2(x + font_char.bearing.width, y + (max_height - font_char.size.height) + (font_char.size.height - font_char.bearing.height));
		info.color = Color::White;
		info.primitive = render_2d->lookup_table[String(c)];
		get_clip_rect(&info.clip_rect);

		x += (font_char.advance >> 6);

		render_primitives.push(info);
	}
}

void Render_Primitive_List::add_rect(float x, float y, float width, float height, const Color &color, u32 rounding, u32 flags)
{
	//////////////////////////////////////////////////////////
	// &Note I am not sure that chache works for primitives //
	//////////////////////////////////////////////////////////
	String hash = String((int)width) + String((int)height) + String((int)rounding) + String((int)flags);

	Render_Primitive_2D render_primitive;
	render_primitive.position.x = x;
	render_primitive.position.y = y;
	render_primitive.color.value = color.value;
	render_primitive.texture = &render_2d->default_texture;
	get_clip_rect(&render_primitive.clip_rect);

	Primitive_2D *found_primitive = NULL;
	if (render_2d->lookup_table.get(hash, found_primitive)) {
		render_primitive.primitive = found_primitive;
		render_primitives.push(render_primitive);
		return;
	}

	Primitive_2D *primitive = new Primitive_2D;
	render_2d->lookup_table.set(hash, primitive);

	render_primitive.primitive = primitive;
	render_primitives.push(render_primitive);

	primitive->add_point(Vector2(width / 2.0f, height / 2.0f));

	float x_divisor = (!(flags & ROUND_LEFT_RECT) || !(flags & ROUND_RIGHT_RECT)) ? 1.0f : 2.0f;
	float y_divisor = (!(flags & ROUND_TOP_RECT) || !(flags & ROUND_BOTTOM_RECT)) ? 1.0f : 2.0f;

	//float x_rounding = math::min((float)rounding, (width / x_divisor));
	//float y_rounding = math::min((float)rounding, (height / y_divisor));
	float x_rounding = rounding;
	float y_rounding = rounding;

	if (rounding > 0) {
		(flags & ROUND_TOP_LEFT_RECT) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_LEFT_TOP, x_rounding, y_rounding) : primitive->add_point(Vector2(0.0f, 0.0f));
		(flags & ROUND_TOP_RIGHT_RECT) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_RIGHT_TOP, x_rounding, y_rounding) : primitive->add_point(Vector2(width, 0.0f));
		(flags & ROUND_BOTTOM_RIGHT_RECT) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_RIGHT_BOTTOM, x_rounding, y_rounding) : primitive->add_point(Vector2(width, height));
		(flags & ROUND_BOTTOM_LEFT_RECT) ? primitive->add_rounded_points(0.0f, 0.0f, width, height, RECT_SIDE_LEFT_BOTTOM, x_rounding, y_rounding) : primitive->add_point(Vector2(0.0f, height));
	} else {
		primitive->add_point(Vector2(0.0f, 0.0f));
		primitive->add_point(Vector2(width, 0.0f));
		primitive->add_point(Vector2(width, height));
		primitive->add_point(Vector2(0.0f, height));
	}

	primitive->make_triangle_polygon();
	render_2d->add_primitive(primitive);
}

void Render_Primitive_List::add_texture(int x, int y, int width, int height, Texture * texture)
{
	String hash = String(width + height);

	Render_Primitive_2D render_primitive;
	render_primitive.position.x = (float)x;
	render_primitive.position.y = (float)y;
	render_primitive.color = Color::White;
	render_primitive.texture = texture;

	Primitive_2D *found_primitive = NULL;
	if (render_2d->lookup_table.get(hash, found_primitive)) {
		render_primitive.primitive = found_primitive;
		render_primitives.push(render_primitive);
		return;
	}

	Primitive_2D *primitive = new Primitive_2D;
	render_2d->lookup_table.set(hash, primitive);
	render_primitive.primitive = primitive;
	render_primitives.push(render_primitive);

	primitive->add_point(Vector2(0.0f, 0.0f), Vector2(0.0f, 0.0f));
	primitive->add_point(Vector2((float)width, 0.0f), Vector2(1.0f, 0.0f));
	primitive->add_point(Vector2((float)width, (float)height), Vector2(1.0f, 1.0f));
	primitive->add_point(Vector2(0.0f, (float)height), Vector2(0.0f, 1.0f));

	primitive->make_triangle_polygon();
	render_2d->add_primitive(primitive);
}
