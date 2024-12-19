#include <d3d12.h>

#include "texture.h"
#include "../../sys/utils.h"

Texture::Texture()
{
}

Texture::~Texture()
{
}

void Texture::create(Gpu_Device &device, Texture2D_Desc &desc)
{
	D3D12_HEAP_PROPERTIES heap_properties = {};
	ZeroMemory(&heap_properties, sizeof(D3D12_HEAP_PROPERTIES));
	heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC resource_desc = {};
	ZeroMemory(&resource_desc, sizeof(D3D12_RESOURCE_DESC));
	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resource_desc.Alignment = 0;
	resource_desc.Width = desc.width;
	resource_desc.Height = desc.height;
	resource_desc.DepthOrArraySize = 1;
	resource_desc.MipLevels = desc.miplevels;
	resource_desc.Format = desc.format;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.SampleDesc.Quality = 0;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resource_desc.Flags = static_cast<D3D12_RESOURCE_FLAGS>(desc.flags);

	D3D12_CLEAR_VALUE clear_value;
	ZeroMemory(&clear_value, sizeof(D3D12_CLEAR_VALUE));
	clear_value.Format = desc.format;
	if (desc.clear_value.type == CLEAR_VALUE_COLOR) {
		desc.clear_value.color.store(clear_value.Color);
	} else if (desc.clear_value.type == CLEAR_VALUE_DEPTH_STENCIL) {
		clear_value.DepthStencil.Depth = desc.clear_value.depth;
		clear_value.DepthStencil.Stencil = desc.clear_value.stencil;
	}

	D3D12_RESOURCE_STATES resource_states = D3D12_RESOURCE_STATE_COMMON;
	if (desc.flags & DEPTH_STENCIL_RESOURCE) {
		resource_states = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	}

	HR(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, resource_states, &clear_value, IID_PPV_ARGS(release_and_get_address())));
}

Clear_Value::Clear_Value() : type(CLEAR_VALUE_UNKNOWN)
{
}

Clear_Value::Clear_Value(Color &_color)
{
	type = CLEAR_VALUE_COLOR;
	color = _color;
}

Clear_Value::Clear_Value(float _depth, u8 _stencil)
{
	type = CLEAR_VALUE_DEPTH_STENCIL;
	depth = _depth;
	stencil = _stencil;
}

Clear_Value::~Clear_Value()
{
}
