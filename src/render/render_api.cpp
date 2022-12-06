#include "render_api.h"
#include "../sys/sys_local.h"
#include "../libs/str.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
//#include <D3DCompiler.inl>


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
}

inline u32 size_of_dxgi_format(DXGI_FORMAT format)
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

//FORCEINLINE HRESULT D3D11Reflect(_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData, _In_ SIZE_T SrcDataSize, _Out_ ID3D11ShaderReflection** ppReflector)
//{
//	return D3DReflect(pSrcData, SrcDataSize, IID_ID3D11ShaderReflection, (void**)ppReflector);
//}

Gpu_Buffer::~Gpu_Buffer()
{
	RELEASE_COM(buffer);
}

u32 Gpu_Buffer::get_data_width()
{
	return data_count * data_size;
}

ID3D11Buffer **Gpu_Buffer::get_buffer_ptr()
{
	return &buffer;;
}

Input_Layout *Gpu_Device::vertex_xc = NULL;
Input_Layout *Gpu_Device::vertex_xnuv;
Input_Layout *Gpu_Device::vertex_xuv;


void Gpu_Device::shutdown()
{
	RELEASE_COM(device);
	RELEASE_COM(Gpu_Device::vertex_xc);
	RELEASE_COM(Gpu_Device::vertex_xnuv);
	RELEASE_COM(Gpu_Device::vertex_xuv);
}

Gpu_Buffer *Gpu_Device::create_gpu_buffer(Gpu_Buffer_Desc *desc)
{
	assert(desc->data_count > 0);
	assert(desc->data_size > 0);

	Gpu_Buffer *buffer = new Gpu_Buffer();

	buffer->data_count = desc->data_count;
	buffer->data_size = desc->data_size;

	D3D11_BUFFER_DESC buffer_desc;
	ZeroMemory(&buffer_desc, sizeof(D3D11_BUFFER_DESC));
	buffer_desc.Usage = to_dx11_resource_usage(desc->usage);
	buffer_desc.BindFlags = desc->bind_flags;
	buffer_desc.ByteWidth = buffer->get_data_width();
	buffer_desc.CPUAccessFlags = desc->cpu_access;

	if (desc->data) {
		D3D11_SUBRESOURCE_DATA resource_data_desc;
		ZeroMemory(&resource_data_desc, sizeof(D3D11_SUBRESOURCE_DATA));
		resource_data_desc.pSysMem = (void *)desc->data;
		HR(device->CreateBuffer(&buffer_desc, &resource_data_desc, buffer->get_buffer_ptr()));
	} else {
		HR(device->CreateBuffer(&buffer_desc, NULL, buffer->get_buffer_ptr()));
	}
	return buffer;
}

Gpu_Buffer *Gpu_Device::create_constant_buffer(u32 data_size)
{
	Gpu_Buffer_Desc buffer_desc;
	buffer_desc.data_count = 1;
	buffer_desc.data_size = data_size;
	buffer_desc.usage = RESOURCE_USAGE_DYNAMIC;
	buffer_desc.bind_flags = BIND_CONSTANT_BUFFER;
	buffer_desc.cpu_access = CPU_ACCESS_WRITE;

	return create_gpu_buffer(&buffer_desc);
}

Texture_Sampler *Gpu_Device::create_sampler()
{
	Texture_Sampler *sampler = NULL;
	D3D11_SAMPLER_DESC sampler_desc;
	ZeroMemory(&sampler_desc, sizeof(sampler_desc));
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

	HR(device->CreateSamplerState(&sampler_desc, &sampler));
	return sampler;
}

Texture *Gpu_Device::create_texture_2d(u32 width, u32 height, void *data, u32 mip_levels, Resource_Usage usage, DXGI_FORMAT format)
{
	Texture *texture = new Texture();
	texture->width = width;
	texture->height = height;
	texture->format_size = size_of_dxgi_format(format);

	D3D11_TEXTURE2D_DESC texture_2d_desc;
	ZeroMemory(&texture_2d_desc, sizeof(D3D11_TEXTURE2D_DESC));
	texture_2d_desc.Width = width;
	texture_2d_desc.Height = height;
	texture_2d_desc.MipLevels = mip_levels;
	texture_2d_desc.ArraySize = 1;
	texture_2d_desc.Format = format;
	texture_2d_desc.SampleDesc.Count = 1;
	texture_2d_desc.Usage = to_dx11_resource_usage(usage);
	texture_2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texture_2d_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	u32 is_support_mips;
	HR(device->CheckFormatSupport(format, &is_support_mips));
	if ((mip_levels == 0) && (is_support_mips & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN)) {
		texture_2d_desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
		texture_2d_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	}

	if (data && (mip_levels == 1)) {
		D3D11_SUBRESOURCE_DATA subresource_desc;
		ZeroMemory(&subresource_desc, sizeof(D3D11_SUBRESOURCE_DATA));
		subresource_desc.pSysMem = data;
		subresource_desc.SysMemPitch = width * texture->format_size;

		HR(device->CreateTexture2D(&texture_2d_desc, &subresource_desc, (ID3D11Texture2D **)&texture->gpu_resource))
	} else {
		HR(device->CreateTexture2D(&texture_2d_desc, NULL, (ID3D11Texture2D **)&texture->gpu_resource));
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_desc;
	shader_resource_desc.Format = format;
	shader_resource_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shader_resource_desc.Texture2D.MostDetailedMip = 0;
	shader_resource_desc.Texture2D.MipLevels = mip_levels == 1 ? 1 : (mip_levels == 0 ? -1 : mip_levels);

	HR(device->CreateShaderResourceView(texture->gpu_resource, &shader_resource_desc, &texture->shader_resource));
	
	return texture;
}

Rasterizer *Gpu_Device::create_rasterizer(Rasterizer_Desc *rasterizer_desc)
{
	Rasterizer *rasterizer = NULL;
	HR(device->CreateRasterizerState(&rasterizer_desc->desc, &rasterizer));
	return rasterizer;
}

Blending_Test *Gpu_Device::create_blending_test(Blending_Test_Desc *blending_desc)
{
	D3D11_BLEND_DESC desc12;
	ZeroMemory(&desc12, sizeof(desc12));
	desc12.AlphaToCoverageEnable = false;
	desc12.RenderTarget[0].BlendEnable = true;
	desc12.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	desc12.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	desc12.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	desc12.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	desc12.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	desc12.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	desc12.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	
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

	Blending_Test *blending = NULL;
	HR(device->CreateBlendState(&desc, &blending));
	return blending;
}

Depth_Stencil_Test *Gpu_Device::create_depth_stencil_test(Depth_Stencil_Test_Desc *depth_stencil_desc)
{
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

	Depth_Stencil_Test *depth_stencil_test = NULL;
	HR(device->CreateDepthStencilState(&desc, &depth_stencil_test));
	return depth_stencil_test;
}

void Gpu_Device::create_input_layouts(Hash_Table<String, Shader *> &shaders)
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

	Shader *forward_light = shaders["forward_light"];
	Shader *text = shaders["draw_text"];

	//HR(directx11.device->CreateInputLayout(vertex_col_desc, 2, (void *)render_2d->byte_code, render_2d->byte_code_size, &vertex_color));
	HR(device->CreateInputLayout(vertex_xnuv_desc, 3, (void *)forward_light->byte_code, forward_light->byte_code_size, &Gpu_Device::vertex_xnuv));
	HR(device->CreateInputLayout(vertex_xuv_desc, 2, (void *)text->byte_code, text->byte_code_size, &Gpu_Device::vertex_xuv));
}

//#include "d3dcompiler.h"
//FORCEINLINE HRESULT
//D3D11Reflect(_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData,
//	_In_ SIZE_T SrcDataSize,
//	_Out_ ID3D11ShaderReflection** ppReflector)
//{
//	return D3DReflect(pSrcData, SrcDataSize,
//		IID_ID3D11ShaderReflection, (void**)ppReflector);
//}

void Gpu_Device::create_shader(u8 * byte_code, u32 byte_code_size, Shader_Type shader_type, Shader * shader)
{
	switch (shader_type) {
	case VERTEX_SHADER:
	{
		HR(device->CreateVertexShader((void *)byte_code, byte_code_size, NULL, &shader->vertex_shader));

		//print("{}.hlsl: Vertex shader", shader->name);

		//ID3D11ShaderReflection* pReflector = NULL;
		//D3D11Reflect((void *)byte_code, byte_code_size, &pReflector);

		//D3D11_SHADER_DESC shaderDesc;
		//pReflector->GetDesc(&shaderDesc);
		//u32 buffer_count = shaderDesc.ConstantBuffers;


		//for (u32 i = 0; i < buffer_count; i++) {
		//	ID3D11ShaderReflectionConstantBuffer* cbuffer = pReflector->GetConstantBufferByIndex(i);
		//	D3D11_SHADER_BUFFER_DESC sbuffer;
		//	cbuffer->GetDesc(&sbuffer);

		//	String type;
		//	switch (sbuffer.Type)
		//	{
		//	case D3D11_CT_CBUFFER:
		//		type = "Consttanc buffer";
		//		break;
		//	case D3D11_CT_TBUFFER:
		//		type = "Texture buffer";
		//		break;
		//	case D3D11_CT_INTERFACE_POINTERS:
		//		type = "Interface buffer";
		//		break;
		//	case D3D11_CT_RESOURCE_BIND_INFO:
		//		type = "Bind buffer";
		//		break;
		//	default:
		//		break;
		//	}
		//	print("	Index [{}] buffer name [{}]", i, sbuffer.Name);

		//}
		break;
	}
	case GEOMETRY_SHADER:
	{
		HR(device->CreateGeometryShader((void *)byte_code, byte_code_size, NULL, &shader->geometry_shader));
		break;
	}
	case COMPUTE_SHADER:
	{
		HR(device->CreateComputeShader((void *)byte_code, byte_code_size, NULL, &shader->compute_shader));
		break;
	}
	case HULL_SHADER:
	{
		HR(device->CreateHullShader((void *)byte_code, byte_code_size, NULL, &shader->hull_shader));
		break;
	}
	case DOMAIN_SHADER:
	{
		HR(device->CreateDomainShader((void *)byte_code, byte_code_size, NULL, &shader->domain_shader));
		break;
	}
	case PIXEL_SHADER:
	{
		HR(device->CreatePixelShader((void *)byte_code, byte_code_size, NULL, &shader->pixel_shader));

		print("{}.hlsl: Pixel Shader", shader->name);

		//ID3D11ShaderReflection* pReflector = NULL;
		//D3D11Reflect((void *)byte_code, byte_code_size, &pReflector);

		//D3D11_SHADER_DESC shaderDesc;
		//pReflector->GetDesc(&shaderDesc);
		//u32 buffer_count = shaderDesc.ConstantBuffers;


		//for (u32 i = 0; i < buffer_count; i++) {
		//	ID3D11ShaderReflectionConstantBuffer* cbuffer = pReflector->GetConstantBufferByIndex(i);
		//	D3D11_SHADER_BUFFER_DESC sbuffer;
		//	cbuffer->GetDesc(&sbuffer);

		//	String type;
		//	switch (sbuffer.Type)
		//	{
		//	case D3D11_CT_CBUFFER:
		//		type = "Consttanc buffer";
		//		break;
		//	case D3D11_CT_TBUFFER:
		//		type = "Texture buffer";
		//		break;
		//	case D3D11_CT_INTERFACE_POINTERS:
		//		type = "Interface buffer";
		//		break;
		//	case D3D11_CT_RESOURCE_BIND_INFO:
		//		type = "Bind buffer";
		//		break;
		//	default:
		//		break;
		//	}
		//	print("	Index [{}] buffer name [{}]", i, sbuffer.Name);
		//}

		//u32 bind_count = shaderDesc.BoundResources;
		//for (u32 i = 0; i < bind_count; i++) {
		//	D3D11_SHADER_INPUT_BIND_DESC bind_desc;
		//	pReflector->GetResourceBindingDesc(i, &bind_desc);
		//	print("	Bind Resource: Name [{}]", bind_desc.Name);

		//}
		break;
	}
	}
}

void Render_Pipeline::resize(Gpu_Device *gpu_device, u32 window_width, u32 window_height)
{
	assert(gpu_device->device);
	assert(pipeline);
	assert(swap_chain);

	if ((window_width == 0) || (window_height == 0)) {
		return;
	}

	RELEASE_COM(render_target_view);
	RELEASE_COM(depth_stencil_view);
	RELEASE_COM(depth_stencil_buffer);

	HR(swap_chain->ResizeBuffers(1, window_width, window_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));

	ID3D11Texture2D* back_buffer = NULL;
	HR(swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer)));
	HR(gpu_device->device->CreateRenderTargetView(back_buffer, 0, &render_target_view));

	RELEASE_COM(back_buffer);

	// Create the depth/stencil width and view.

	D3D11_TEXTURE2D_DESC depth_stencil_desc;

	depth_stencil_desc.Width = window_width;
	depth_stencil_desc.Height = window_height;
	depth_stencil_desc.MipLevels = 1;
	depth_stencil_desc.ArraySize = 1;
	depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Use 4X MSAA? --must match swap chain MSAA values.
	if (true) {
		depth_stencil_desc.SampleDesc.Count = 4;
		depth_stencil_desc.SampleDesc.Quality = gpu_device->quality_levels - 1;
	} else {
		depth_stencil_desc.SampleDesc.Count = 1;
		depth_stencil_desc.SampleDesc.Quality = 0;
	}

	depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depth_stencil_desc.CPUAccessFlags = 0;
	depth_stencil_desc.MiscFlags = 0;

	HR(gpu_device->device->CreateTexture2D(&depth_stencil_desc, 0, &depth_stencil_buffer));
	HR(gpu_device->device->CreateDepthStencilView(depth_stencil_buffer, 0, &depth_stencil_view));


	// Bind the render target view and depth/stencil view to the pipeline.

	pipeline->OMSetRenderTargets(1, &render_target_view, depth_stencil_view);


	// Set the viewport transform.

	D3D11_VIEWPORT mScreenViewport;
	mScreenViewport.Width = static_cast<float>(window_width);
	mScreenViewport.Height = static_cast<float>(window_height);
	mScreenViewport.MaxDepth = 1.0f;
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;

	pipeline->RSSetViewports(1, &mScreenViewport);
}

void Render_Pipeline::shutdown()
{
}

void *Render_Pipeline::map(Gpu_Buffer *gpu_buffer)
{
	D3D11_MAPPED_SUBRESOURCE subresource;
	HR(pipeline->Map(gpu_buffer->buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource));
	return subresource.pData;
}

void Render_Pipeline::unmap(Gpu_Buffer *gpu_buffer)
{
	pipeline->Unmap(gpu_buffer->buffer, 0);
}

void Render_Pipeline::update_constant_buffer(Gpu_Buffer *gpu_buffer, void *data)
{
	assert(gpu_buffer);
	assert(data);

	void *buffer_data = map(gpu_buffer);

	memcpy(buffer_data, data, gpu_buffer->data_size);

	unmap(gpu_buffer);
}

void Render_Pipeline::update_subresource(Texture *texture, void *data, u32 row_pitch, Rect_u32 *rect)
{
	if (rect) {
		D3D11_BOX box;
		box.left = rect->x;
		box.right = rect->x + rect->width;
		box.top = rect->y;
		box.bottom = rect->y + rect->height;
		box.front = 0;
		box.back = 1;
		pipeline->UpdateSubresource(texture->gpu_resource, 0, &box, (const void *)data, row_pitch, 0);
		return;
	}
	pipeline->UpdateSubresource(texture->gpu_resource, 0, NULL, (const void *)data, row_pitch, 0);
}

void Render_Pipeline::generate_mips(Shader_Resource_View *shader_resource)
{
	pipeline->GenerateMips(shader_resource);
}

void Render_Pipeline::set_input_layout(Input_Layout *input_layout)
{
	pipeline->IASetInputLayout(input_layout);
}

void Render_Pipeline::set_primitive(Render_Primitive_Type primitive_type)
{
	pipeline->IASetPrimitiveTopology(to_dx11_primitive_type(primitive_type));
}

void Render_Pipeline::set_vertex_buffer(Gpu_Buffer *gpu_buffer)
{
	u32 strides = gpu_buffer->data_size;
	u32 offsets = 0;
	pipeline->IASetVertexBuffers(0, 1, gpu_buffer->get_buffer_ptr(), &strides, &offsets);
}

void Render_Pipeline::set_index_buffer(Gpu_Buffer *gpu_buffer)
{
	pipeline->IASetIndexBuffer(gpu_buffer->buffer, DXGI_FORMAT_R32_UINT, 0);
}

void Render_Pipeline::set_vertex_shader(Shader *shader)
{
	assert(shader->vertex_shader);
	pipeline->VSSetShader(shader->vertex_shader, 0, 0);
}

void Render_Pipeline::set_geometry_shader(Shader *shader)
{
	assert(shader->geometry_shader);
	pipeline->GSSetShader(shader->geometry_shader, 0, 0);
}

void Render_Pipeline::set_computer_shader(Shader *shader)
{
	assert(shader->compute_shader);
	pipeline->CSSetShader(shader->compute_shader, 0, 0);
}

void Render_Pipeline::set_hull_shader(Shader *shader)
{
	assert(shader->hull_shader);
	pipeline->HSSetShader(shader->hull_shader, 0, 0);
}

void Render_Pipeline::set_domain_shader(Shader *shader)
{
	assert(shader->domain_shader);
	pipeline->DSSetShader(shader->domain_shader, 0, 0);
}

void Render_Pipeline::set_pixel_shader(Shader *shader)
{
	assert(shader->pixel_shader);
	pipeline->PSSetShader(shader->pixel_shader, 0, 0);
}

void Render_Pipeline::set_veretex_shader_resource(Gpu_Buffer *constant_buffer)
{
	pipeline->VSSetConstantBuffers(0, 1, &constant_buffer->buffer);
}

void Render_Pipeline::set_pixel_shader_sampler(Texture_Sampler *sampler)
{
	pipeline->PSSetSamplers(0, 1, &sampler);
}

void Render_Pipeline::set_pixel_shader_resource(Gpu_Buffer *constant_buffer)
{
	pipeline->PSSetConstantBuffers(0, 1, &constant_buffer->buffer);
}

void Render_Pipeline::set_pixel_shader_resource(Shader_Resource_View *shader_resource_view)
{
	pipeline->PSSetShaderResources(0, 1, &shader_resource_view);
}

void Render_Pipeline::set_rasterizer(Rasterizer *rasterizer)
{
	pipeline->RSSetState(rasterizer);
}

void Render_Pipeline::set_scissor(Rect_s32 *rect)
{
	D3D11_RECT rects[1];
	rects[0].left = rect->x;
	rects[0].right = rect->right();
	rects[0].top = rect->y;
	rects[0].bottom = rect->bottom();

	pipeline->RSSetScissorRects(1, rects);
}

void Render_Pipeline::reset_rasterizer()
{
	pipeline->RSSetState(0);
}

void Render_Pipeline::set_blending_text(Blending_Test *blending_test)
{
	float b[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	pipeline->OMSetBlendState(blending_test, b, 0xffffffff);
}

void Render_Pipeline::reset_blending_test()
{
	float b[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	pipeline->OMSetBlendState(0, b, 0);
}

void Render_Pipeline::set_depth_stencil_test(Depth_Stencil_Test *depth_stencil_test, u32 stencil_ref)
{
	pipeline->OMSetDepthStencilState(depth_stencil_test, stencil_ref);
}

void Render_Pipeline::reset_depth_stencil_test()
{
	pipeline->OMSetDepthStencilState(NULL, 0);
}

void Render_Pipeline::draw_indexed(u32 index_count, u32 index_offset, u32 vertex_offset)
{
	pipeline->DrawIndexed(index_count, index_offset, vertex_offset);
}

Vertex_Buffer_Desc::Vertex_Buffer_Desc(u32 _data_count, u32 _data_size, void *_data, Resource_Usage _usage, u32 _cpu_access)
{
	data_count = _data_count;
	data_size = _data_size;
	data = _data;
	usage = _usage;
	cpu_access = _cpu_access;
	bind_flags = BIND_VERTEX_BUFFER;
}

Index_Buffer_Desc::Index_Buffer_Desc(u32 _data_count, void *_data, Resource_Usage _usage, u32 _cpu_access)
{
	data_count = _data_count;
	data_size = sizeof(u32);
	data = _data;
	usage = _usage;
	cpu_access = _cpu_access;
	bind_flags = BIND_INDEX_BUFFER;
}

Rasterizer_Desc::Rasterizer_Desc()
{
	desc.FillMode = D3D11_FILL_SOLID;
	desc.CullMode = D3D11_CULL_FRONT;
	desc.FrontCounterClockwise = false;
	desc.DepthBias = false;
	desc.DepthBiasClamp = 0;
	desc.SlopeScaledDepthBias = 0;
	desc.DepthClipEnable = true;
	desc.ScissorEnable = true;
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

Blending_Test_Desc::Blending_Test_Desc()
{
	bool enable = true;
	Blend_Option src = BLEND_ONE;
	Blend_Option dest = BLEND_ZERO;
	Blend_Operation blend_op = BLEND_OP_ADD;
	Blend_Option src_alpha = BLEND_ONE;
	Blend_Option dest_alpha = BLEND_ZERO;
	Blend_Operation blend_op_alpha = BLEND_OP_ADD;
	u8 write_mask = D3D11_COLOR_WRITE_ENABLE_ALL;
}

Depth_Stencil_Test_Desc::Depth_Stencil_Test_Desc(Stencil_Operation _stencil_failed, Stencil_Operation _depth_failed, Stencil_Operation _pass, Comparison_Func _compare_func, u32 _write_mask, u32 _read_mask, bool _enable_depth_test)
{
	enable_depth_test = _enable_depth_test;
	enalbe_stencil_test = true;
	stencil_read_mask = _read_mask;
	stencil_write_mack = _write_mask;
	stencil_failed = _stencil_failed;
	depth_failed = _depth_failed;
	pass = _pass;
	compare_func = _compare_func;
}

u32 Texture::get_row_pitch()
{
	return width * format_size;
}

u32 *create_color_buffer(u32 width, u32 height, const Color &color)
{
	u32 *data = new u32[width * height];

	u8* pixels = (u8 *)data;
	for (u32 row = 0; row < height; row++)
	{
		u32 row_start = row * (width * sizeof(u32));
		for (u32 col = 0; col < width; col++)
		{
			u32 col_start = col * 4;
			pixels[row_start + col_start + 0] = u8(color.value.x * 255);
			pixels[row_start + col_start + 1] = u8(color.value.y * 255);
			pixels[row_start + col_start + 2] = u8(color.value.z * 255);
			pixels[row_start + col_start + 3] = u8(color.value.w * 255);
		}
	}
	return data;
}

void init_render_api(Gpu_Device *gpu_device, Render_Pipeline *render_pipeline, Win32_Info *win32_info)
{
	UINT create_device_flag = 0;

#if defined(DEBUG) || defined(_DEBUG)  
	create_device_flag |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL feature_level;

	HRESULT hr = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, create_device_flag, 0, 0, D3D11_SDK_VERSION, &gpu_device->device, &feature_level, &render_pipeline->pipeline);

	if (FAILED(hr)) {
		error("D3D11CreateDevice Failed.");
		return;
	}

	if (feature_level < D3D_FEATURE_LEVEL_11_0) {
		error("Direct3D Feature Level 11 unsupported.");
		return;
	}

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = win32_info->window_width;
	sd.BufferDesc.Height = win32_info->window_height;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	HR(gpu_device->device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &gpu_device->quality_levels));
	assert(gpu_device->quality_levels > 0);

	if (true) {
		sd.SampleDesc.Count = 4;
		sd.SampleDesc.Quality = gpu_device->quality_levels - 1;
	} else {
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
	}

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = win32_info->window;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;

	IDXGIDevice* dxgi_device = 0;
	HR(gpu_device->device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgi_device));

	IDXGIAdapter* dxgi_adapter = 0;
	HR(dxgi_device->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgi_adapter));

	IDXGIFactory* dxgi_factory = 0;
	HR(dxgi_adapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgi_factory));

	HR(dxgi_factory->CreateSwapChain(gpu_device->device, &sd, &render_pipeline->swap_chain));

	RELEASE_COM(dxgi_device);
	RELEASE_COM(dxgi_adapter);
	RELEASE_COM(dxgi_factory);

	render_pipeline->resize(gpu_device, win32_info->window_width, win32_info->window_height);
}

u32 *r8_to_rgba32(u8 *data, u32 width, u32 height)
{
	u32 *new_data = new u32[width * height];

	u8* pixels = (u8*)new_data;
	for (u32 row = 0; row < height; row++) {
		u32 row_start = row * (width * sizeof(u32));
		u32 row_2 = row * (width * sizeof(u8));

		for (u32 col = 0; col < width; col++) {
			u32 col_start = col * 4;
			pixels[row_start + col_start + 0] = 255;
			pixels[row_start + col_start + 1] = 255;
			pixels[row_start + col_start + 2] = 255;
			pixels[row_start + col_start + 3] = data[row_2 + col];
		}
	}
	return new_data;
}