#include <wincodec.h>

#include "directx.h"
#include "../sys/sys_local.h"
#include "../libs/color.h"


Direct2D direct2d;
DirectX11 directx11;


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

void DirectX11::resize(Direct2D *direct2d)
{
	assert(device);
	assert(device_context);
	assert(swap_chain);


	RELEASE_COM(render_target_view);
	RELEASE_COM(depth_stencil_view);
	RELEASE_COM(depth_stencil_buffer);

	if (direct2d) { direct2d->shutdown(); }

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

	if (direct2d) { direct2d->init(swap_chain); }
}

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
	ID2D1SolidColorBrush *brush = NULL;
	D2D1_RECT_F rect;
	rect.left = x;
	rect.top = y;
	rect.right = x + width;
	rect.bottom = y + height;

	render_target->CreateSolidColorBrush((Color)background_color, &brush);
	render_target->FillRectangle(rect, brush);

	RELEASE_COM(brush);
}

void Direct2D::draw_rect(int x, int y, int width, int height, const Color &stroke_color, ID2D1StrokeStyle *stroke_style, float stroke_width)
{
	ID2D1SolidColorBrush *brush = NULL;
	D2D1_RECT_F rect;
	rect.left = x;
	rect.top = y;
	rect.right = x + width;
	rect.bottom = y + height;

	render_target->CreateSolidColorBrush((Color)stroke_color, &brush);
	render_target->DrawRectangle(rect, brush, stroke_width, stroke_style);

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

	//directx_render.direct2d.render_target->DrawRoundedRectangle(rounded_rect, brush);
	render_target->FillRoundedRectangle(rounded_rect, brush);

	RELEASE_COM(brush);
}

void Direct2D::draw_bitmap(const D2D1_RECT_F &rect, ID2D1Bitmap *bitmap, float scale)
{
	render_target->SetTransform(D2D1::Matrix3x2F::Scale(D2D1::Size(scale, scale), D2D1::Point2F(rect.left, rect.top)));
	render_target->DrawBitmap(bitmap, rect);
	render_target->SetTransform(D2D1::Matrix3x2F::Identity());
}

void load_bitmap_from_file(const char *file_path, int dest_width, int dest_height, ID2D1Bitmap **bitmap)
{
	IWICImagingFactory2 *pIWICFactory = NULL;
	IWICBitmapDecoder *pDecoder = NULL;
	IWICBitmapFrameDecode *pSource = NULL;
	IWICStream *pStream = NULL;
	IWICFormatConverter *pConverter = NULL;
	IWICBitmapScaler *pScaler = NULL;

	HR(CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pIWICFactory)));

	wchar_t *f = (wchar_t *)file_path;
	int len = strlen(file_path);
	wchar_t *wfile_path = new wchar_t[len + 1];

	size_t converted_chars = 0;
	mbstowcs(wfile_path, file_path, len + 1);

	wfile_path[len] = '\0';
	HR(pIWICFactory->CreateDecoderFromFilename(wfile_path, NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder));

	HR(pDecoder->GetFrame(0, &pSource));

	HR(pIWICFactory->CreateFormatConverter(&pConverter));

	HR(pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut));

	HR(direct2d.render_target->CreateBitmapFromWicBitmap(pConverter, NULL, bitmap));

	RELEASE_COM(pIWICFactory)
	RELEASE_COM(pDecoder);
	RELEASE_COM(pSource);
	RELEASE_COM(pStream);
	RELEASE_COM(pConverter);
	RELEASE_COM(pScaler);
}