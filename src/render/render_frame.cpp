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


//void Render_World::init(Free_Camera *_camera)
//{
//	camera = _camera;
//	camera->position = Vector3(5, 20, 20);
//
//
//	Entity * mutant = new Mutant();
//	mutant->position = Vector3(400, 0, 0);
//	
//	Model * m = new Model();
//	m->init_from_file("mutant.fbx");
//	m->mesh.allocate_static_buffer();
//	mutant->model = m;
//
//	print(m->name);
//	
//	entities.push(mutant);
//
//
//	Entity * soldier = new Soldier();
//	soldier->position = Vector3(0, 0, 0);
//
//	Model *s = new Model();
//	s->init_from_file("soldier.fbx");
//	s->mesh.allocate_static_buffer();
//	soldier->model = s;
//
//	entities.push(soldier);
//
//	Entity * mutant2 = new Mutant();
//	mutant2->position = Vector3(-400, 0, 0);
//	mutant2->model = m;
//
//	entities.push(mutant2);
//
//
//	Entity * floor = new Floor();
//	floor->position = Vector3(0, 0, 0);
//	floor->model = generate_floor_model(5000.0f, 5000.0f, 50, 50);
//	floor->model->mesh.allocate_static_buffer();
//	entities.push(floor);
//}
//
//void Render_World::render_world()
//{
//	direct3d.device_context->ClearRenderTargetView(direct3d.render_target_view, (float *)&LightSteelBlue);
//	direct3d.device_context->ClearDepthStencilView(direct3d.depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
//	
//	Matrix4 wvp = camera->get_view_projection_matrix();
//	draw_entities(&entities, &wvp);
//	HR(direct3d.swap_chain->Present(0, 0));
//}

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

void bind_entity_vars_with_shader(Entity *entity, Matrix4 &view, Matrix4 &perspective)
{
	Matrix4 position;
	position.indentity();
	position.matrix[3] = Vector4(entity->position, 1.0f);

	Fx_Shader *base = fx_shader_manager.get_shader("base");
	
	Matrix4 t = position * view * perspective;

	base->bind("texture_map", entity->model->diffuse_texture);
	base->bind("world_view_projection", &t);
	base->attach();
}


void draw_entities(Entity_Manager *entity_manager, Matrix4 &view)
{
	Entity * entity = NULL;
	FOR(entity_manager->entities, entity) {
		bind_entity_vars_with_shader(entity, view, direct3d.perspective_matrix);
		draw_mesh(&entity->model->mesh);
	}
}