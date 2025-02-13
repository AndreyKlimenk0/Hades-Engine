#include <d3d12.h>

#include "texture.h"
#include "../render_helpers.h"
#include "../../sys/utils.h"
#include "../../sys/memory.h"

Texture::Texture()
{
}

Texture::~Texture()
{
}

void Texture::create(Gpu_Device &device, GPU_Heap_Type heap_type, Resource_State resource_state, Texture2D_Desc &desc)
{
	set_resource_parameters(1, desc.width * desc.height);

	D3D12_RESOURCE_DESC resource_desc;
	ZeroMemory(&resource_desc, sizeof(D3D12_RESOURCE_DESC));
	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resource_desc.Alignment = 0;
	resource_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resource_desc.Width = static_cast<u64>(desc.width);
	resource_desc.Height = desc.height;
	resource_desc.DepthOrArraySize = 1;
	resource_desc.MipLevels = desc.miplevels;
	resource_desc.Format = desc.format;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.SampleDesc.Quality = 0;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resource_desc.Flags = static_cast<D3D12_RESOURCE_FLAGS>(desc.flags);

	GPU_Resource::create(device, heap_type, resource_state, resource_desc, desc.clear_value);
}

u32 Texture::get_subresource_count()
{
	D3D12_RESOURCE_DESC d3d12_texture_desc = d3d12_resource_desc();
	assert(d3d12_texture_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D);
	return static_cast<u32>(d3d12_texture_desc.MipLevels);
}

u32 Texture::get_size()
{
	D3D12_RESOURCE_DESC d3d12_texture_desc = d3d12_resource_desc();
	assert(d3d12_texture_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D);
	return align_address<u32>(d3d12_texture_desc.Width * dxgi_format_size(d3d12_texture_desc.Format), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * d3d12_texture_desc.Height;
}

Texture2D_Desc Texture::get_texture2d_desc()
{
	D3D12_RESOURCE_DESC d3d12_texture_desc = d3d12_resource_desc();
	Texture2D_Desc texture_desc;
	texture_desc.width = static_cast<u32>(d3d12_texture_desc.Width);
	texture_desc.height = d3d12_texture_desc.Height;
	texture_desc.miplevels = d3d12_texture_desc.MipLevels;
	texture_desc.flags = d3d12_texture_desc.Flags;
	texture_desc.format = d3d12_texture_desc.Format;
	return texture_desc;
}