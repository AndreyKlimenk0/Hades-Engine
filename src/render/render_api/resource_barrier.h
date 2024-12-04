#ifndef RENDER_API_RESOURCE_BARRIER
#define RENDER_API_RESOURCE_BARRIER

#include <d3d12.h>

#include "resource.h"
#include "../../libs/number_types.h"

enum Resource_State {
	RESOURCE_STATE_UNKNOWN,
	RESOURCE_STATE_PRESENT,
	RESOURCE_STATE_RENDER_TARGET,
	RESOURCE_STATE_DEPTH_WRITE
};

struct Resource_Barrier {
	Resource_Barrier() = default;
	virtual ~Resource_Barrier() = default;

	virtual D3D12_RESOURCE_BARRIER d3d12_resource_barrier() = 0;
};

struct Transition_Resource_Barrier : Resource_Barrier {
	Transition_Resource_Barrier();
	Transition_Resource_Barrier(GPU_Resource resource, Resource_State state_before, Resource_State state_after);
	Transition_Resource_Barrier(u32 subresources, GPU_Resource resource, Resource_State state_before, Resource_State state_after);
	~Transition_Resource_Barrier();

	u32 subresource;
	GPU_Resource resource;
	Resource_State state_before;
	Resource_State state_after;
	
	D3D12_RESOURCE_BARRIER d3d12_resource_barrier();
};

#endif