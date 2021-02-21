#include <assert.h>


#include "base.h"
#include "vertex.h"
#include "../sys/sys_local.h"
#include "../libs/math/vector.h"
#include "../win32/win_types.h"

using namespace D2D1;

Direct3D direct3d;
Direct2D direct2d;

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

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.Height = win32.window_height;
	texDesc.Width = win32.window_width;
	texDesc.MipLevels = 1;
	texDesc.MiscFlags = 0;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;

	ID3D11Texture2D *texture = NULL;
	HR(direct3d.device->CreateTexture2D(&texDesc, NULL, &texture));

	HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory1));
	HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory));

	float dpi_x;
	float dpi_y;
	factory->GetDesktopDpi(&dpi_x, &dpi_y);

	D2D1_RENDER_TARGET_PROPERTIES rtDesc = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_HARDWARE,
		D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi_x, dpi_y
	);

	IDXGISurface *surface = NULL;
	HR(direct3d.swap_chain->GetBuffer(0, IID_PPV_ARGS(&surface)));

	HR(factory->CreateDxgiSurfaceRenderTarget(surface, &rtDesc, &render_target));
	//HR(render_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightSlateGray),&gray_brush));
	//HR(render_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::CornflowerBlue),&blue_brush));
}

void Direct2D::draw()
{
	//// Draw a gradient background.

	//	//D2D1_SIZE_F targetSize = m_pBackBufferRT->GetSize();

	ID2D1SolidColorBrush *black_brush = NULL;
	render_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &black_brush);

	render_target->BeginDraw();
	render_target->DrawRectangle(
		D2D1::RectF(
			10.0f,
			10.0f,
			300.0f,
			300.0f),
		black_brush);
	render_target->EndDraw();
	
	//D2D1_SIZE_F target_size = render_target->GetSize();
	//target_size.width /= 2;
	//target_size.height /= 2;

	//render_target->BeginDraw();

	//gray_brush->SetTransform(
	//	D2D1::Matrix3x2F::Scale(target_size)
	//);

	//D2D1_RECT_F rect = D2D1::RectF(
	//	0.0f,
	//	0.0f,
	//	target_size.width,
	//	target_size.height
	//);

	//render_target->FillRectangle(&rect, gray_brush);

	//render_target->EndDraw();
	//render_target->BeginDraw();

	//render_target->SetTransform(D2D1::Matrix3x2F::Identity());

	//render_target->Clear(D2D1::ColorF(D2D1::ColorF::White));
	//render_target->EndDraw();
}

void Direct3D::init(const Win32_State *win32)
{
	UINT create_device_flag = 0;

#if defined(DEBUG) || defined(_DEBUG)  
	//create_device_flag |= D3D11_CREATE_DEVICE_DEBUG 
	create_device_flag |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;
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
	
	dxgi_factory->CreateSwapChain(device, &sd, &swap_chain);


	//// Init directx 2d
	//DXGI_SWAP_CHAIN_DESC1 sd;
	//sd.Width = win32->window_width;
	//sd.Height = win32->window_height;
	//sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	//if (true) {
	//	sd.SampleDesc.Count = 4;
	//	sd.SampleDesc.Quality = quality_levels - 1;
	//} else {
	//	sd.SampleDesc.Count = 1;
	//	sd.SampleDesc.Quality = 0;
	//}

	//sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	//sd.BufferCount = 1;
	//sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	//sd.Flags = 0;

	//IDXGIDevice* dxgi_device = 0;
	//HR(device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgi_device));

	//IDXGIAdapter* dxgi_adapter = 0;
	//HR(dxgi_device->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgi_adapter));

	//IDXGIFactory2* dxgi_factory = 0;
	//HR(dxgi_adapter->GetParent(__uuidof(IDXGIFactory2), (void**)&dxgi_factory));

	////HR(dxgi_factory->CreateSwapChain(device, &sd, &swap_chain));
	//dxgi_factory->CreateSwapChainForHwnd(device, win32->window, &sd, nullptr, nullptr, &swap_chain);

	//HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &direct2d.factory));
	//HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &direct2d.factory1));
	//
	////dxgi_factory->
	//HR(direct2d.factory1->CreateDevice(dxgi_device, &direct2d.device));
	//HR(direct2d.device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &direct2d.device_context));
	//
	//IDXGISurface *surface = NULL;
	//HR(swap_chain->GetBuffer(0, __uuidof(IDXGISurface), (void **)surface));

	//auto props = BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

	//ID2D1Bitmap1 *bitmap = NULL;
	//HR(direct2d.device_context->CreateBitmapFromDxgiSurface(surface, props, &bitmap));

	//direct2d.device_context->SetTarget(bitmap);

	//float32 dpi_x;
	//float32 dpi_y;
	//direct2d.factory->GetDesktopDpi(&dpi_x, &dpi_y);

	//direct2d.device_context->SetDpi(dpi_x, dpi_y);


	RELEASE_COM(dxgi_device);
	RELEASE_COM(dxgi_adapter);
	RELEASE_COM(dxgi_factory);

	resize(win32);
}


//void Direct3D::init(const Win32_State *win32)
//{
//	D2D1_FACTORY_OPTIONS fo = {};
//#if defined(DEBUG) || defined(_DEBUG)  
//	fo.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
//#endif
//
//	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
//	
//	ID2D1Factory1 *factory1;
//	HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, fo, &factory1));
//
//	D3D_FEATURE_LEVEL featureLevels[] =
//	{
//		D3D_FEATURE_LEVEL_11_1,
//		D3D_FEATURE_LEVEL_11_0,
//		D3D_FEATURE_LEVEL_10_1,
//		D3D_FEATURE_LEVEL_10_0,
//		D3D_FEATURE_LEVEL_9_3,
//		D3D_FEATURE_LEVEL_9_2,
//		D3D_FEATURE_LEVEL_9_1
//	};
//	D3D_FEATURE_LEVEL available_feature_level;
//	D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, creationFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &device, &available_feature_level , &device_context);
//
//	IDXGIDevice *dxgi_device = NULL;
//	HR(device->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgi_device));
//
//	
//	HR(factory1->CreateDevice(dxgi_device, &direct2d.device));
//
//	HR(direct2d.device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &direct2d.device_context));
//
//
//	// Allocate a descriptor.
//	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = { 0 };
//	swap_chain_desc.Width = win32->window_width;                           // use automatic sizing
//	swap_chain_desc.Height = win32->window_height;
//	swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // this is the most common swapchain format
//	swap_chain_desc.Stereo = false;
//	swap_chain_desc.SampleDesc.Count = 1;                // don't use multi-sampling
//	swap_chain_desc.SampleDesc.Quality = 0;
//	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//	swap_chain_desc.BufferCount = 2;                     // use double buffering to enable flip
//	swap_chain_desc.Scaling = DXGI_SCALING_NONE;
//	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // all apps must use this SwapEffect
//	swap_chain_desc.Flags = 0;
//
//	//DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
//	//swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
//	//swap_chain_desc.SampleDesc.Count = 1;
//	//swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//	//swap_chain_desc.BufferCount = 2;
//
//
//	IDXGIAdapter* dxgi_adapter = 0;
//	HR(dxgi_device->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgi_adapter));
//
//	IDXGIFactory2* dxgi_factory = 0;
//	HR(dxgi_adapter->GetParent(__uuidof(IDXGIFactory2), (void**)&dxgi_factory));
//
//	HR(dxgi_factory->CreateSwapChainForHwnd(device, win32->window, &swap_chain_desc, nullptr, nullptr, &swap_chain))
//
//	//dxgi_device->SetMaximumFrameLatency(1);
//
//	ID3D11Texture2D* back_buffer = NULL;
//	HR(swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&back_buffer));
//
//	float dpi_x;
//	float dpi_y;
//	factory1->GetDesktopDpi(&dpi_x, &dpi_y);
//	
//	D2D1_BITMAP_PROPERTIES1 bitmapProperties = BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dpi_x, dpi_y);
//	
//	IDXGISurface *surface = NULL;
//	swap_chain->GetBuffer(0, IID_PPV_ARGS(&surface));
//	
//	direct2d.device_context->CreateBitmapFromDxgiSurface(surface, &bitmapProperties, &direct2d.bitmap1);
//	direct2d.device_context->SetTarget(direct2d.bitmap1);
//}

void Direct3D::shutdown()
{
	RELEASE_COM(device);
	RELEASE_COM(device_context);
	RELEASE_COM(swap_chain);
	RELEASE_COM(render_target_view);
	RELEASE_COM(depth_stencil_view);
	RELEASE_COM(depth_stencil_buffer);
	RELEASE_COM(back_buffer);
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


