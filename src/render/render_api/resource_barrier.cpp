#include <assert.h>
#include "resource_barrier.h"

static D3D12_RESOURCE_STATES to_d3d12_resource_state(const Resource_State &resource_state)
{
    switch (resource_state) {
        case RESOURCE_STATE_RENDER_TARGET:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case RESOURCE_STATE_PRESENT:
            return D3D12_RESOURCE_STATE_PRESENT;
        case RESOURCE_STATE_DEPTH_WRITE:
            return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    assert(false);
    return D3D12_RESOURCE_STATE_COMMON;
}

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

