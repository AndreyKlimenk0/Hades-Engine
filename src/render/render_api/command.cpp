#include "command.h"
#include "../../sys/utils.h"

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
    ID3D12CommandAllocator *temp = command_allocators[command_allocator_index].get();
    HR(d3d12_object->Reset(temp, NULL));
}

void Command_List::reset(u32 command_allocator_index, Pipeline_State &pipeline_state)
{
    ID3D12CommandAllocator *temp = command_allocators[command_allocator_index].get();
    HR(d3d12_object->Reset(temp, pipeline_state.get()));
}

void Command_List::create(Gpu_Device &device, u32 number_command_allocators, Command_List_Type command_list_type)
{
    assert(number_command_allocators > 0);

    command_allocators.reserve(number_command_allocators);
    for (u32 i = 0; i < number_command_allocators; i++) {
        command_allocators[i].create(device, command_list_type);
    }
    device->CreateCommandList(0, command_list_type_to_d3d12(command_list_type), command_allocators.first().get(), NULL, IID_PPV_ARGS(release_and_get_address()));
}


Graphics_Command_List::Graphics_Command_List()
{
}

Graphics_Command_List::~Graphics_Command_List()
{
}

ID3D12CommandList *Graphics_Command_List::get_d3d12_command_list()
{
    return static_cast<ID3D12CommandList *>(get());
}

void Graphics_Command_List::clear_render_target_view(RT_Descriptor &descriptor, const Color &color)
{
    float temp[4] = { color.value.x, color.value.y, color.value.z, color.value.w };
    d3d12_object->ClearRenderTargetView(descriptor.cpu_handle, temp, 0, NULL);
}

void Graphics_Command_List::clear_depth_stencil_view(DS_Descriptor &descriptor, float depth, u8 stencil)
{
    d3d12_object->ClearDepthStencilView(descriptor.cpu_handle, D3D12_CLEAR_FLAG_DEPTH, depth, stencil, 0, NULL);
}

void Graphics_Command_List::resource_barrier(const Resource_Barrier &resource_barrier)
{
    D3D12_RESOURCE_BARRIER d3d12_resource_barrier = const_cast<Resource_Barrier &>(resource_barrier).d3d12_resource_barrier();
    d3d12_object->ResourceBarrier(1, &d3d12_resource_barrier);
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

void Graphics_Command_List::create(Gpu_Device &device, u32 number_command_allocators)
{
    Command_List::create(device, number_command_allocators, COMMAND_LIST_TYPE_DIRECT);
    close(); // Should Command_List::create call this function ?
}

Command_Queue::Command_Queue()
{
}

Command_Queue::~Command_Queue()
{
}

void Command_Queue::create(Gpu_Device &device, Command_List_Type command_list_type)
{
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

u64 Command_Queue::signal(u64 &fence_value, Fence &fence)
{
    u64 fence_value_for_signal = ++fence_value;
    d3d12_object->Signal(fence.get(), fence_value_for_signal);
    return fence_value_for_signal;
}

Copy_Command_List::Copy_Command_List()
{
}

Copy_Command_List::~Copy_Command_List()
{
}

void Copy_Command_List::reset(Command_Allocator &command_allocator)
{
    HR(d3d12_object->Reset(command_allocator.get(), NULL));
}

void Copy_Command_List::create(Gpu_Device &device, u32 number_command_allocators)
{
    Command_List::create(device, number_command_allocators, COMMAND_LIST_TYPE_DIRECT);
    close(); // Should Command_List::create call this function ?
}

ID3D12CommandList *Copy_Command_List::get_d3d12_command_list()
{
    return static_cast<ID3D12CommandList *>(get());
}
