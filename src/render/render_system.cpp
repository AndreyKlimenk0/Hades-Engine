#include <assert.h>
#include "directx.h"
#include "render_system.h"
#include "../win32/win_local.h"
#include "../sys/sys_local.h"


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

static void draw_world_entities(World *world)
{
	Entity_Manager *entity_manager = &world->entity_manager;

	Fx_Shader *light = fx_shader_manager.get_shader("forward_light");

	bind_light(light, &entity_manager->lights);

	light->bind_per_frame_info(render_sys.free_camera);

	Render_Entity *render_entity = NULL;
	For(world->render_entities, render_entity) {

		if (render_entity->stencil_test) {
			enable_stencil_test(render_entity->stencil_test, render_entity->stencil_ref_value);
		}

		bind(light, render_entity->entity);

		Render_Mesh *render_mesh = NULL;
		For(render_entity->render_model->render_meshes, render_mesh) {
			bind(light, render_mesh);
			light->attach("draw");
			draw_mesh(&render_mesh->mesh);
		}

		if (render_entity->draw_after_drawn_entity) {
			render_entity->draw_after_drawn_entity(render_entity);
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

	view_matrix = free_camera->get_view_matrix();

	Render_Entity *render_entity = NULL;
	For(current_render_world->render_entities, render_entity)
	{
		make_outlining(render_entity);
	}

	draw_world_entities(current_render_world);

	For(current_render_world->render_entities, render_entity)
	{
		free_outlining(render_entity);
	}

	draw_texture_on_screen(0, 0, texture_manager.get_texture("Lion_Albedo.png"), 250.0f, 250.0f);

	draw_text(0.5, 0.5, "AAAAAAAAAAAAAAAAAAAAA");
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
	view_info->orthogonal_matrix = XMMatrixOrthographicLH(view_info->window_width, view_info->window_height, near_plane, far_plane);
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
	render_entity->draw_after_drawn_entity = draw_outlining;
}

void free_outlining(Render_Entity *render_entity)
{
	render_entity->stencil_ref_value = 0;
	free_com_object(render_entity->stencil_test);
	render_entity->draw_after_drawn_entity = NULL;
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

void draw_texture_on_screen(s32 x, s32 y, Texture *texture, float _width, float _height)
{
	float xpos = x;
	float ypos = y;

	float width = _width == 0.0f ? texture->width : _width;
	float height = _height == 0.0f ? texture->height : _height;

	//Vertex_XUV vertices[6] = {
	//	Vertex_XUV(Vector3(xpos, ypos + height, 1.0f), Vector2(0.0f, 0.0f)),
	//	Vertex_XUV(Vector3(xpos, ypos, 1.0f),          Vector2(0.0f, 1.0f)),
	//	Vertex_XUV(Vector3(xpos + width, ypos, 1.0f),  Vector2(1.0f, 1.0f)),

	//	Vertex_XUV(Vector3(xpos, ypos + height, 1.0f), Vector2(0.0f, 0.0f)),
	//	Vertex_XUV(Vector3(xpos + width, ypos, 1.0f),  Vector2(1.0f, 1.0f)),
	//	Vertex_XUV(Vector3(xpos + width, ypos + height, 1.0f), Vector2(1.0f, 0.0f))
	//};

	//Vertex vertices[6] = {
	//	Vertex(Vector3(xpos, ypos + height, 0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 0.0f)),
	//	Vertex(Vector3(xpos, ypos, 0.0f),  Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 1.0f)),
	//	Vertex(Vector3(xpos + width, ypos, 0.0f),Vector3(0.0f, 0.0f, 1.0f),  Vector2(1.0f, 1.0f)),

	//	Vertex(Vector3(xpos, ypos + height, 0.0f),Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 0.0f)),
	//	Vertex(Vector3(xpos + width, ypos, 0.0f),Vector3(0.0f, 0.0f, 1.0f),  Vector2(1.0f, 1.0f)),
	//	Vertex(Vector3(xpos + width, ypos + height, 0.0f),Vector3(0.0f, 0.0f, 1.0f), Vector2(1.0f, 0.0f))
	//};

	//Vertex vertices[6] = {
	//	Vertex(Vector3(xpos, ypos, 0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 0.0f)),
	//	Vertex(Vector3(xpos + width, ypos + height, 0.0f),  Vector3(0.0f, 0.0f, 1.0f), Vector2(1.0f, 0.0f)),
	//	Vertex(Vector3(xpos, ypos + height, 0.0f),Vector3(0.0f, 0.0f, 1.0f),  Vector2(1.0f, 1.0f)),
	//	Vertex(Vector3(xpos + width, ypos, 0.0f),Vector3(0.0f, 0.0f, 1.0f),  Vector2(0.0f, 1.0f)),
	//};

	//Vertex vertices[6] = {
	//	Vertex(Vector3(xpos, ypos + height, 0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 1.0f)),
	//	Vertex(Vector3(xpos, ypos, 0.0f),  Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 0.0f)),
	//	Vertex(Vector3(xpos + width, ypos, 0.0f),Vector3(0.0f, 0.0f, 1.0f),  Vector2(1.0f, 0.0f)),
	//	Vertex(Vector3(xpos + width, ypos + height, 0.0f),Vector3(0.0f, 0.0f, 1.0f),  Vector2(1.0f, 1.0f)),
	//};	
	Vertex vertices[4] = {
		Vertex(Vector3(x_to_screen_space(xpos), y_to_screen_space(ypos + height), 0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 1.0f)),
		Vertex(Vector3(x_to_screen_space(xpos), y_to_screen_space(ypos), 0.0f),  Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 0.0f)),
		Vertex(Vector3(x_to_screen_space(xpos + width), y_to_screen_space(ypos), 0.0f),Vector3(0.0f, 0.0f, 1.0f),  Vector2(1.0f, 0.0f)),
		Vertex(Vector3(x_to_screen_space(xpos + width), y_to_screen_space(ypos + height), 0.0f),Vector3(0.0f, 0.0f, 1.0f),  Vector2(1.0f, 1.0f)),
	};


	//Vertex vertices[6] = {
	//	Vertex(Vector3(xpos, ypos + height, 0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector2(1.0f, 0.0f)),
	//	Vertex(Vector3(xpos, ypos, 0.0f),  Vector3(0.0f, 0.0f, 1.0f), Vector2(1.0f, 1.0f)),
	//	Vertex(Vector3(xpos + width, ypos, 0.0f),Vector3(0.0f, 0.0f, 1.0f),  Vector2(0.0f, 1.0f)),
	//	Vertex(Vector3(xpos + width, ypos + height, 0.0f),Vector3(0.0f, 0.0f, 1.0f),  Vector2(0.0f, 0.0f)),
	//};

	//u32 indices[6] = {
	//	0, 1, 2,
	//	0, 3, 1
	//};
	u32 indices[6] = {
		0, 1, 2,
		0, 2, 3
	};	
	//u32 indices[6] = {
	//	0, 2, 1,
	//	3, 2, 1
	//};

	//Vertex vertices[6] = {
	//	Vertex(Vector3(x_to_screen_space(xpos), y_to_screen_space(ypos + height), 0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 0.0f)),
	//	Vertex(Vector3(x_to_screen_space(xpos), y_to_screen_space(ypos), 0.0f),  Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 1.0f)),
	//	Vertex(Vector3(x_to_screen_space(xpos + width), y_to_screen_space(ypos), 0.0f),Vector3(0.0f, 0.0f, 1.0f),  Vector2(1.0f, 1.0f)),

	//	Vertex(Vector3(x_to_screen_space(xpos), y_to_screen_space(ypos + height), 0.0f),Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 0.0f)),
	//	Vertex(Vector3(x_to_screen_space(xpos + width), y_to_screen_space(ypos), 0.0f),Vector3(0.0f, 0.0f, 1.0f),  Vector2(1.0f, 1.0f)),
	//	Vertex(Vector3(x_to_screen_space(xpos + width), y_to_screen_space(ypos + height), 0.0f),Vector3(0.0f, 0.0f, 1.0f), Vector2(1.0f, 0.0f))
	//};


	//Vertex vertices[4] = {
	//	Vertex(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f)),
	//	Vertex(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f)),
	//	Vertex(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f)),
	//	Vertex(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f)),
	//};
	ID3D11RasterizerState *WireFrame;
	D3D11_RASTERIZER_DESC wfdesc;
	ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
	wfdesc.FillMode = D3D11_FILL_SOLID;
	wfdesc.CullMode = D3D11_CULL_FRONT;
	//wfdesc.FrontCounterClockwise = false;
	directx11.device->CreateRasterizerState(&wfdesc, &WireFrame);
	//directx11.device_context->RSSetState(WireFrame);

	//		D3D11_DEPTH_STENCIL_DESC depth_stencil_desc;
	//ZeroMemory(&depth_stencil_desc, sizeof(D3D11_DEPTH_STENCILOP_DESC));
	//	depth_stencil_desc.DepthEnable = false;
	//	depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	//	depth_stencil_desc.DepthFunc = D3D11_COMPARISON_ALWAYS;

	//	Stencil_Test *stencil_state;
	//HR(directx11.device->CreateDepthStencilState(&depth_stencil_desc, &stencil_state));

	//directx11.device_context->OMSetDepthStencilState(stencil_state, 0);

	Fx_Shader *color = fx_shader_manager.get_shader("base");

	Matrix4 v = render_sys.view_matrix;
	//Matrix4 m =  render_sys.view_info->orthogonal_matrix;
	Matrix4 m;
	m.indentity();
	//Matrix4 m = render_sys.view_info->orthogonal_matrix;
	color->bind("world_view_projection", &m);
	color->bind("texture_map", texture->shader_resource);

	color->attach("draw_texture");

	ID3D11Buffer *vertex_buffer = NULL;
	ID3D11Buffer *index_buffer = NULL;
	
	D3D11_BUFFER_DESC vertex_buffer_desc;
	ZeroMemory(&vertex_buffer_desc, sizeof(D3D11_BUFFER_DESC));
	vertex_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertex_buffer_desc.ByteWidth = sizeof(Vertex) * 4;

	D3D11_SUBRESOURCE_DATA vertex_resource_data;
	ZeroMemory(&vertex_resource_data, sizeof(D3D11_SUBRESOURCE_DATA));
	vertex_resource_data.pSysMem = (void *)vertices;

	HR(directx11.device->CreateBuffer(&vertex_buffer_desc, &vertex_resource_data, &vertex_buffer));

	D3D11_BUFFER_DESC index_buffer_desc;
	ZeroMemory(&index_buffer_desc, sizeof(D3D11_BUFFER_DESC));
	index_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	index_buffer_desc.ByteWidth = sizeof(u32) * 6;


	D3D11_SUBRESOURCE_DATA index_data;
	ZeroMemory(&index_data, sizeof(D3D11_SUBRESOURCE_DATA));
	index_data.pSysMem = (void *)indices;
	
	HR(directx11.device->CreateBuffer(&index_buffer_desc, &index_data, &index_buffer));

	directx11.device_context->IASetInputLayout(Input_Layout::vertex);
	directx11.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	directx11.device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
	directx11.device_context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R32_UINT, 0);

	directx11.device_context->DrawIndexed(6, 0, 0);

	free_com_object(vertex_buffer);
}