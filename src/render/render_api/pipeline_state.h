#ifndef RENDER_API_PIPELINE_STATE
#define RENDER_API_PIPELINE_STATE

#include <d3d12.h>
#include "d3d12_object.h"

struct D12_Rasterizer_Desc {
	D12_Rasterizer_Desc();
	~D12_Rasterizer_Desc();

	D3D12_RASTERIZER_DESC d3d12_rasterizer_desc;
};

struct D12_Blend_Desc {
	D12_Blend_Desc();
	~D12_Blend_Desc();

	D3D12_BLEND_DESC d3d12_blend_desc;
};


struct Pipeline_State : D3D12_Object<ID3D12PipelineState> {

};

#endif