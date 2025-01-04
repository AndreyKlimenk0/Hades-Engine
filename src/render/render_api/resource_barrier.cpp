#include <assert.h>
#include "resource_barrier.h"

Transition_Resource_Barrier::Transition_Resource_Barrier()
    : subresource(0), resource(), state_before(RESOURCE_STATE_UNKNOWN), state_after(RESOURCE_STATE_UNKNOWN)
{
}

Transition_Resource_Barrier::Transition_Resource_Barrier(GPU_Resource resource, Resource_State state_before, Resource_State state_after)
    : Transition_Resource_Barrier(0, resource, state_before, state_after)
{
}

Transition_Resource_Barrier::Transition_Resource_Barrier(u32 subresource, GPU_Resource resource, Resource_State state_before, Resource_State state_after)
    : subresource(subresource), resource(resource), state_before(state_before), state_after(state_after)
{
}

Transition_Resource_Barrier::~Transition_Resource_Barrier()
{
}

D3D12_RESOURCE_BARRIER Transition_Resource_Barrier::d3d12_resource_barrier()
{
    D3D12_RESOURCE_BARRIER d3d12_resource_barrier;
    ZeroMemory(&d3d12_resource_barrier, sizeof(D3D12_RESOURCE_BARRIER));
    d3d12_resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    d3d12_resource_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    d3d12_resource_barrier.Transition.pResource = resource.get();
    d3d12_resource_barrier.Transition.Subresource = subresource;
    d3d12_resource_barrier.Transition.StateBefore = to_d3d12_resource_state(state_before);
    d3d12_resource_barrier.Transition.StateAfter = to_d3d12_resource_state(state_after);
    return d3d12_resource_barrier;
}

