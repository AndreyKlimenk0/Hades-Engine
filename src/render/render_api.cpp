#include "render_api.h"
#include "../sys/sys_local.h"
#include "../libs/str.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "render_world.h"
#include "../sys/engine.h"


static Multisample_Info default_render_api_multisample;


inline D3D11_PRIMITIVE_TOPOLOGY to_dx11_primitive_type(Render_Primitive_Type primitive_type)
{
	switch (primitive_type)
	{
		case RENDER_PRIMITIVE_TRIANGLES:
			return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case RENDER_PRIMITIVE_LINES:
			return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	}
	assert(false);
	return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

inline D3D11_USAGE to_dx11_resource_usage(Resource_Usage usage)
{
	switch (usage) {
		case RESOURCE_USAGE_DEFAULT:
			return D3D11_USAGE_DEFAULT;
		case RESOURCE_USAGE_IMMUTABLE:
			return D3D11_USAGE_IMMUTABLE;
		case RESOURCE_USAGE_DYNAMIC:
			return D3D11_USAGE_DYNAMIC;
		case RESOURCE_USAGE_STAGING:
			return D3D11_USAGE_STAGING;
	}
	assert(false);
	return D3D11_USAGE_DEFAULT;
}

inline D3D11_BLEND to_dx11_blend(Blend_Option blend_option)
{
	switch (blend_option)
	{
		case BLEND_ZERO:
			return D3D11_BLEND_ZERO;
		case BLEND_ONE:
			return D3D11_BLEND_ONE;
		case BLEND_SRC_COLOR:
			return D3D11_BLEND_SRC_COLOR;
		case BLEND_INV_SRC_COLOR:
			return D3D11_BLEND_INV_SRC_COLOR;
		case BLEND_SRC_ALPHA:
			return D3D11_BLEND_SRC_ALPHA;
		case BLEND_INV_SRC_ALPHA:
			return D3D11_BLEND_INV_SRC_ALPHA;
		case BLEND_DEST_ALPHA:
			return D3D11_BLEND_DEST_ALPHA;
		case BLEND_INV_DEST_ALPHA:
			return D3D11_BLEND_INV_DEST_ALPHA;
		case BLEND_DEST_COLOR:
			return D3D11_BLEND_DEST_COLOR;
		case BLEND_INV_DEST_COLOR:
			return D3D11_BLEND_INV_DEST_COLOR;
		case BLEND_SRC_ALPHA_SAT:
			return D3D11_BLEND_SRC_ALPHA_SAT;
		case BLEND_BLEND_FACTOR:
			return D3D11_BLEND_BLEND_FACTOR;
		case BLEND_INV_BLEND_FACTOR:
			return D3D11_BLEND_INV_BLEND_FACTOR;
		case BLEND_SRC1_COLOR:
			return D3D11_BLEND_SRC1_COLOR;
		case BLEND_INV_SRC1_COLOR:
			return D3D11_BLEND_INV_SRC1_COLOR;
		case BLEND_SRC1_ALPHA:
			return D3D11_BLEND_SRC1_ALPHA;
		case BLEND_INV_SRC1_ALPHA:
			return D3D11_BLEND_INV_SRC1_ALPHA;
	}
	assert(false);
	return D3D11_BLEND_ZERO;
}

inline D3D11_BLEND_OP to_dx11_blend_op(Blend_Operation blend_operation)
{
	switch (blend_operation)
	{
		case BLEND_OP_ADD:
			return D3D11_BLEND_OP_ADD;
		case BLEND_OP_SUBTRACT:
			return D3D11_BLEND_OP_SUBTRACT;
		case BLEND_OP_REV_SUBTRACT:
			return D3D11_BLEND_OP_REV_SUBTRACT;
		case BLEND_OP_MIN:
			return D3D11_BLEND_OP_MIN;
		case BLEND_OP_MAX:
			return D3D11_BLEND_OP_MAX;
	}
	assert(false);
	return D3D11_BLEND_OP_ADD;
}

inline D3D11_STENCIL_OP to_dx11_stencil_op(Stencil_Operation stencil_operation)
{
	switch (stencil_operation)
	{
		case STENCIL_OP_KEEP:
			return D3D11_STENCIL_OP_KEEP;
		case STENCIL_OP_ZERO:
			return D3D11_STENCIL_OP_ZERO;
		case STENCIL_OP_REPLACE:
			return D3D11_STENCIL_OP_REPLACE;
		case STENCIL_OP_INCR_SAT:
			return D3D11_STENCIL_OP_INCR_SAT;
		case STENCIL_OP_DECR_SAT:
			return D3D11_STENCIL_OP_DECR_SAT;
		case STENCIL_OP_INVERT:
			return D3D11_STENCIL_OP_INVERT;
		case STENCIL_OP_INCR:
			return D3D11_STENCIL_OP_INCR;
		case STENCIL_OP_DECR:
			return D3D11_STENCIL_OP_DECR;
	}
	assert(false);
	return D3D11_STENCIL_OP_KEEP;
}

inline D3D11_COMPARISON_FUNC to_dx11_comparison_func(Comparison_Func func)
{
	switch (func)
	{
		case COMPARISON_NEVER:
			return D3D11_COMPARISON_NEVER;
		case COMPARISON_LESS:
			return D3D11_COMPARISON_LESS;
		case COMPARISON_EQUAL:
			return D3D11_COMPARISON_EQUAL;
		case COMPARISON_LESS_EQUAL:
			return D3D11_COMPARISON_LESS_EQUAL;
		case COMPARISON_GREATER:
			return D3D11_COMPARISON_GREATER;
		case COMPARISON_NOT_EQUAL:
			return D3D11_COMPARISON_NOT_EQUAL;
		case COMPARISON_GREATER_EQUAL:
			return D3D11_COMPARISON_GREATER_EQUAL;
		case COMPARISON_ALWAYS:
			return D3D11_COMPARISON_ALWAYS;
	}
	assert(false);
	return D3D11_COMPARISON_NEVER;
}

inline u32 dxgi_format_size(DXGI_FORMAT format)
{
	switch (static_cast<int>(format))
	{
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		return 16;

	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		return 12;

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_Y416:
	case DXGI_FORMAT_Y210:
	case DXGI_FORMAT_Y216:
		return 8;

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DXGI_FORMAT_AYUV:
	case DXGI_FORMAT_Y410:
	case DXGI_FORMAT_YUY2:
		return 4;

	case DXGI_FORMAT_P010:
	case DXGI_FORMAT_P016:
		return 3;

	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_A8P8:
	case DXGI_FORMAT_B4G4R4A4_UNORM:
		return 2;

	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return 1;

	default:
		return 0;
	}
}

void Gpu_Buffer::free()
{
	data_size = 0;
	data_count = 0;
	resource.Reset();
}

u32 Gpu_Buffer::get_data_width()
{
	return data_count * data_size;
}

Input_Layout Gpu_Device::vertex_xc;
Input_Layout Gpu_Device::vertex_xnuv;
Input_Layout Gpu_Device::vertex_xuv;

void Gpu_Device::create_gpu_buffer(Gpu_Buffer_Desc *desc, Gpu_Buffer *buffer)
{
	assert(buffer);
	assert(desc->data_count > 0);
	assert(desc->data_size > 0);

	buffer->data_count = desc->data_count;
	buffer->data_size = desc->data_size;

	D3D11_BUFFER_DESC buffer_desc;
	ZeroMemory(&buffer_desc, sizeof(D3D11_BUFFER_DESC));
	buffer_desc.Usage = to_dx11_resource_usage(desc->usage);
	buffer_desc.BindFlags = desc->bind_flags;
	buffer_desc.ByteWidth = buffer->get_data_width();
	buffer_desc.CPUAccessFlags = desc->cpu_access;
	buffer_desc.MiscFlags = desc->misc_flags;
	buffer_desc.StructureByteStride = desc->struct_size;

	if (desc->data) {
		D3D11_SUBRESOURCE_DATA resource_data_desc;
		ZeroMemory(&resource_data_desc, sizeof(D3D11_SUBRESOURCE_DATA));
		resource_data_desc.pSysMem = (void *)desc->data;
		HR(dx11_device->CreateBuffer(&buffer_desc, &resource_data_desc, buffer->resource.ReleaseAndGetAddressOf()));
	} else {
		HR(dx11_device->CreateBuffer(&buffer_desc, NULL, buffer->resource.ReleaseAndGetAddressOf()));
	}
}

void Gpu_Device::create_constant_buffer(u32 data_size, Gpu_Buffer *buffer)
{
	assert(buffer);

	Gpu_Buffer_Desc buffer_desc;
	buffer_desc.data_count = 1;
	buffer_desc.data_size = data_size;
	buffer_desc.usage = RESOURCE_USAGE_DYNAMIC;
	buffer_desc.bind_flags = BIND_CONSTANT_BUFFER;
	buffer_desc.cpu_access = CPU_ACCESS_WRITE;
	create_gpu_buffer(&buffer_desc, buffer);
}

void Gpu_Device::create_sampler(Sampler_State *sampler)
{
	assert(sampler);

	D3D11_SAMPLER_DESC sampler_desc;
	ZeroMemory(&sampler_desc, sizeof(sampler_desc));
	sampler_desc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

	HR(dx11_device->CreateSamplerState(&sampler_desc, sampler->ReleaseAndGetAddressOf()));
}

void Gpu_Device::create_texture_2d(Texture_Desc *texture_desc, Texture2D *texture)
{
	assert(texture);
	assert(texture_desc);

	texture->width = texture_desc->width;
	texture->height = texture_desc->height;
	texture->format = texture_desc->format;
	texture->format_size = dxgi_format_size(texture_desc->format);
	texture->usage = texture_desc->usage;
	texture->multisampling = texture_desc->multisampling;
	texture->mip_levels = texture_desc->mip_levels;

	D3D11_TEXTURE2D_DESC texture_2d_desc;
	ZeroMemory(&texture_2d_desc, sizeof(D3D11_TEXTURE2D_DESC));
	texture_2d_desc.Width = texture_desc->width;
	texture_2d_desc.Height = texture_desc->height;
	texture_2d_desc.MipLevels = texture_desc->mip_levels;
	texture_2d_desc.ArraySize = texture_desc->array_count;
	texture_2d_desc.Format = texture_desc->format;
	texture_2d_desc.SampleDesc.Count = texture_desc->multisampling.count;
	texture_2d_desc.SampleDesc.Quality = texture_desc->multisampling.quality;
	texture_2d_desc.Usage = to_dx11_resource_usage(texture_desc->usage);
	texture_2d_desc.BindFlags = texture_desc->bind;
	texture_2d_desc.CPUAccessFlags = texture_desc->cpu_access;

	u32 is_support_mips;
	HR(dx11_device->CheckFormatSupport(texture_desc->format, &is_support_mips));
	if ((texture_desc->mip_levels != 1) && (is_support_mips & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN)) {
		texture_2d_desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
		texture_2d_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	}

	if (texture_desc->data && (texture_desc->mip_levels == 1)) {
		D3D11_SUBRESOURCE_DATA subresource_desc;
		ZeroMemory(&subresource_desc, sizeof(D3D11_SUBRESOURCE_DATA));
		subresource_desc.pSysMem = texture_desc->data;
		subresource_desc.SysMemPitch = texture_desc->width * texture->format_size;

		HR(dx11_device->CreateTexture2D(&texture_2d_desc, &subresource_desc, texture->resource.ReleaseAndGetAddressOf()));
	} else {
		HR(dx11_device->CreateTexture2D(&texture_2d_desc, NULL, texture->resource.ReleaseAndGetAddressOf()));
	}
}

void Gpu_Device::create_rasterizer_state(Rasterizer_Desc *rasterizer_desc, Rasterizer_State *rasterizer_state)
{
	HR(dx11_device->CreateRasterizerState(&rasterizer_desc->desc, rasterizer_state->ReleaseAndGetAddressOf()));
}

void Gpu_Device::create_blend_state(Blend_State_Desc *blending_desc, Blend_State *blend_state)
{	
	assert(blending_desc);
	assert(blend_state);
	
	D3D11_BLEND_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.AlphaToCoverageEnable = false;
	desc.RenderTarget[0].BlendEnable = blending_desc->enable;
	desc.RenderTarget[0].SrcBlend = to_dx11_blend(blending_desc->src);
	desc.RenderTarget[0].DestBlend = to_dx11_blend(blending_desc->dest);
	desc.RenderTarget[0].BlendOp = to_dx11_blend_op(blending_desc->blend_op);
	desc.RenderTarget[0].SrcBlendAlpha = to_dx11_blend(blending_desc->src_alpha);
	desc.RenderTarget[0].DestBlendAlpha = to_dx11_blend(blending_desc->dest_alpha);
	desc.RenderTarget[0].BlendOpAlpha = to_dx11_blend_op(blending_desc->blend_op_alpha);
	desc.RenderTarget[0].RenderTargetWriteMask = blending_desc->write_mask;

	HR(dx11_device->CreateBlendState(&desc, blend_state->ReleaseAndGetAddressOf()));
}

void Gpu_Device::create_depth_stencil_state(Depth_Stencil_State_Desc *depth_stencil_desc, Depth_Stencil_State *depth_stencil_state)
{
	assert(depth_stencil_desc);
	assert(depth_stencil_state);

	D3D11_DEPTH_STENCIL_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_DEPTH_STENCILOP_DESC));

	if (depth_stencil_desc->enable_depth_test) {
		desc.DepthEnable = true;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS;
	} else {
		desc.DepthEnable = false;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	}
	
	desc.StencilEnable = true;
	desc.StencilReadMask = depth_stencil_desc->stencil_read_mask;
	desc.StencilWriteMask = depth_stencil_desc->stencil_write_mack;

	desc.FrontFace.StencilFailOp = to_dx11_stencil_op(depth_stencil_desc->stencil_failed);
	desc.FrontFace.StencilDepthFailOp = to_dx11_stencil_op(depth_stencil_desc->depth_failed);
	desc.FrontFace.StencilPassOp = to_dx11_stencil_op(depth_stencil_desc->pass);
	desc.FrontFace.StencilFunc = to_dx11_comparison_func(depth_stencil_desc->compare_func);

	desc.BackFace.StencilFailOp = to_dx11_stencil_op(depth_stencil_desc->stencil_failed);
	desc.BackFace.StencilDepthFailOp = to_dx11_stencil_op(depth_stencil_desc->depth_failed);
	desc.BackFace.StencilPassOp = to_dx11_stencil_op(depth_stencil_desc->pass);
	desc.BackFace.StencilFunc = to_dx11_comparison_func(depth_stencil_desc->compare_func);

	HR(dx11_device->CreateDepthStencilState(&desc, depth_stencil_state->ReleaseAndGetAddressOf()));
}

inline DXGI_FORMAT to_depth_stencil_view_format(DXGI_FORMAT format)
{
	switch (format) {
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	assert(false);
	return DXGI_FORMAT_UNKNOWN;
}

void Gpu_Device::create_shader_resource_view(Gpu_Buffer *gpu_buffer)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
	ZeroMemory(&shader_resource_view_desc, sizeof(shader_resource_view_desc));
	shader_resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;
	shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	shader_resource_view_desc.Buffer.NumElements = gpu_buffer->data_count;

	HR(dx11_device->CreateShaderResourceView(gpu_buffer->resource.Get(), &shader_resource_view_desc, gpu_buffer->srv.ReleaseAndGetAddressOf()));
}

inline DXGI_FORMAT to_shader_resource_view_format(DXGI_FORMAT format)
{
	switch (format) {
		case DXGI_FORMAT_R24G8_TYPELESS:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}
	return format;
}

void Gpu_Device::create_shader_resource_view(Texture2D *texture)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
	ZeroMemory(&shader_resource_view_desc, sizeof(shader_resource_view_desc));
	shader_resource_view_desc.Format = to_shader_resource_view_format(texture->format);
	if (texture->is_multisampled()) {
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
	} else {
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shader_resource_view_desc.Texture2D.MipLevels = texture->mip_levels;
	}
	
	HR(dx11_device->CreateShaderResourceView(texture->resource.Get(), &shader_resource_view_desc, texture->srv.ReleaseAndGetAddressOf()));
}

void Gpu_Device::create_depth_stencil_view(Texture2D *texture)
{
	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc;
	ZeroMemory(&depth_stencil_view_desc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	depth_stencil_view_desc.Format = to_depth_stencil_view_format(texture->format);
	depth_stencil_view_desc.ViewDimension = texture->is_multisampled() ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
	HR(dx11_device->CreateDepthStencilView(texture->resource.Get(), &depth_stencil_view_desc, texture->dsv.ReleaseAndGetAddressOf()));
}

void Gpu_Device::create_render_target_view(Texture2D *texture)
{
	HR(dx11_device->CreateRenderTargetView(texture->resource.Get(), NULL, texture->rtv.ReleaseAndGetAddressOf()));
}

void Gpu_Device::create_unordered_access_view(Texture2D *texture)
{
	assert(texture->is_multisampled());
	
	D3D11_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc;
	ZeroMemory(&unordered_access_view_desc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
	unordered_access_view_desc.Format = texture->format;
	unordered_access_view_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

	HR(dx11_device->CreateUnorderedAccessView(texture->resource.Get(), &unordered_access_view_desc, texture->uav.ReleaseAndGetAddressOf()));
}

void Gpu_Device::create_input_layouts(Hash_Table<String, Shader *> &shader_table)
{
	const D3D11_INPUT_ELEMENT_DESC vertex_xuv_desc[2] = {
	{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{"TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	const D3D11_INPUT_ELEMENT_DESC vertex_xc_desc[2] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	const D3D11_INPUT_ELEMENT_DESC vertex_xnuv_desc[3] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	Shader *forward_light = shader_table["forward_light.hlsl"];
	Shader *text = shader_table["draw_text.hlsl"];

	HR(dx11_device->CreateInputLayout(vertex_xnuv_desc, 3, (void *)forward_light->byte_code, forward_light->byte_code_size, Gpu_Device::vertex_xnuv.ReleaseAndGetAddressOf()));
	HR(dx11_device->CreateInputLayout(vertex_xuv_desc, 2, (void *)text->byte_code, text->byte_code_size, Gpu_Device::vertex_xuv.ReleaseAndGetAddressOf()));
}

void Gpu_Device::create_shader(u8 *byte_code, u32 byte_code_size, Shader_Type shader_type, Shader *shader)
{
	assert(shader);
	switch (shader_type) {
		case VERTEX_SHADER: {
			HR(dx11_device->CreateVertexShader((void *)byte_code, byte_code_size, NULL, shader->vertex_shader.ReleaseAndGetAddressOf()));
			break;
		}
		case GEOMETRY_SHADER: {
			HR(dx11_device->CreateGeometryShader((void *)byte_code, byte_code_size, NULL, shader->geometry_shader.ReleaseAndGetAddressOf()));
			break;
		}
		case COMPUTE_SHADER: {
			HR(dx11_device->CreateComputeShader((void *)byte_code, byte_code_size, NULL, shader->compute_shader.ReleaseAndGetAddressOf()));
			break;
		}
		case HULL_SHADER: {
			HR(dx11_device->CreateHullShader((void *)byte_code, byte_code_size, NULL, shader->hull_shader.ReleaseAndGetAddressOf()));
			break;
		}
		case DOMAIN_SHADER: {
			HR(dx11_device->CreateDomainShader((void *)byte_code, byte_code_size, NULL, shader->domain_shader.ReleaseAndGetAddressOf()));
			break;
		}
		case PIXEL_SHADER: {
			HR(dx11_device->CreatePixelShader((void *)byte_code, byte_code_size, NULL, shader->pixel_shader.ReleaseAndGetAddressOf()));
			break;
		}
	}
}

void Render_Pipeline_State::setup_default_state(Render_System *render_sys)
{
	blend_state = render_sys->render_pipeline_states.default_blend_state;
	depth_stencil_state = render_sys->render_pipeline_states.default_depth_stencil_state;
	rasterizer_state = render_sys->render_pipeline_states.default_rasterizer_state;
	sampler_state = render_sys->render_pipeline_states.default_sampler_state;
	view_port.width = Render_System::screen_width;
	view_port.height = Render_System::screen_height;
}

void Render_Pipeline::apply(Render_Pipeline_State *render_pipeline_state)
{
	set_primitive(render_pipeline_state->primitive_type);
	set_input_layout(NULL);
	set_vertex_buffer(NULL);
	set_index_buffer(NULL);

	set_vertex_shader(render_pipeline_state->shader);

	set_blend_state(render_pipeline_state->blend_state);
	set_depth_stencil_state(render_pipeline_state->depth_stencil_state);

	set_rasterizer_state(render_pipeline_state->rasterizer_state);
	set_view_port(&render_pipeline_state->view_port);

	set_pixel_shader_sampler(render_pipeline_state->sampler_state);
	set_pixel_shader(render_pipeline_state->shader);

	set_render_target(render_pipeline_state->render_target_view, render_pipeline_state->depth_stencil_view);
}

void Render_Pipeline::update_constant_buffer(Gpu_Buffer *gpu_buffer, void *data)
{
	assert(gpu_buffer);
	assert(data);

	void *buffer_data = map(*gpu_buffer);
	memcpy(buffer_data, data, gpu_buffer->data_size);
	unmap(*gpu_buffer);
}

void Render_Pipeline::update_subresource(Texture2D *texture, void *data, u32 row_pitch, Rect_u32 *rect)
{
	assert(texture);
	assert(data);

	if (rect) {
		D3D11_BOX box = { rect->x, rect->y, 0, rect->right(), rect->bottom(), 1 };
		dx11_context->UpdateSubresource(texture->resource.Get(), 0, &box, (const void *)data, row_pitch, 0);
	} else {
		dx11_context->UpdateSubresource(texture->resource.Get(), 0, NULL, (const void *)data, row_pitch, 0);
	}
}

void Render_Pipeline::generate_mips(const Shader_Resource_View &shader_resource)
{
	dx11_context->GenerateMips(shader_resource.Get());
}

void Render_Pipeline::set_input_layout(void *pointer)
{
	dx11_context->IASetInputLayout(NULL);
}

void Render_Pipeline::set_input_layout(const Input_Layout &input_layout)
{
	dx11_context->IASetInputLayout(input_layout.Get());
}

void Render_Pipeline::set_primitive(Render_Primitive_Type primitive_type)
{
	dx11_context->IASetPrimitiveTopology(to_dx11_primitive_type(primitive_type));
}

void Render_Pipeline::set_vertex_buffer(Gpu_Buffer *gpu_buffer)
{
	u32 offsets = 0;
	u32 strides = 0;
	if (gpu_buffer) {
		strides = gpu_buffer->data_size;
		dx11_context->IASetVertexBuffers(0, 1, gpu_buffer->resource.GetAddressOf(), &strides, &offsets);
	} else {
		dx11_context->IASetVertexBuffers(0, 0, NULL, &strides, &offsets);
	}
}

void Render_Pipeline::set_index_buffer(Gpu_Buffer *gpu_buffer)
{
	if (gpu_buffer) {
		dx11_context->IASetIndexBuffer(gpu_buffer->resource.Get(), DXGI_FORMAT_R32_UINT, 0);
	} else {
		dx11_context->IASetIndexBuffer(NULL, DXGI_FORMAT_R32_UINT, 0);
	}
}

void Render_Pipeline::set_vertex_shader(Shader *shader)
{
	assert(shader);
	dx11_context->VSSetShader(shader->vertex_shader.Get(), 0, 0);
}

void Render_Pipeline::set_geometry_shader(Shader *shader)
{
	assert(shader);
	dx11_context->GSSetShader(shader->geometry_shader.Get(), 0, 0);
}

void Render_Pipeline::set_compute_shader(Shader *shader)
{
	assert(shader);
	dx11_context->CSSetShader(shader->compute_shader.Get(), 0, 0);
}

void Render_Pipeline::set_hull_shader(Shader *shader)
{
	assert(shader);
	dx11_context->HSSetShader(shader->hull_shader.Get(), 0, 0);
}

void Render_Pipeline::set_domain_shader(Shader *shader)
{
	assert(shader);
	dx11_context->DSSetShader(shader->domain_shader.Get(), 0, 0);
}

void Render_Pipeline::set_pixel_shader(Shader *shader)
{
	assert(shader);
	dx11_context->PSSetShader(shader->pixel_shader.Get(), 0, 0);
}

void Render_Pipeline::set_vertex_shader_resource(u32 gpu_register, const Gpu_Buffer &constant_buffer)
{
	dx11_context->VSSetConstantBuffers(gpu_register, 1, constant_buffer.resource.GetAddressOf());
}

void Render_Pipeline::set_vertex_shader_resource(u32 gpu_register, const Shader_Resource_View &shader_resource)
{
	dx11_context->VSSetShaderResources(gpu_register, 1, shader_resource.GetAddressOf());
}

void Render_Pipeline::set_vertex_shader_resource(u32 shader_resource_register, const Struct_Buffer &struct_buffer)
{
	dx11_context->VSSetShaderResources(shader_resource_register, 1, struct_buffer.gpu_buffer.srv.GetAddressOf());
}

void Render_Pipeline::set_pixel_shader_sampler(const Sampler_State &sampler_state)
{
	dx11_context->PSSetSamplers(0, 1, sampler_state.GetAddressOf());
}

void Render_Pipeline::set_pixel_shader_resource(u32 gpu_register, const Gpu_Buffer &constant_buffer)
{
	dx11_context->PSSetConstantBuffers(gpu_register, 1, constant_buffer.resource.GetAddressOf());
}

void Render_Pipeline::set_pixel_shader_resource(u32 shader_resource_register, const Shader_Resource_View &shader_resource_view)
{
	dx11_context->PSSetShaderResources(shader_resource_register, 1, shader_resource_view.GetAddressOf());
}

void Render_Pipeline::set_pixel_shader_resource(u32 shader_resource_register, const Struct_Buffer &struct_buffer)
{
	dx11_context->PSSetShaderResources(shader_resource_register, 1, struct_buffer.gpu_buffer.srv.GetAddressOf());
}

void Render_Pipeline::set_rasterizer_state(const Rasterizer_State &rasterizer_state)
{
	dx11_context->RSSetState(rasterizer_state.Get());
}

void Render_Pipeline::set_scissor(Rect_s32 *rect)
{
	D3D11_RECT rects[1];
	rects[0].left = rect->x;
	rects[0].right = rect->right();
	rects[0].top = rect->y;
	rects[0].bottom = rect->bottom();

	dx11_context->RSSetScissorRects(1, rects);
}

void Render_Pipeline::set_view_port(View_Port *view_port)
{
	D3D11_VIEWPORT dx11_view_port;
	dx11_view_port.TopLeftX = (float)view_port->x;
	dx11_view_port.TopLeftY = (float)view_port->y;
	dx11_view_port.Width = (float)view_port->width;
	dx11_view_port.Height = (float)view_port->height;
	dx11_view_port.MinDepth = (float)view_port->min_depth;
	dx11_view_port.MaxDepth = (float)view_port->max_depth;

	dx11_context->RSSetViewports(1, &dx11_view_port);
}

void Render_Pipeline::reset_rasterizer()
{
	dx11_context->RSSetState(0);
}

void Render_Pipeline::set_blend_state(const Blend_State &blend_state)
{
	float b[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	dx11_context->OMSetBlendState(blend_state.Get(), b, 0xffffffff);
}

void Render_Pipeline::reset_blending_state()
{
	float b[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	dx11_context->OMSetBlendState(0, b, 0xffffffff);
}

void Render_Pipeline::set_depth_stencil_state(const Depth_Stencil_State &depth_stencil_state, u32 stencil_ref)
{
	dx11_context->OMSetDepthStencilState(depth_stencil_state.Get(), stencil_ref);
}

void Render_Pipeline::reset_depth_stencil_state()
{
	dx11_context->OMSetDepthStencilState(NULL, 0);
}

void Render_Pipeline::set_render_target(const Render_Target_View &render_target_view, const Depth_Stencil_View &depth_stencil_view)
{
	u32 render_target_count = 0;
	if (render_target_view) {
		render_target_count = 1;
	}
	dx11_context->OMSetRenderTargets(render_target_count, render_target_view.GetAddressOf(), depth_stencil_view.Get());
}

void Render_Pipeline::reset_vertex_buffer()
{
	u32 strides = 0;
	u32 offsets = 0;
	dx11_context->IASetVertexBuffers(0, 0, NULL, &strides, &offsets);
}

void Render_Pipeline::reset_index_buffer()
{
	dx11_context->IASetIndexBuffer(NULL, DXGI_FORMAT_R32_UINT, 0);
}

void Render_Pipeline::draw(u32 vertex_count)
{
	dx11_context->Draw(vertex_count, 0);
}

void Render_Pipeline::draw_indexed(u32 index_count, u32 index_offset, u32 vertex_offset)
{
	dx11_context->DrawIndexed(index_count, index_offset, vertex_offset);
}

Gpu_Buffer_Desc make_index_buffer_desc(u32 data_count, void *data, Resource_Usage usage, u32 cpu_access)
{
	Gpu_Buffer_Desc gpu_buffer_desc;
	gpu_buffer_desc.data_count = data_count;
	gpu_buffer_desc.data_size = sizeof(u32);
	gpu_buffer_desc.data = data;
	gpu_buffer_desc.usage = usage;
	gpu_buffer_desc.cpu_access = cpu_access;
	gpu_buffer_desc.bind_flags = BIND_INDEX_BUFFER;
	return gpu_buffer_desc;
}

Rasterizer_Desc::Rasterizer_Desc()
{
	desc.FillMode = D3D11_FILL_SOLID;
	desc.CullMode = D3D11_CULL_BACK;
	desc.FrontCounterClockwise = false;
	desc.DepthBias = false;
	desc.DepthBiasClamp = 0;
	desc.SlopeScaledDepthBias = 0;
	desc.DepthClipEnable = true;
	desc.ScissorEnable = false;
	desc.MultisampleEnable = false;
	desc.AntialiasedLineEnable = false;
}

void Rasterizer_Desc::set_sciccor(bool state)
{
	desc.ScissorEnable = state;
}

void Rasterizer_Desc::set_counter_clockwise(bool state)
{
	desc.FrontCounterClockwise = state;
}

Blend_State_Desc::Blend_State_Desc()
{
	enable = true;
	src = BLEND_ONE;
	dest = BLEND_ZERO;
	blend_op = BLEND_OP_ADD;
	src_alpha = BLEND_ONE;
	dest_alpha = BLEND_ZERO;
	blend_op_alpha = BLEND_OP_ADD;
	write_mask = D3D11_COLOR_WRITE_ENABLE_ALL;
}

bool Texture2D::is_multisampled()
{
	return multisampling.count > 1;
}

u32 Texture2D::get_pitch()
{
	return width * format_size;
}

u32 Texture2D::get_size()
{
	return width * height * format_size;
}

void Texture2D::release()
{
	Gpu_Resource::release();
	srv.Reset();
}

void init_render_api(Gpu_Device *gpu_device, Render_Pipeline *render_pipeline)
{
	print("[Initialze render API]");
	UINT create_device_flag = 0;

#if defined(DEBUG) || defined(_DEBUG)  
	create_device_flag |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL feature_level;

	HRESULT hr = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, create_device_flag, 0, 0, D3D11_SDK_VERSION, &gpu_device->dx11_device, &feature_level, &render_pipeline->dx11_context);
	if (FAILED(hr)) {
		error("D3D11CreateDevice Failed.");
	}

	if (feature_level < D3D_FEATURE_LEVEL_11_0) {
		error("Direct3D Feature Level 11 unsupported.");
	}
}

void setup_multisampling(Gpu_Device *gpu_device, Multisample_Info *multisample_info)
{
	assert(gpu_device);
	assert(multisample_info);


	HRESULT result = gpu_device->dx11_device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, multisample_info->count, &multisample_info->quality);
	if (FAILED(result) || (multisample_info->quality == 0)) {
		u32 quality_levels = 0;
		u32 multisampling_count = 0;
		Multisample_Info available_multisamplings[128];
		for (u32 sample_count = 1; sample_count <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; sample_count *= 2) {
			HRESULT result = gpu_device->dx11_device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, sample_count, &quality_levels);
			if (FAILED(result) || (quality_levels == 0)) {
				continue;
			}
			quality_levels = quality_levels > 0 ? quality_levels - 1 : 0;
			available_multisamplings[multisampling_count++] = { sample_count, quality_levels };
			print("init_render_api: An adapter supports multisample sample count {} and quality levels {} for DXGI_FORMAT_R8G8B8A8_UNORM.", sample_count, quality_levels);
		}

		if (multisampling_count == 0) {
			default_render_api_multisample.count = 0;
			default_render_api_multisample.quality = 0;
			print("There is no available multisample quality levels on an adapter.");
		}

		if (multisampling_count == 1) {
			default_render_api_multisample = available_multisamplings[1];
		}

		u32 index = 1;
		for (; index < multisampling_count; index++) {
			if (available_multisamplings[index].count > multisample_info->count) {
				default_render_api_multisample = available_multisamplings[index - 1];
				return;
			}
		}
		default_render_api_multisample = available_multisamplings[index - 1];
	} else {
		multisample_info->quality -= 1;
		default_render_api_multisample = *multisample_info;
	}
}

void Swap_Chain::init(Gpu_Device *gpu_device, Win32_Info *win32_info)
{
	multisampling = default_render_api_multisample;

	DXGI_SWAP_CHAIN_DESC swap_chain_desc;
	swap_chain_desc.BufferDesc.Width = win32_info->window_width;
	swap_chain_desc.BufferDesc.Height = win32_info->window_height;
	swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swap_chain_desc.SampleDesc.Count = default_render_api_multisample.count;
	swap_chain_desc.SampleDesc.Quality = default_render_api_multisample.quality;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount = 1;
	swap_chain_desc.OutputWindow = win32_info->window;
	swap_chain_desc.Windowed = true;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swap_chain_desc.Flags = 0;

	ComPtr<IDXGIDevice> dxgi_device;
	ComPtr<IDXGIAdapter> dxgi_adapter;
	ComPtr<IDXGIFactory> dxgi_factory;

	HR(gpu_device->dx11_device->QueryInterface(__uuidof(IDXGIDevice), (void**)dxgi_device.ReleaseAndGetAddressOf()));
	HR(dxgi_device->GetParent(__uuidof(IDXGIAdapter), (void**)dxgi_adapter.ReleaseAndGetAddressOf()));
	HR(dxgi_adapter->GetParent(__uuidof(IDXGIFactory), (void**)dxgi_factory.ReleaseAndGetAddressOf()));

	HR(dxgi_factory->CreateSwapChain(gpu_device->dx11_device.Get(), &swap_chain_desc, dxgi_swap_chain.ReleaseAndGetAddressOf()));
}

void Swap_Chain::resize(u32 window_width, u32 window_height)
{
	HR(dxgi_swap_chain->ResizeBuffers(1, window_width, window_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
}

void Swap_Chain::get_back_buffer_as_texture(Texture2D *texture)
{
	HR(dxgi_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(texture->resource.GetAddressOf())));
}
