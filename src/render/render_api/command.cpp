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

void Graphics_Command_List::reset(Command_Allocator &command_allocator)
{
    HR(d3d12_object->Reset(command_allocator.get(), NULL));
}

void Graphics_Command_List::reset(Command_Allocator &command_allocator, Pipeline_State &pipeline_state)
{
    HR(d3d12_object->Reset(command_allocator.get(), pipeline_state.get()));
}

void Graphics_Command_List::clear_render_target_view(D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle, const Color &color)
{
    float temp[4] = { color.value.x, color.value.y, color.value.z, color.value.w };
    d3d12_object->ClearRenderTargetView(cpu_descriptor_handle, temp, 0, NULL);
}

void Graphics_Command_List::resource_barrier(const Resource_Barrier &resource_barrier)
{
    D3D12_RESOURCE_BARRIER d3d12_resource_barrier = const_cast<Resource_Barrier &>(resource_barrier).d3d12_resource_barrier();
    d3d12_object->ResourceBarrier(1, &d3d12_resource_barrier);
}

void Graphics_Command_List::create(Gpu_Device &device, Command_Allocator &command_allocator)
{
    Command_List::create(device, COMMAND_LIST_TYPE_DIRECT, command_allocator);
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

void Command_List::create(Gpu_Device &device, Command_List_Type command_list_type, Command_Allocator &command_allocator)
{
    device->CreateCommandList(0, command_list_type_to_d3d12(command_list_type), command_allocator.get(), NULL, IID_PPV_ARGS(release_and_get_address()));
}

Copy_Command_List::Copy_Command_List()
{
}

Copy_Command_List::~Copy_Command_List()
{
}

void Copy_Command_List::create(Gpu_Device &device, Command_Allocator &command_allocator)
{
    Command_List::create(device, COMMAND_LIST_TYPE_COPY, command_allocator);
    close(); // Should Command_List::create call this function ?
}

ID3D12CommandList *Copy_Command_List::get_d3d12_command_list()
{
    return static_cast<ID3D12CommandList *>(get());
}
