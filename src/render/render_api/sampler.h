#ifndef RENDER_API_SAMPLER_H
#define RENDER_API_SAMPLER_H

#include <d3d12.h>

enum Sampler_Filter {
	SAMPLER_FILTER_POINT,
	SAMPLER_FILTER_LINEAR,
	SAMPLER_FILTER_ANISOTROPIC
};

enum Address_Mode {
	ADDRESS_MODE_WRAP,
	ADDRESS_MODE_MIRROR,
	ADDRESS_MODE_CLAMP,
	ADDRESS_MODE_BORDER,
};

struct Sampler {
	Sampler();
	Sampler(Sampler_Filter filter, Address_Mode uvw);
	~Sampler();

	Sampler_Filter filter;
	Address_Mode u;
	Address_Mode v;
	Address_Mode w;

	D3D12_SAMPLER_DESC to_d3d12_sampler_desc();
};

#endif
