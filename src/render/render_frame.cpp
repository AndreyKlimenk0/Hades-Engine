#include "base.h"
#include "effect.h"
#include "vertex.h"
#include "render_frame.h"
#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"

#include <d3dx11effect.h>
#include <D3DX11.h>

Vector4 White = { 1.0f, 1.0f, 1.0f, 1.0f };
Vector4 Black = { 0.0f, 0.0f, 0.0f, 1.0f };
Vector4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
Vector4 Green = { 0.0f, 1.0f, 0.0f, 1.0f };
Vector4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
Vector4 Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
Vector4 Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
Vector4 Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };
Vector4 Silver = { 0.75f, 0.75f, 0.75f, 1.0f };
Vector4 LightSteelBlue = { 0.69f, 0.77f, 0.87f, 1.0f };


void render_frame(Direct3D *direct3d, Win32_State *win32)
{
	const float color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	direct3d->device_context->ClearRenderTargetView(direct3d->render_target_view, color);
	direct3d->device_context->ClearDepthStencilView(direct3d->depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	
	Vertex_Color vertices[] =
	{
		{ Vector3(-1.0f, -1.0f, -1.0f), White },
		{ Vector3(-1.0f, +1.0f, -1.0f), Black   },
		{ Vector3(+1.0f, +1.0f, -1.0f), Red     },
		{ Vector3(+1.0f, -1.0f, -1.0f), Green   },
		{ Vector3(-1.0f, -1.0f, +1.0f), Blue    },
		{ Vector3(-1.0f, +1.0f, +1.0f), Yellow  },
		{ Vector3(+1.0f, +1.0f, +1.0f), Cyan    },
		{ Vector3(+1.0f, -1.0f, +1.0f), Magenta }
	};

	UINT indices[] = {
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	direct3d->device_context->IASetInputLayout(Input_Layout::vertex_color);
	direct3d->device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D11Buffer *box_vertex = NULL;
	ID3D11Buffer *box_index = NULL;
	
	create_default_buffer(direct3d->device, sizeof(Vertex_Color) * 8, &vertices, 36, &indices, &box_vertex, &box_index);

	UINT stride = sizeof(Vertex_Color);
	UINT offset = 0;
	direct3d->device_context->IASetVertexBuffers(0, 1, &box_vertex, &stride, &offset);
	direct3d->device_context->IASetIndexBuffer(box_index, DXGI_FORMAT_R32_UINT, 0);


	ID3DX11Effect *fx = NULL;
	get_fx_shaders(direct3d)->get("base", fx);

	D3DX11_PASS_DESC pass_desc;
	D3DX11_TECHNIQUE_DESC tech_desc;
	fx->GetTechniqueByIndex(0)->GetPassByIndex(0)->GetDesc(&pass_desc);
	fx->GetTechniqueByIndex(0)->GetDesc(&tech_desc);
	

	Vector4 pos = Vector4(2.0f, 0.0f, -10.0f, 0.0f);
	Vector4 target = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 up = Vector4(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);

	Matrix4 mv = Matrix4(V);
	Matrix4 m = get_perspective_matrix(win32);

	ID3DX11EffectMatrixVariable *world_view_proejction = fx->GetVariableByName("world_view_projection")->AsMatrix();

	Matrix4 r = mv * m;
	world_view_proejction->SetMatrix((const float *)&r);
	
	fx->GetTechniqueByIndex(0)->GetPassByIndex(0)->Apply(0, direct3d->device_context);
	
	direct3d->device_context->DrawIndexed(36, 0, 0);

	HR(direct3d->swap_chain->Present(0, 0));
}