#include <assert.h>

#include "base.h"
#include "vertex.h"
#include "../sys/sys_local.h"
#include "../libs/math/vector.h"

Direct3D direct3d;

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

Direct2D::~Direct2D()
{
	RELEASE_COM(factory);
	RELEASE_COM(render_target);
	RELEASE_COM(gray_brush);
	RELEASE_COM(blue_brush);
}

void Direct2D::init()
{
	//HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory));

	//D2D1_SIZE_U window_size;
	//get_window_size(win32.window, (s32 *)&window_size.width, (s32 *)&window_size.height);
	//HR(factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(win32.window, window_size), &render_target));

	//HR(render_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightSlateGray),&gray_brush));

	//HR(render_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::CornflowerBlue),&blue_brush));
}

void Direct3D::init(const Win32_State *win32)
{
	UINT create_device_flag = 0;

#if defined(DEBUG) || defined(_DEBUG)  
	create_device_flag |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL feature_level;
	HRESULT hr = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, create_device_flag, 0, 0, D3D11_SDK_VERSION,
		&device, &feature_level, &device_context);

	if (FAILED(hr)) {
		MessageBox(0, "D3D11CreateDevice Failed.", 0, 0);
		return;
	}

	if (feature_level != D3D_FEATURE_LEVEL_11_0) {
		MessageBox(0, "Direct3D Feature Level 11 unsupported.", 0, 0);
		return;
	}


	HR(device->CheckMultisampleQualityLevels(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, &quality_levels));
	//assert(m4xMsaaQuality > 0);

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = win32->window_width;
	sd.BufferDesc.Height = win32->window_height;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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
	sd.OutputWindow = win32->window;
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

	resize(win32);
}

void Direct3D::shutdown()
{
	RELEASE_COM(device);
	RELEASE_COM(device_context);
	RELEASE_COM(swap_chain);
	RELEASE_COM(render_target_view);
	RELEASE_COM(depth_stencil_view);
	RELEASE_COM(depth_stencil_buffer);
}

void Direct3D::resize(const Win32_State *win32)
{
	assert(device);
	assert(device_context);
	assert(swap_chain);


	RELEASE_COM(render_target_view);
	RELEASE_COM(depth_stencil_view);
	RELEASE_COM(depth_stencil_buffer);
	

	// Resize the swap chain and recreate the render target view.

	HR(swap_chain->ResizeBuffers(1, win32->window_width, win32->window_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	
	ID3D11Texture2D* back_buffer = NULL;
	HR(swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer)));
	HR(device->CreateRenderTargetView(back_buffer, 0, &render_target_view));
	RELEASE_COM(back_buffer);

	// Create the depth/stencil buffer and view.

	D3D11_TEXTURE2D_DESC depth_stencil_desc;

	depth_stencil_desc.Width = win32->window_width;
	depth_stencil_desc.Height = win32->window_height;
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
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(win32->window_width);
	mScreenViewport.Height = static_cast<float>(win32->window_height);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	device_context->RSSetViewports(1, &mScreenViewport);

	perspective_matrix = get_perspective_matrix(win32->window_width, win32->window_height, 1.0f, 1000.0f);
}


