#include "directx.h"
#include "../sys/sys_local.h"
#include "../libs/color.h"


Direct2D::~Direct2D()
{
	shutdown();
}

void Direct2D::init(IDXGISwapChain *swap_chain)
{
	assert(swap_chain);

	HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory));

	float dpi_x;
	float dpi_y;
	factory->GetDesktopDpi(&dpi_x, &dpi_y);

	D2D1_RENDER_TARGET_PROPERTIES rtDesc = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_HARDWARE, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi_x, dpi_y);

	IDXGISurface *surface = NULL;
	HR(swap_chain->GetBuffer(0, IID_PPV_ARGS(&surface)));

	HR(factory->CreateDxgiSurfaceRenderTarget(surface, &rtDesc, &render_target));

	RELEASE_COM(surface);
}

void Direct2D::shutdown()
{
	RELEASE_COM(factory);
	RELEASE_COM(render_target);
}

DirectX11::~DirectX11()
{
	shutdown();
}

void DirectX11::init()
{
	UINT create_device_flag = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(DEBUG) || defined(_DEBUG)  
	create_device_flag |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL feature_level;
	HRESULT hr = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, create_device_flag, 0, 0, D3D11_SDK_VERSION,
		&device, &feature_level, &device_context);

	if (FAILED(hr)) {
		error("D3D11CreateDevice Failed.");
		return;
	}

	if (feature_level < D3D_FEATURE_LEVEL_11_0) {
		error("Direct3D Feature Level 11 unsupported.");
		return;
	}

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = win32.window_width;
	sd.BufferDesc.Height = win32.window_height;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	if (true) {
		sd.SampleDesc.Count = 4;
		sd.SampleDesc.Quality = quality_levels - 1;
	} else {
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
	}

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = win32.window;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;

	IDXGIDevice* dxgi_device = 0;
	HR(device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgi_device));

	IDXGIAdapter* dxgi_adapter = 0;
	HR(dxgi_device->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgi_adapter));

	IDXGIFactory* dxgi_factory = 0;
	HR(dxgi_adapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgi_factory));

	HR(dxgi_factory->CreateSwapChain(device, &sd, &swap_chain));

	RELEASE_COM(dxgi_device);
	RELEASE_COM(dxgi_adapter);
	RELEASE_COM(dxgi_factory);

	resize();
}


void DirectX11::shutdown()
{
	RELEASE_COM(device);
	RELEASE_COM(device_context);
	RELEASE_COM(swap_chain);
	RELEASE_COM(render_target_view);
	RELEASE_COM(depth_stencil_view);
	RELEASE_COM(depth_stencil_buffer);
}

void DirectX11::resize()
{
	assert(device);
	assert(device_context);
	assert(swap_chain);


	RELEASE_COM(render_target_view);
	RELEASE_COM(depth_stencil_view);
	RELEASE_COM(depth_stencil_buffer);

	direct2d.shutdown();

	// Resize the swap chain and recreate the render target view.

	HR(swap_chain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));

	ID3D11Texture2D* back_buffer = NULL;
	HR(swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer)));
	HR(device->CreateRenderTargetView(back_buffer, 0, &render_target_view));

	RELEASE_COM(back_buffer);

	// Create the depth/stencil buffer and view.

	D3D11_TEXTURE2D_DESC depth_stencil_desc;

	depth_stencil_desc.Width = win32.window_width;
	depth_stencil_desc.Height = win32.window_height;
	depth_stencil_desc.MipLevels = 1;
	depth_stencil_desc.ArraySize = 1;
	depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Use 4X MSAA? --must match swap chain MSAA values.
	if (true) {
		depth_stencil_desc.SampleDesc.Count = 4;
		depth_stencil_desc.SampleDesc.Quality = quality_levels - 1;
	} else {
		depth_stencil_desc.SampleDesc.Count = 1;
		depth_stencil_desc.SampleDesc.Quality = 0;
	}

	depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depth_stencil_desc.CPUAccessFlags = 0;
	depth_stencil_desc.MiscFlags = 0;

	HR(device->CreateTexture2D(&depth_stencil_desc, 0, &depth_stencil_buffer));
	HR(device->CreateDepthStencilView(depth_stencil_buffer, 0, &depth_stencil_view));


	// Bind the render target view and depth/stencil view to the pipeline.

	device_context->OMSetRenderTargets(1, &render_target_view, depth_stencil_view);


	// Set the viewport transform.

	D3D11_VIEWPORT mScreenViewport;
	mScreenViewport.Width = static_cast<float>(win32.window_width);
	mScreenViewport.Height = static_cast<float>(win32.window_height);
	mScreenViewport.MaxDepth = 1.0f;
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;

	device_context->RSSetViewports(1, &mScreenViewport);

	direct2d.init(swap_chain);
}

#define BEGIN_DRAW() (render_target->BeginDraw())
#define END_DRAW() (render_target->EndDraw())

void Direct2D::test_draw()
{
	ID2D1SolidColorBrush *black_brush = NULL;
	render_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &black_brush);

	render_target->BeginDraw();
	render_target->DrawRectangle(D2D1::RectF(10.0f, 10.0f, 200.0f, 200.0f), black_brush);

	fill_rect(10, 320, 200, 200, Color::Black);
	draw_rect(10, 320, 200, 200, Color::Red);

	render_target->EndDraw();
}

void Direct2D::fill_rect(int x, int y, int width, int height, const Color &background_color)
{
	BEGIN_DRAW();
	ID2D1SolidColorBrush *brush = NULL;
	D2D1_RECT_F rect;
	rect.left = x;
	rect.top = y;
	rect.right = x + width;
	rect.bottom = y + height;

	render_target->CreateSolidColorBrush((Color)background_color, &brush);
	render_target->FillRectangle(rect, brush);

	RELEASE_COM(brush);
	END_DRAW();
}

void Direct2D::draw_rect(int x, int y, int width, int height, const Color &stroke_color, ID2D1StrokeStyle *stroke_style, float stroke_width)
{
	ID2D1SolidColorBrush *brush = NULL;
	D2D1_RECT_F rect;
	rect.left = x;
	rect.top = y;
	rect.right = x + width;
	rect.bottom = y + height;

	BEGIN_DRAW();
	render_target->CreateSolidColorBrush((Color)stroke_color, &brush);
	render_target->DrawRectangle(rect, brush, stroke_width, stroke_style);
	END_DRAW();

	RELEASE_COM(brush);
}

void Direct2D::draw_rounded_rect(int x, int y, int width, int height, float radius_x, float radius_y, const Color &background_color)
{
	D2D1_RECT_F rect;
	rect.left = x;
	rect.top = y;
	rect.right = x + width;
	rect.bottom = y + height;

	ID2D1SolidColorBrush *brush = NULL;
	render_target->CreateSolidColorBrush((Color)background_color, &brush);

	D2D1_ROUNDED_RECT rounded_rect = D2D1::RoundedRect(rect, radius_x, radius_y);

	BEGIN_DRAW();
	//directx_render.direct2d.render_target->DrawRoundedRectangle(rounded_rect, brush);
	render_target->FillRoundedRectangle(rounded_rect, brush);
	END_DRAW();

	RELEASE_COM(brush);
}

void Direct2D::draw_bitmap(const D2D1_RECT_F &rect, ID2D1Bitmap *bitmap, float scale)
{
	BEGIN_DRAW();
	render_target->SetTransform(D2D1::Matrix3x2F::Scale(D2D1::Size(scale, scale), D2D1::Point2F(rect.left, rect.top)));
	render_target->DrawBitmap(bitmap, rect);
	render_target->SetTransform(D2D1::Matrix3x2F::Identity());
	END_DRAW();
}

void DirectX11::draw_normals(Entity *entity, float line_len)
{
	//Triangle_Mesh *mesh = &entity->model->mesh;
	//Array<Vertex_XC> normals;

	//for (int i = 0; i < mesh->vertex_count; i++) {
	//	Vertex_XC position1;
	//	position1.position = mesh->vertices[i].position;
	//	position1.color = Vector3(1.0f, 0.0f, 0.0f);
	//	normals.push(position1);

	//	Vertex_XC position2;
	//	position2.position = (mesh->vertices[i].position) + (mesh->vertices[i].normal * Vector3(line_len, line_len, line_len));
	//	position2.color = Vector3(1.0f, 0.0f, 0.0f);
	//	normals.push(position2);
	//}

	//device_context->IASetInputLayout(Input_Layout::vertex_color);
	//device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	//u32 stride = sizeof(Vertex_XC);
	//u32 offset = 0;

	//ID3D11Buffer *buffer = NULL;
	//create_static_vertex_buffer(normals.count, stride, (void *)&normals.items[0], &buffer);

	//device_context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);

	//device_context->Draw(normals.count, 0);

	//buffer->Release();
}


void DirectX11::draw_indexed_mesh(Triangle_Mesh *mesh)
{
	assert(mesh->index_buffer);

	device_context->IASetInputLayout(Input_Layout::vertex);
	device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	device_context->IASetVertexBuffers(0, 1, &mesh->vertex_buffer, &stride, &offset);
	device_context->IASetIndexBuffer(mesh->index_buffer, DXGI_FORMAT_R32_UINT, 0);

	device_context->DrawIndexed(mesh->index_count, 0, 0);
}

void DirectX11::draw_not_indexed_mesh(Triangle_Mesh *mesh)
{
	device_context->IASetInputLayout(Input_Layout::vertex);
	device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	device_context->IASetVertexBuffers(0, 1, &mesh->vertex_buffer, &stride, &offset);

	device_context->Draw(mesh->vertex_count, 0);
}

void DirectX11::draw_mesh(Triangle_Mesh *mesh)
{
	assert(mesh);
	assert(mesh->vertex_buffer);

	if (mesh->is_indexed) {
		draw_indexed_mesh(mesh);
	} else {
		draw_not_indexed_mesh(mesh);
	}

}

void DirectX11::draw_shadow(Entity *entity, Fx_Shader *fx_shader_light, Light *light, Matrix4 &view, Matrix4 &perspective)
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

	HR(device->CreateBlendState(&transparent_desc, &transparent));

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

	HR(device->CreateDepthStencilState(&no_double_blending_desc, &no_double_blending));

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

	device_context->OMSetDepthStencilState(no_double_blending, 0);
	float b[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	device_context->OMSetBlendState(transparent, b, 0xffffffff);

	if (entity->model->render_surface_use == RENDER_MODEL_SURFACE_USE_TEXTURE) {
		fx_shader_light->attach();
	} else {
		fx_shader_light->attach(1);
	}
	draw_mesh(&entity->model->mesh);

	device_context->OMSetBlendState(0, b, 0xffffffff);
	device_context->OMSetDepthStencilState(0, 0);
}