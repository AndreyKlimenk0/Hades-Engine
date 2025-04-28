#include <assert.h>

#include "command.h"
#include "../render_helpers.h"
#include "../../sys/utils.h"

static D3D_PRIMITIVE_TOPOLOGY to_d3d_primitive_topology(Primitive_Type primitive_type)
{
    switch (primitive_type) {
        case PRIMITIVE_TYPE_UNKNOWN:
            return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        case PRIMITIVE_TYPE_POINT:
            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case PRIMITIVE_TYPE_LINE:
            return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case PRIMITIVE_TYPE_TRIANGLE:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PRIMITIVE_TYPE_PATCH:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    }
    assert(false);
    return (D3D_PRIMITIVE_TOPOLOGY)0;
}

static D3D12_COMMAND_LIST_TYPE command_list_type_to_d3d12(Command_List_Type command_list_type)
{
    switch (command_list_type) {
        case COMMAND_LIST_TYPE_DIRECT:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case COMMAND_LIST_TYPE_BUNDLE:
            return D3D12_COMMAND_LIST_TYPE_BUNDLE;
        case COMMAND_LIST_TYPE_COMPUTE:
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        case COMMAND_LIST_TYPE_COPY:
            return D3D12_COMMAND_LIST_TYPE_COPY;
        default:
            assert(false);
    }
    return (D3D12_COMMAND_LIST_TYPE)0;
}

Command_Allocator::Command_Allocator()
{
}

Command_Allocator::~Command_Allocator()
{
}

void Command_Allocator::reset()
{
    d3d12_object->Reset();
}

void Command_Allocator::create(Gpu_Device &device, Command_List_Type command_list_type)
{
    HR(device->CreateCommandAllocator(command_list_type_to_d3d12(command_list_type), IID_PPV_ARGS(release_and_get_address())));
}

Command_List::Command_List()
{
}

Command_List::~Command_List()
{
}

void Command_List::close()
{
    HR(d3d12_object->Close());
}

void Command_List::reset(u32 command_allocator_index)
{
    Command_Allocator &command_allocator = command_allocators[command_allocator_index];
    command_allocator.reset();
    HR(d3d12_object->Reset(command_allocator.get(), NULL));
}

void Command_List::create(Gpu_Device &device, u32 number_command_allocators, Command_List_Type command_list_type)
{
    assert(number_command_allocators > 0);

    command_allocators.reserve(number_command_allocators);
    for (u32 i = 0; i < number_command_allocators; i++) {
        command_allocators[i].create(device, command_list_type);
    }
    device->CreateCommandList(0, command_list_type_to_d3d12(command_list_type), command_allocators.first().get(), NULL, IID_PPV_ARGS(release_and_get_address()));
    close();
}

ID3D12CommandList *Command_List::get_d3d12_command_list()
{
    return get();
}

Copy_Command_List::Copy_Command_List()
{
}

Copy_Command_List::~Copy_Command_List()
{
}

void Copy_Command_List::resource_barrier(const Resource_Barrier &resource_barrier)
{
    D3D12_RESOURCE_BARRIER d3d12_resource_barrier = const_cast<Resource_Barrier &>(resource_barrier).d3d12_resource_barrier();
    d3d12_object->ResourceBarrier(1, &d3d12_resource_barrier);
}

void Copy_Command_List::copy_resources(GPU_Resource &dest, GPU_Resource &source)
{
    d3d12_object->CopyResource(dest.get(), source.get());
}

void Copy_Command_List::copy_buffer_to_texture(GPU_Resource &dest, GPU_Buffer &source, Subresource_Footprint &subresource_footprint)
{
    D3D12_TEXTURE_COPY_LOCATION dest_texture_copy_location;
    ZeroMemory(&dest_texture_copy_location, sizeof(D3D12_TEXTURE_COPY_LOCATION));
    dest_texture_copy_location.pResource = dest.get();
    dest_texture_copy_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dest_texture_copy_location.SubresourceIndex = subresource_footprint.subresource_index;

    D3D12_TEXTURE_COPY_LOCATION source_texture_copy_location;
    ZeroMemory(&source_texture_copy_location, sizeof(D3D12_TEXTURE_COPY_LOCATION));
    source_texture_copy_location.pResource = source.get();
    source_texture_copy_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    source_texture_copy_location.PlacedFootprint.Footprint = subresource_footprint.d3d12_subresource_footprint();

    d3d12_object->CopyTextureRegion(&dest_texture_copy_location, 0, 0, 0, &source_texture_copy_location, NULL);
}

void Copy_Command_List::create(Gpu_Device &device, u32 number_command_allocators)
{
    Command_List::create(device, number_command_allocators, COMMAND_LIST_TYPE_COPY);
}

Compute_Command_List::Compute_Command_List()
{
}

Compute_Command_List::~Compute_Command_List()
{
}

void Compute_Command_List::set_pipeline_state(Pipeline_State &pipeline_state)
{
    d3d12_object->SetPipelineState(pipeline_state.get());
}

void Compute_Command_List::set_compute_root_signature(Root_Signature &root_signature)
{
    d3d12_object->SetComputeRootSignature(root_signature.get());
}

void Compute_Command_List::set_descriptor_heaps(CBSRUA_Descriptor_Heap &cbsrua_descriptor_heap, Sampler_Descriptor_Heap &sampler_descriptor_heap)
{
    ID3D12DescriptorHeap *descriptor_heaps[] = { cbsrua_descriptor_heap.get(), sampler_descriptor_heap.get() };
    d3d12_object->SetDescriptorHeaps(2, descriptor_heaps);
}

void Compute_Command_List::set_compute_root_descriptor_table(u32 parameter_index, const GPU_Descriptor &base_descriptor)
{
    assert(const_cast<GPU_Descriptor &>(base_descriptor).valid());
    d3d12_object->SetComputeRootDescriptorTable(parameter_index, base_descriptor.gpu_handle);
}

void Compute_Command_List::dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z)
{
    d3d12_object->Dispatch(group_count_x, group_count_y, group_count_z);
}

void Compute_Command_List::create(Gpu_Device &device, u32 number_command_allocators)
{
    Command_List::create(device, number_command_allocators, COMMAND_LIST_TYPE_COMPUTE);
}

Graphics_Command_List::Graphics_Command_List()
{
}

Graphics_Command_List::~Graphics_Command_List()
{
}

void Graphics_Command_List::set_primitive_type(Primitive_Type primitive_type)
{
    d3d12_object->IASetPrimitiveTopology(to_d3d_primitive_topology(primitive_type));
}

void Graphics_Command_List::set_viewport(const Viewport &viewport)
{
    D3D12_VIEWPORT d3d12_viewport;
    ZeroMemory(&d3d12_viewport, sizeof(D3D12_VIEWPORT));
    d3d12_viewport.TopLeftX = viewport.x;
    d3d12_viewport.TopLeftY = viewport.y;
    d3d12_viewport.Width = viewport.width;
    d3d12_viewport.Height = viewport.height;
    d3d12_viewport.MinDepth = viewport.min_depth;
    d3d12_viewport.MaxDepth = viewport.max_depth;

    d3d12_object->RSSetViewports(1, &d3d12_viewport);
}

void Graphics_Command_List::set_clip_rect(const Rect_u32 &clip_rect)
{
    assert(clip_rect.width > 0);
    assert(clip_rect.height > 0);

    D3D12_RECT d3d12_clip_rect;
    ZeroMemory(&d3d12_clip_rect, sizeof(D3D12_RECT));
    d3d12_clip_rect.left = clip_rect.x;
    d3d12_clip_rect.top = clip_rect.y;
    d3d12_clip_rect.right = clip_rect.x + clip_rect.width;
    d3d12_clip_rect.bottom = clip_rect.y + clip_rect.height;

    d3d12_object->RSSetScissorRects(1, &d3d12_clip_rect);
}

void Graphics_Command_List::clear_render_target_view(const RT_Descriptor &descriptor, const Color &color)
{
    float temp[4] = { color.value.x, color.value.y, color.value.z, color.value.w };
    d3d12_object->ClearRenderTargetView(descriptor.cpu_handle, temp, 0, NULL);
}

void Graphics_Command_List::clear_depth_stencil_view(const DS_Descriptor &descriptor, float depth, u8 stencil)
{
    d3d12_object->ClearDepthStencilView(descriptor.cpu_handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, NULL);
}

void Graphics_Command_List::set_vertex_buffer(GPU_Resource &resource)
{
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
    vertex_buffer_view.BufferLocation = resource.get_gpu_address();
    vertex_buffer_view.SizeInBytes = resource.get_size();
    vertex_buffer_view.StrideInBytes = resource.stride;
    
    d3d12_object->IASetVertexBuffers(0, 1, &vertex_buffer_view);
}

void Graphics_Command_List::set_index_buffer(GPU_Resource &resource)
{
    D3D12_INDEX_BUFFER_VIEW index_buffer_view;
    index_buffer_view.BufferLocation = resource.get_gpu_address();
    index_buffer_view.SizeInBytes = resource.get_size();
    index_buffer_view.Format = DXGI_FORMAT_R32_UINT;
    
    d3d12_object->IASetIndexBuffer(&index_buffer_view);
}

void Graphics_Command_List::set_graphics_root_signature(Root_Signature &root_signature)
{
    d3d12_object->SetGraphicsRootSignature(root_signature.get());
}

void Graphics_Command_List::set_render_target(const RT_Descriptor &render_target_descriptor, const DS_Descriptor &depth_stencil_descriptor)
{
    d3d12_object->OMSetRenderTargets(1, &render_target_descriptor.cpu_handle, FALSE, &depth_stencil_descriptor.cpu_handle);
}

void Graphics_Command_List::set_graphics_root_descriptor_table(u32 parameter_index, const GPU_Descriptor &base_descriptor)
{
    assert(const_cast<GPU_Descriptor &>(base_descriptor).valid());
    d3d12_object->SetGraphicsRootDescriptorTable(parameter_index, base_descriptor.gpu_handle);
}

void Graphics_Command_List::draw(u32 vertex_count)
{
    d3d12_object->DrawInstanced(vertex_count, 1, 0, 0);
}

void Graphics_Command_List::draw_indexed(u32 index_count)
{
    d3d12_object->DrawIndexedInstanced(index_count, 1, 0, 0, 0);
}

void Graphics_Command_List::create(Gpu_Device &device, u32 number_command_allocators)
{
    Command_List::create(device, number_command_allocators, COMMAND_LIST_TYPE_DIRECT);
}

Command_Queue::Command_Queue()
{
}

Command_Queue::~Command_Queue()
{
}

void Command_Queue::wait(Fence &fence)
{
    d3d12_object->Wait(fence.get(), fence.expected_value);
}

void Command_Queue::flush_gpu()
{
    fence.increment_expected_value();
    signal(fence);
    fence.wait_for_gpu();
}

void Command_Queue::create(Gpu_Device &device, Command_List_Type command_list_type)
{
    fence.create(device);

    D3D12_COMMAND_QUEUE_DESC command_queue_desc;
    ZeroMemory(&command_queue_desc, sizeof(D3D12_COMMAND_QUEUE_DESC));
    command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    command_queue_desc.Type = command_list_type_to_d3d12(command_list_type);

    HR(device->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(release_and_get_address())));
}

void Command_Queue::execute_command_list(Command_List &command_list)
{
    ID3D12CommandList *command_lists[] = { command_list.get_d3d12_command_list() };
    d3d12_object->ExecuteCommandLists(1, command_lists);
}

void Command_Queue::signal(Fence &fence)
{
    d3d12_object->Signal(fence.get(), fence.expected_value);
}