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

void Render_World::init(Free_Camera *_camera)
{
	camera = _camera;
	camera->position = Vector3(5, 20, 20);


	Triangle_Mesh *grid = new Triangle_Mesh();

	generate_grid(5000.0f, 5000.0f, 50, 50, grid);
	create_default_buffer(grid);
	meshes.push(grid);

	
	Triangle_Mesh *mutant_mesh = new Triangle_Mesh();
	load_fbx_model("mutant.fbx", mutant_mesh);

	create_default_buffer(mutant_mesh);
	meshes.push(mutant_mesh);


	HR(D3DX11CreateShaderResourceViewFromFile(direct3d.device, "E:\\andrey\\dev\\hades\\data\\textures\\floor.jpg", NULL, NULL, &grid->texture, NULL));

}

void Render_World::render_world()
{
	direct3d.device_context->ClearRenderTargetView(direct3d.render_target_view, (float *)&LightSteelBlue);
	direct3d.device_context->ClearDepthStencilView(direct3d.depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	
	Matrix4 wvp = camera->get_view_projection_matrix();
	for (int i = 0; i < meshes.count; i++) {
		draw_mesh(meshes.at(i), wvp);
	}
	HR(direct3d.swap_chain->Present(0, 0));
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

void draw_mesh(Triangle_Mesh *mesh, Matrix4 world_view_projection)
{
	assert(mesh);
	assert(mesh->texture);
	assert(mesh->vertex_buffer);


	Fx_Shader *base = fx_shader_manager.get_shader("base");

	base->bind("texture_map", mesh->texture);
	base->bind("texture_asdfasdf", mesh->texture);
	base->bind("world_view_projection", &world_view_projection);
	base->bind("slslls", &world_view_projection);
	base->attach();

	if (mesh->is_indexed) {
		draw_indexed_mesh(mesh);
	} else {
		draw_not_indexed_mesh(mesh);
	}

}