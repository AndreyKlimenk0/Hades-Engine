#include <d3dx11effect.h>
#include <D3DX11.h>

#include "base.h"
#include "effect.h"
#include "model.h"
#include "mesh.h"
#include "vertex.h"
#include "render_frame.h"

#include "../sys/sys_local.h"
#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"
#include "../libs/geometry_generator.h"

#include "../libs/fbx_loader.h"
#include "../game/entity.h"


void draw_normals(Entity *entity, float line_len)
{
	Triangle_Mesh *mesh = &entity->model->mesh;
	Array<Vertex_XC> normals;

	for (int i = 0; i < mesh->vertex_count; i++) {
		Vertex_XC position1;
		position1.position = mesh->vertices[i].position;
		position1.color = Red;
		normals.push(position1);

		Vertex_XC position2;
		position2.position = (mesh->vertices[i].position) + (mesh->vertices[i].normal * Vector3(line_len, line_len, line_len)); 
		position2.color = Red;
		normals.push(position2);
	}
	
	direct3d.device_context->IASetInputLayout(Input_Layout::vertex_color);
	direct3d.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	u32 stride = sizeof(Vertex_XC);
	u32 offset = 0;

	ID3D11Buffer *buffer = NULL;
	create_static_vertex_buffer(normals.count, stride, (void *)&normals.items[0], &buffer);

	direct3d.device_context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);

	direct3d.device_context->Draw(normals.count, 0);

	buffer->Release();
}


void draw_indexed_mesh(Triangle_Mesh *mesh)
{
	assert(mesh->index_buffer);

	direct3d.device_context->IASetInputLayout(Input_Layout::vertex);
	direct3d.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	direct3d.device_context->IASetVertexBuffers(0, 1, &mesh->vertex_buffer, &stride, &offset);
	direct3d.device_context->IASetIndexBuffer(mesh->index_buffer, DXGI_FORMAT_R32_UINT, 0);

	direct3d.device_context->DrawIndexed(mesh->index_count, 0, 0);
}

void draw_not_indexed_mesh(Triangle_Mesh *mesh)
{
	direct3d.device_context->IASetInputLayout(Input_Layout::vertex);
	direct3d.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	direct3d.device_context->IASetVertexBuffers(0, 1, &mesh->vertex_buffer, &stride, &offset);

	direct3d.device_context->Draw(mesh->vertex_count, 0);
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
	if (entity->type == ENTITY_TYPE_FLOOR || entity->type == ENTITY_TYPE_LIGHT) {
		return;
	}

	ID3D11BlendState* transparent = NULL;
	
	D3D11_BLEND_DESC transparent_desc = { 0 };
	transparent_desc.AlphaToCoverageEnable = false;
	transparent_desc.IndependentBlendEnable = false;

	transparent_desc.RenderTarget[0].BlendEnable = true;
	transparent_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	transparent_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	transparent_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	transparent_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	transparent_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	transparent_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	transparent_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	HR(direct3d.device->CreateBlendState(&transparent_desc, &transparent));

	ID3D11DepthStencilState* no_double_blending = NULL;

	D3D11_DEPTH_STENCIL_DESC no_double_blending_desc;
	no_double_blending_desc.DepthEnable = true;
	no_double_blending_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	no_double_blending_desc.DepthFunc = D3D11_COMPARISON_LESS;
	no_double_blending_desc.StencilEnable = true;
	no_double_blending_desc.StencilReadMask = 0xff;
	no_double_blending_desc.StencilWriteMask = 0xff;

	no_double_blending_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	no_double_blending_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	no_double_blending_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	no_double_blending_desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	no_double_blending_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	no_double_blending_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	no_double_blending_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	no_double_blending_desc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	HR(direct3d.device->CreateDepthStencilState(&no_double_blending_desc, &no_double_blending));

	Material shadow_material;
	shadow_material.ambient = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	shadow_material.diffuse = Vector4(0.0f, 0.0f, 0.0f, 0.5f);
	shadow_material.specular = Vector4(0.0f, 0.0f, 0.0f, 16.0f);

	XMVECTOR plane = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // xz plane
	XMVECTOR light_direction = -XMLoadFloat3((XMFLOAT3 *)&light->direction);
	Matrix4 shadow_matrix = XMMatrixShadow(plane, light_direction);
	Matrix4 offset = XMMatrixTranslation(0.0f, 1.0f, 0.0f);

	Matrix4  world = entity->get_world_matrix();
	Matrix4 shadow_plane = world * shadow_matrix * offset;
	Matrix4 world_view_perspective = shadow_plane * view * perspective;
	
	fx_shader_light->bind("world", (Matrix4 *)&shadow_plane);
	fx_shader_light->bind("world_view_projection", (Matrix4 *)&world_view_perspective);
	fx_shader_light->bind("material", (void *)&shadow_material, sizeof(Material));

	direct3d.device_context->OMSetDepthStencilState(no_double_blending, 0);
	float b[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	direct3d.device_context->OMSetBlendState(transparent, b, 0xffffffff);

	if (entity->model->render_surface_use == RENDER_MODEL_SURFACE_USE_TEXTURE) {
		fx_shader_light->attach();
	} else {
		fx_shader_light->attach(1);
	}
	draw_mesh(&entity->model->mesh);

	direct3d.device_context->OMSetBlendState(0, b, 0xffffffff);
	direct3d.device_context->OMSetDepthStencilState(0, 0);
}

void draw_entities(Entity_Manager *entity_manager, Matrix4 &view, Free_Camera *camera)
{
	Fx_Shader *light = fx_shader_manager.get_shader("forward_light");

	bind_light_entities(light, &entity_manager->lights);

	light->bind_per_frame_vars(camera);
	
	Entity * entity = NULL;
	FOR(entity_manager->entities, entity) {
		light->bind_per_entity_vars(entity, view, direct3d.perspective_matrix);

		if (entity->type == ENTITY_TYPE_LIGHT) {
			int light_model_index = 0;
			if (!entity->model) {
				continue;
			}
			light->bind("light_model_index", light_model_index++);
			light->attach("render_light_model");
			draw_mesh(&entity->model->mesh);
			continue;
		}
		
		if (entity->model->render_surface_use == RENDER_MODEL_SURFACE_USE_TEXTURE) {
			light->attach("render_model_use_texture");
		} else {
			light->attach("render_model_use_color");
		}
		draw_mesh(&entity->model->mesh);
		draw_shadow(entity, light, entity_manager->lights[0], view, direct3d.perspective_matrix);
	}
}