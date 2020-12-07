#include <d3dx11effect.h>
#include <D3DX11.h>


#include "effect.h"
#include "model.h"
#include "vertex.h"
#include "render_frame.h"
#include "../libs/general.h"
#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"
#include "../libs/geometry_generator.h"


void Render_World::init(Direct3D *_direct3d, Win32_State *_win32, Free_Camera *_camera)
{
	direct3d = _direct3d;
	win32 = _win32;
	camera = _camera;

	Triangle_Mesh *mesh2 = new Triangle_Mesh();
	load_model_from_obj_file("D:\\andrey\\dev\\directx11_tutorial\\directx11_tutorial\\models\\aline.obj", mesh2);
	create_default_buffer(direct3d->device, mesh2);

	Triangle_Mesh *mesh = new Triangle_Mesh();
	generate_box(mesh);
	create_default_buffer(direct3d->device, mesh);

	Triangle_Mesh *mesh1 = new Triangle_Mesh();
	generate_grid(40, 40, mesh1);
	create_default_buffer(direct3d->device, mesh1);

	//meshes.push(mesh);
	//meshes.push(mesh1);
	meshes.push(mesh2);

}

void Render_World::render_world()
{
	Matrix4 r = camera->get_view_matrix() * get_perspective_matrix(win32);
	for (int i = 0; i < meshes.count; i++) {
		draw_mesh(direct3d, meshes.at(i), r);
	}
}

void draw_mesh(Direct3D *direct3d, Triangle_Mesh *mesh, Matrix4 world_view_projection)
{
	assert(mesh->vertex_buffer != NULL);
	assert(mesh->index_buffer != NULL);

	direct3d->device_context->ClearRenderTargetView(direct3d->render_target_view, (float *)&LightSteelBlue);
	direct3d->device_context->ClearDepthStencilView(direct3d->depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	direct3d->device_context->IASetInputLayout(Input_Layout::vertex_color);
	direct3d->device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	UINT stride = sizeof(Vertex_Color);
	UINT offset = 0;
	direct3d->device_context->IASetVertexBuffers(0, 1, &mesh->vertex_buffer, &stride, &offset);
	direct3d->device_context->IASetIndexBuffer(mesh->index_buffer, DXGI_FORMAT_R32_UINT, 0);

	ID3DX11Effect *fx = NULL;
	get_fx_shaders(direct3d)->get("base", fx);
	
	D3DX11_PASS_DESC pass_desc;
	D3DX11_TECHNIQUE_DESC tech_desc;
	fx->GetTechniqueByIndex(0)->GetPassByIndex(0)->GetDesc(&pass_desc);
	fx->GetTechniqueByIndex(0)->GetDesc(&tech_desc);

	ID3DX11EffectMatrixVariable *world_view_proejction = fx->GetVariableByName("world_view_projection")->AsMatrix();
	world_view_proejction->SetMatrix((const float *)&world_view_projection);

	fx->GetTechniqueByIndex(0)->GetPassByIndex(0)->Apply(0, direct3d->device_context);

	direct3d->device_context->DrawIndexed(mesh->index_count, 0, 0);

	HR(direct3d->swap_chain->Present(0, 0));

}