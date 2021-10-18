#include <assert.h>
#include "directx.h"
#include "render_system.h"
#include "../win32/win_local.h"
#include "../sys/sys_local.h"


Render_System render_sys;


enum Engine_Mode {
	GAME_MODE,
	EDITOR_MODE
};

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

void Render_System::render_frame()
{

	view_matrix = free_camera->get_view_matrix();
	
	draw_world_entities(&current_render_world->entity_manager);
}

void draw_meshes(Render_Model *render_model, Fx_Shader *light)
{
	//Fx_Shader *light = fx_shader_manager.get_shader("forward_light");
	Render_Mesh *render_mesh = NULL;
	
	if (render_model->is_single_mesh_model()) {
		render_mesh = render_model->get_render_mesh();
		light->bind("texture_map", render_mesh->diffuse_texture->shader_resource);
		light->bind("material", (void *)&render_mesh->material, sizeof(Material));
		draw_mesh(render_model->get_triangle_mesh());
	} else {
		For(render_model->render_meshes, render_mesh) {
			render_mesh->material.specular = Vector4(1, 1, 1, render_mesh->material.specular.w);
			light->bind("texture_map", render_mesh->diffuse_texture->shader_resource);
			light->bind("material", (void *)&render_mesh->material, sizeof(Material));
			draw_mesh(&render_mesh->mesh);
		}
	}
}

void make_world_view_perspective_matrix_for_entity(Entity *entity, Matrix4 &result)
{
	Matrix4 world;
	entity->get_world_matrix(world);
	result = world * render_sys.view_matrix * render_sys.view_info->perspective_matrix;
}

void draw_world_entities(Entity_Manager *entity_manager)
{
	//Fx_Shader *light = fx_shader_manager.get_shader("forward_light");

	//bind_light_entities(light, &entity_manager->lights);

	//light->bind_per_frame_info(render_sys.free_camera);

	//Entity * entity = NULL;
	//For(entity_manager->entities, entity) {
	//	light->bind_entity(entity, render_sys.view_matrix, render_sys.view_info->perspective_matrix);

	//	if (entity->type == ENTITY_TYPE_LIGHT) {
	//		int light_model_index = 0;
	//		if (!entity->model) {
	//			continue;
	//		}
	//		//light->bind("light_model_index", light_model_index++);
	//		//light->attach("render_light_model");
	//		//draw_meshes(entity->model);
	//		//draw_mesh(&entity->model->mesh);
	//		continue;
	//	}

	//	draw_meshes(entity->model, light);
	//	light->attach("render_model_use_texture");
	//	//draw_shadow(entity, light, entity_manager->lights[0], view_matrix, view_info->perspective_matrix);
	//}

	Fx_Shader *light = fx_shader_manager.get_shader("forward_light");

	bind_light_entities(light, &entity_manager->lights);

	light->bind_per_frame_info(render_sys.free_camera);

	Entity * entity = NULL;
	For(entity_manager->entities, entity) {

		if (entity->type == ENTITY_TYPE_LIGHT) {
				int light_model_index = 0;
				if (!entity->model) {
					continue;
				}
				light->bind("light_model_index", light_model_index++);
				light->attach("render_light_model");
				draw_mesh(entity->model->get_triangle_mesh());
				continue;
			}

		Render_Mesh *render_mesh = NULL;
		For(entity->model->render_meshes, render_mesh) {

			light->bind_entity(entity, render_sys.view_matrix, render_sys.view_info->perspective_matrix, render_mesh);

			if (1) {
				light->attach("render_model_use_texture");
			} else {
				//light->attach("render_model_use_color");
			}
			draw_mesh(&render_mesh->mesh);
		}

		//light->bind_entity(entity, render_sys.view_matrix, render_sys.view_info->perspective_matrix);

		//if (entity->type == ENTITY_TYPE_LIGHT) {
		//	int light_model_index = 0;
		//	if (!entity->model) {
		//		continue;
		//	}
		//	light->bind("light_model_index", light_model_index++);
		//	light->attach("render_light_model");
		//	draw_mesh(entity->model->get_triangle_mesh());
		//	continue;
		//}

		//if (1) {
		//	light->attach("render_model_use_texture");
		//} else {
		//	//light->attach("render_model_use_color");
		//}
		//draw_mesh(entity->model->get_triangle_mesh());
		//draw_shadow(entity, light, entity_manager->lights[0], render_sys.view_matrix, render_sys.view_info->perspective_matrix);
	}
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


void draw_normals(Entity *entity, float line_len)
{
	Triangle_Mesh *mesh = entity->model->get_triangle_mesh();
	Array<Vertex_XC> normals;

	for (int i = 0; i < mesh->vertex_count; i++) {
		Vertex_XC position1;
		position1.position = mesh->vertices[i].position;
		position1.color = Vector3(1.0f, 0.0f, 0.0f);
		normals.push(position1);

		Vertex_XC position2;
		position2.position = (mesh->vertices[i].position) + (mesh->vertices[i].normal * Vector3(line_len, line_len, line_len));
		position2.color = Vector3(1.0f, 0.0f, 0.0f);
		normals.push(position2);
	}

	directx11.device_context->IASetInputLayout(Input_Layout::vertex_color);
	directx11.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	u32 stride = sizeof(Vertex_XC);
	u32 offset = 0;

	ID3D11Buffer *buffer = NULL;
	create_static_vertex_buffer(normals.count, stride, (void *)&normals.items[0], &buffer);

	directx11.device_context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);

	directx11.device_context->Draw(normals.count, 0);

	buffer->Release();
}


void draw_indexed_mesh(Triangle_Mesh *mesh)
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

void draw_not_indexed_mesh(Triangle_Mesh *mesh)
{
	directx11.device_context->IASetInputLayout(Input_Layout::vertex);
	directx11.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	directx11.device_context->IASetVertexBuffers(0, 1, &mesh->vertex_buffer, &stride, &offset);

	directx11.device_context->Draw(mesh->vertex_count, 0);
}

void draw_mesh(Triangle_Mesh *mesh)
{
	assert(mesh);
	assert(mesh->vertex_buffer);

	if (mesh->is_indexed) {
		draw_indexed_mesh(mesh);
	} else {
		draw_not_indexed_mesh(mesh);
	}

}

void draw_shadow(Entity *entity, Fx_Shader *fx_shader_light, Light *light, Matrix4 &view, Matrix4 &perspective)
{
	//if (entity->type == ENTITY_TYPE_FLOOR || entity->type == ENTITY_TYPE_LIGHT) {
	//	return;
	//}

	//ID3D11BlendState* transparent = NULL;

	//D3D11_BLEND_DESC transparent_desc = { 0 };
	//transparent_desc.AlphaToCoverageEnable = false;
	//transparent_desc.IndependentBlendEnable = false;

	//transparent_desc.RenderTarget[0].BlendEnable = true;
	//transparent_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	//transparent_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	//transparent_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	//transparent_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	//transparent_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	//transparent_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	//transparent_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	//HR(directx11.device->CreateBlendState(&transparent_desc, &transparent));

	//ID3D11DepthStencilState* no_double_blending = NULL;

	//D3D11_DEPTH_STENCIL_DESC no_double_blending_desc;
	//no_double_blending_desc.DepthEnable = true;
	//no_double_blending_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	//no_double_blending_desc.DepthFunc = D3D11_COMPARISON_LESS;
	//no_double_blending_desc.StencilEnable = true;
	//no_double_blending_desc.StencilReadMask = 0xff;
	//no_double_blending_desc.StencilWriteMask = 0xff;

	//no_double_blending_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	//no_double_blending_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	//no_double_blending_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	//no_double_blending_desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	//no_double_blending_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	//no_double_blending_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	//no_double_blending_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	//no_double_blending_desc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	//HR(directx11.device->CreateDepthStencilState(&no_double_blending_desc, &no_double_blending));

	//Material shadow_material;
	//shadow_material.ambient = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	//shadow_material.diffuse = Vector4(0.0f, 0.0f, 0.0f, 0.5f);
	//shadow_material.specular = Vector4(0.0f, 0.0f, 0.0f, 16.0f);

	//XMVECTOR plane = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // xz plane
	//XMVECTOR light_direction = -XMLoadFloat3((XMFLOAT3 *)&light->direction);
	//Matrix4 shadow_matrix = XMMatrixShadow(plane, light_direction);
	//Matrix4 offset = XMMatrixTranslation(0.0f, 1.0f, 0.0f);

	//Matrix4  world = entity->get_world_matrix();
	//Matrix4 shadow_plane = world * shadow_matrix * offset;
	//Matrix4 world_view_perspective = shadow_plane * view * perspective;

	//fx_shader_light->bind("world", (Matrix4 *)&shadow_plane);
	//fx_shader_light->bind("world_view_projection", (Matrix4 *)&world_view_perspective);
	//fx_shader_light->bind("material", (void *)&shadow_material, sizeof(Material));

	//directx11.device_context->OMSetDepthStencilState(no_double_blending, 0);
	//float b[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//directx11.device_context->OMSetBlendState(transparent, b, 0xffffffff);

	////if (entity->model->render_surface_use == RENDER_MODEL_SURFACE_USE_TEXTURE) {
	////	fx_shader_light->attach();
	////} else {
	//	fx_shader_light->attach();
	////}
	////draw_mesh(&entity->model->get_triangle_mesh());

	//directx11.device_context->OMSetBlendState(0, b, 0xffffffff);
	//directx11.device_context->OMSetDepthStencilState(0, 0);
}
