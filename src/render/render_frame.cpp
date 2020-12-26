#include <d3dx11effect.h>
#include <D3DX11.h>

#include "base.h"
#include "effect.h"
#include "model.h"
#include "mesh.h"
#include "vertex.h"
#include "render_frame.h"
#include "../libs/general.h"
#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"
#include "../libs/geometry_generator.h"

#include "../libs/fbx_loader.h"

void Render_World::init(Free_Camera *_camera)
{
	camera = _camera;


	//Triangle_Mesh *mesh2 = new Triangle_Mesh();
	//load_model_from_obj_file("D:\\andrey\\dev\\directx11_tutorial\\directx11_tutorial\\models\\aline.obj", mesh2);
	//create_default_buffer(direct3d->device, mesh2);


	Triangle_Mesh *mesh = new Triangle_Mesh();
	//generate_box(10, 10, 10, mesh);
	//create_default_buffer(mesh);

	//Triangle_Mesh *mesh1 = new Triangle_Mesh();

	//generate_grid(1000, 1000, mesh1);
	//create_default_buffer(mesh1);

	//meshes.push(mesh);
	//meshes.push(mesh1);

//	generate_grid(40, 40, mesh1);
//	create_default_buffer(direct3d->device, mesh1);

	Triangle_Mesh *box_mesh = new Triangle_Mesh();
	Fbx_Binary_File box;
	//box.read("E:\\andrey\\dev\\models\\test.fbx");
	box.read("E:\\andrey\\dev\\hades\\data\\models\\FBX\\gun.fbx");
	//box.read("E:\\andrey\\dev\\hades\\data\\models\\bird.fbx");
	//box.read("E:\\andrey\\dev\\hades\\data\\models\\box.fbx");
	box.fill_out_mesh(box_mesh);
	create_default_buffer(box_mesh);
	
	//meshes.push(mesh);
	//meshes.push(mesh1);
//	meshes.push(mesh2);
	meshes.push(box_mesh);
	HR(D3DX11CreateShaderResourceViewFromFile(direct3d.device, "E:\\andrey\\dev\\hades\\data\\textures\\gun.jpg", NULL, NULL, &texture, NULL));
}

void Render_World::render_world()
{
	direct3d.device_context->ClearRenderTargetView(direct3d.render_target_view, (float *)&LightSteelBlue);
	direct3d.device_context->ClearDepthStencilView(direct3d.depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	
	Matrix4 r = camera->get_view_projection_matrix();
	for (int i = 0; i < meshes.count; i++) {
		draw_mesh(meshes.at(i), r, texture);
	}
	HR(direct3d.swap_chain->Present(0, 0));
}

void draw_mesh(Triangle_Mesh *mesh, Matrix4 world_view_projection, ID3D11ShaderResourceView *texture)
{
	assert(mesh->vertex_buffer != NULL);
	assert(mesh->index_buffer != NULL);


	//HR(D3DX11CreateShaderResourceViewFromFile(direct3d.device, "E:\\andrey\\dev\\directx11_tutorial\\directx11_tutorial\\Textures\\WoodCrate01.dds", NULL, NULL, &texture, NULL));

	direct3d.device_context->IASetInputLayout(Input_Layout::vertex);
	direct3d.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	direct3d.device_context->IASetVertexBuffers(0, 1, &mesh->vertex_buffer, &stride, &offset);
	direct3d.device_context->IASetIndexBuffer(mesh->index_buffer, DXGI_FORMAT_R32_UINT, 0);

	ID3DX11Effect *fx = NULL;
	get_fx_shaders(&direct3d)->get("base", fx);
	
	D3DX11_PASS_DESC pass_desc;
	D3DX11_TECHNIQUE_DESC tech_desc;
	fx->GetTechniqueByIndex(0)->GetPassByIndex(0)->GetDesc(&pass_desc);
	fx->GetTechniqueByIndex(0)->GetDesc(&tech_desc);

	ID3DX11EffectShaderResourceVariable *texture_map = fx->GetVariableByName("texture_map")->AsShaderResource();
	ID3DX11EffectMatrixVariable *world_view_proejction = fx->GetVariableByName("world_view_projection")->AsMatrix();

	texture_map->SetResource(texture);
	world_view_proejction->SetMatrix((const float *)&world_view_projection);

	fx->GetTechniqueByIndex(0)->GetPassByIndex(0)->Apply(0, direct3d.device_context);

	direct3d.device_context->DrawIndexed(mesh->index_count, 0, 0);
	//RELEASE_COM(texture);
}