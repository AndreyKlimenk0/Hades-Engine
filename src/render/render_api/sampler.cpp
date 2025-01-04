#include <assert.h>
#include "sampler.h"

static D3D12_FILTER to_d3d12_filter(Sampler_Filter filter)
{
	switch (filter) {
		case SAMPLER_FILTER_POINT:
			return D3D12_FILTER_MIN_MAG_MIP_POINT;
		case SAMPLER_FILTER_LINEAR:
			return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		case SAMPLER_FILTER_ANISOTROPIC:
			return D3D12_FILTER_ANISOTROPIC;
	}
	assert(false);
	return (D3D12_FILTER)0;
}

static D3D12_TEXTURE_ADDRESS_MODE to_d3d12_texture_address_mode(Address_Mode address_mode)
{
	switch (address_mode) {
		case ADDRESS_MODE_WRAP:
			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		case ADDRESS_MODE_MIRROR:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		case ADDRESS_MODE_CLAMP:
			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		case ADDRESS_MODE_BORDER:
			return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	}
	assert(false);
	return (D3D12_TEXTURE_ADDRESS_MODE)0;
}

Sampler::Sampler()
{
}

Sampler::Sampler(Sampler_Filter filter, Address_Mode uvw) : filter(filter), u(uvw), v(uvw), w(uvw)
{
}

Sampler::~Sampler()
{
}

D3D12_SAMPLER_DESC Sampler::to_d3d12_sampler_desc()
{
	D3D12_SAMPLER_DESC d3d12_sampler_desc;
	ZeroMemory(&d3d12_sampler_desc, sizeof(D3D12_SAMPLER_DESC));
	d3d12_sampler_desc.Filter = to_d3d12_filter(filter);
	d3d12_sampler_desc.AddressU = to_d3d12_texture_address_mode(u);
	d3d12_sampler_desc.AddressV = to_d3d12_texture_address_mode(v);
	d3d12_sampler_desc.AddressW = to_d3d12_texture_address_mode(w);
	d3d12_sampler_desc.MipLODBias = 0;
	d3d12_sampler_desc.MaxAnisotropy = filter == SAMPLER_FILTER_ANISOTROPIC ? 16 : 0;
	d3d12_sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3d12_sampler_desc.BorderColor[0] = 0;
	d3d12_sampler_desc.BorderColor[1] = 0;
	d3d12_sampler_desc.BorderColor[2] = 0;
	d3d12_sampler_desc.BorderColor[3] = 0;
	d3d12_sampler_desc.MinLOD = 0;
	d3d12_sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
	return d3d12_sampler_desc;
}