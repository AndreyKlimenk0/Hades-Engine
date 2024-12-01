#include <assert.h>
#include <stdlib.h>

#include <dxgi1_5.h>

#include "../sys/sys.h"
#include "../sys/utils.h"
#include "../libs/number_types.h"

#include "render_api.h"

inline char *to_string(const wchar_t *unicode_string)
{
    char *new_string = NULL;
    u32 unicode_string_len = (u32)wcslen(unicode_string);
    if (unicode_string_len > 0) {
        u32 new_string_len = unicode_string_len + 1;
        new_string = new char[new_string_len];

        size_t new_string_size_in_bytes = 0;
        errno_t result = wcstombs_s(&new_string_size_in_bytes, new_string, new_string_len, unicode_string, unicode_string_len);
        assert(result == 0);
        assert((new_string_size_in_bytes / sizeof(u8)) == new_string_len);
    }
    return new_string;
}

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

bool Gpu_Device::init()
{
    bool result = false;
    u32 factory_flags = 0;
#ifdef _DEBUG
    factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
    
    ComPtr<ID3D12Debug> debug_interface;
    HR(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)));
    debug_interface->EnableDebugLayer();
#endif
    ComPtr<IDXGIFactory4> dxgi_factory;
    HR(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&dxgi_factory)));

    ComPtr<IDXGIAdapter1> dxgi_adapter1;
    for (u32 i = 0; dxgi_factory->EnumAdapters1(i, dxgi_adapter1.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; i++) {
        DXGI_ADAPTER_DESC1 dxgi_adapter_desc;
        dxgi_adapter1->GetDesc1(&dxgi_adapter_desc);
        if (dxgi_adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }
        if (SUCCEEDED(D3D12CreateDevice(dxgi_adapter1.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) {
            char *device_desc = to_string(dxgi_adapter_desc.Description);
            print("init_GPU_device: {} created a new D3D12 device.", device_desc);
            DELETE_PTR(device_desc);
            result = true;
            break;
        }
    }
#ifdef _DEBUG
    ComPtr<ID3D12InfoQueue> info_queue;
    if (result && SUCCEEDED(device.As(&info_queue))) {
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        D3D12_MESSAGE_SEVERITY apathetic_levels[] = {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        D3D12_MESSAGE_ID apathetic_messages[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
        };

        D3D12_INFO_QUEUE_FILTER message_filter;
        ZeroMemory(&message_filter, sizeof(D3D12_INFO_QUEUE_FILTER));
        message_filter.DenyList.NumSeverities = _countof(apathetic_levels);
        message_filter.DenyList.pSeverityList = apathetic_levels;
        message_filter.DenyList.NumIDs = _countof(apathetic_messages);
        message_filter.DenyList.pIDList = apathetic_messages;
        
        HR(info_queue->PushStorageFilter(&message_filter));
    }
#endif
    if (!result) {
        print("init_GPU_device: Failed to create D3D12 device. There are no adapters support D3D12.");
    }
    return result;
}

void Gpu_Device::release()
{
    device.Reset();
}

D3D12_CLEAR_VALUE clear_value_to_d3d12(DXGI_FORMAT format, const Clear_Value &clear_value)
{
    D3D12_CLEAR_VALUE d3d12_clear_value;
    d3d12_clear_value.Format = format;
    switch (format) {
        case DXGI_FORMAT_D32_FLOAT: {
            d3d12_clear_value.DepthStencil.Depth = clear_value.depth_stencil_value.depth;
            d3d12_clear_value.DepthStencil.Stencil = clear_value.depth_stencil_value.stencil;
            break;
        }

    }
    return d3d12_clear_value;
}

void Gpu_Device::create_texture2d(const Texture2D_Desc &texture_desc, Resource_State resource_state, GPU_Resource &resource)
{
    D3D12_HEAP_PROPERTIES heap_properties = {};
    ZeroMemory(&heap_properties, sizeof(D3D12_HEAP_PROPERTIES));
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resource_desc = {};
    ZeroMemory(&resource_desc, sizeof(D3D12_RESOURCE_DESC));
    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment = 0;
    resource_desc.Width = texture_desc.width;
    resource_desc.Height = texture_desc.height;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = texture_desc.miplevels;
    resource_desc.Format = texture_desc.format;
    resource_desc.SampleDesc.Count = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags = (D3D12_RESOURCE_FLAGS)texture_desc.resource_options;

    D3D12_CLEAR_VALUE clear_value = clear_value_to_d3d12(texture_desc.format, texture_desc.clear_value);

    device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, to_d3d12_resource_state(resource_state), &clear_value, IID_PPV_ARGS(resource.release_and_get_address()));
}

void Gpu_Device::create_default_resource(u32 resource_size, GPU_Resource &resource)
{
    create_commited_resource(resource_size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, resource);
}

void Gpu_Device::create_upload_resource(u32 resource_size, GPU_Resource &resource)
{
    create_commited_resource(resource_size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, resource);
}

void Gpu_Device::create_commited_resource(u32 resource_size, D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_STATES resource_state, GPU_Resource &resource)
{
    D3D12_HEAP_PROPERTIES heap_properties = {};
    ZeroMemory(&heap_properties, sizeof(D3D12_HEAP_PROPERTIES));
    heap_properties.Type = heap_type;

    D3D12_RESOURCE_DESC resource_desc = {};
    ZeroMemory(&resource_desc, sizeof(D3D12_RESOURCE_DESC));
    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment = 0;
    resource_desc.Width = resource_size;
    resource_desc.Height = 1;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HR(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, resource_state, NULL, IID_PPV_ARGS(resource.release_and_get_address())));
}

void Gpu_Device::create_fence(Fence &fence)
{
    HR(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.ReleaseAndGetAddressOf())));
}

D3D12_COMMAND_LIST_TYPE command_list_type_to_d3d12(Command_List_Type command_list_type) 
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


void Gpu_Device::create_command_allocator(Command_List_Type command_list_type, Command_Allocator &command_allocator)
{
    HR(device->CreateCommandAllocator(command_list_type_to_d3d12(command_list_type), IID_PPV_ARGS(command_allocator.command_allocator.ReleaseAndGetAddressOf())));
}

void Gpu_Device::create_command_queue(Command_List_Type command_list_type, Command_Queue &command_queue)
{
    // Should I reset command queue before creating ?

    D3D12_COMMAND_QUEUE_DESC queueDesc;
    ZeroMemory(&queueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = command_list_type_to_d3d12(command_list_type);

    HR(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(command_queue.release_and_get_address())));
}

//void Gpu_Device::create_command_list(const GPU_Pipeline_State &pipeline_state, Graphics_Command_List &command_list)
//{
//    HR(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_list.command_allocator.Get(), pipeline_state.Get(), IID_PPV_ARGS(command_list.command_list.ReleaseAndGetAddressOf())));
//    command_list.close();
//}

void Gpu_Device::create_command_list(Command_List_Type command_list_type, Command_Allocator &command_allocator, Graphics_Command_List &command_list)
{
    HR(device->CreateCommandList(0, command_list_type_to_d3d12(command_list_type), command_allocator.command_allocator.Get(), nullptr, IID_PPV_ARGS(command_list.release_and_get_address())));
    command_list.close();
}

void Gpu_Device::create_rtv_descriptor_heap(u32 descriptor_count, Descriptor_Heap &descriptor_heap)
{
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
    ZeroMemory(&heap_desc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
    heap_desc.NumDescriptors = descriptor_count;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HR(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(descriptor_heap.heap.ReleaseAndGetAddressOf())));

    descriptor_heap.descriptor_count = descriptor_count;
    descriptor_heap.increment_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    descriptor_heap.cpu_handle = descriptor_heap.heap->GetCPUDescriptorHandleForHeapStart();
}

void Gpu_Device::create_cbv_srv_uav_descriptor_heap(u32 descriptor_count, Descriptor_Heap &descriptor_heap)
{
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
    ZeroMemory(&heap_desc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
    heap_desc.NumDescriptors = descriptor_count;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HR(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(descriptor_heap.heap.ReleaseAndGetAddressOf())));

    descriptor_heap.descriptor_count = descriptor_count;
    descriptor_heap.increment_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    descriptor_heap.cpu_handle = descriptor_heap.heap->GetCPUDescriptorHandleForHeapStart();
    descriptor_heap.gpu_handle = descriptor_heap.heap->GetGPUDescriptorHandleForHeapStart();
}

void Swap_Chain::init(bool allow_tearing, u32 buffer_count, u32 width, u32 height, HWND handle, Command_Queue &command_queue)
{
    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc;
    ZeroMemory(&swap_chain_desc, sizeof(DXGI_SWAP_CHAIN_DESC1));
    swap_chain_desc.Width = width;
    swap_chain_desc.Height = height;
    swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SampleDesc.Quality = 0;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = buffer_count;
    swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_chain_desc.Flags = (allow_tearing && check_tearing_support()) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    u32 factory_flags = 0;
#ifdef _DEBUG
    factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
    ComPtr<IDXGIFactory4> dxgi_factory;
    HR(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&dxgi_factory)));

    ComPtr<IDXGISwapChain1> dxgi_swap_chain1;
    HR(dxgi_factory->CreateSwapChainForHwnd(command_queue.get(), handle, &swap_chain_desc, NULL, NULL, &dxgi_swap_chain1));

    HR(dxgi_factory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER));

    HR(dxgi_swap_chain1.As(&dxgi_swap_chain));
}

void Swap_Chain::release()
{
    dxgi_swap_chain.Reset();
}

u32 Swap_Chain::get_current_back_buffer_index()
{
    return dxgi_swap_chain->GetCurrentBackBufferIndex();
}

void Swap_Chain::resize(u32 width, u32 height)
{
}

void Swap_Chain::present(u32 sync_interval, u32 flags)
{
    HR(dxgi_swap_chain->Present(sync_interval, flags));
}

void Swap_Chain::get_buffer(u32 buffer_index, GPU_Resource &resource)
{
    HR(dxgi_swap_chain->GetBuffer(buffer_index, IID_PPV_ARGS(resource.release_and_get_address())));
}

Descriptor_Heap::Descriptor_Heap()
{
    ZeroMemory(&cpu_handle, sizeof(D3D12_CPU_DESCRIPTOR_HANDLE));
    ZeroMemory(&gpu_handle, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
}

Descriptor_Heap::~Descriptor_Heap()
{
    release();
}

void Descriptor_Heap::release()
{
    descriptor_count = 0;
    increment_size = 0;
    ZeroMemory(&cpu_handle, sizeof(D3D12_CPU_DESCRIPTOR_HANDLE));
    ZeroMemory(&gpu_handle, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
    heap.Reset();
}

D3D12_CPU_DESCRIPTOR_HANDLE Descriptor_Heap::get_cpu_descriptor_heap_handle(u32 descriptor_index)
{
    assert(0 < increment_size);
    assert(descriptor_index <= descriptor_count);

    return { cpu_handle.ptr + (increment_size * descriptor_index) };
}

D3D12_GPU_DESCRIPTOR_HANDLE Descriptor_Heap::get_gpu_descriptor_heap_handle(u32 descriptor_index)
{
    assert(0 < increment_size);
    assert(descriptor_index <= descriptor_count);

    return { gpu_handle.ptr + (increment_size * descriptor_index) };
}

D3D12_GPU_DESCRIPTOR_HANDLE Descriptor_Heap::get_base_gpu_descriptor_handle()
{
    return get_gpu_descriptor_heap_handle(0);
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

void Graphics_Command_List::close()
{
    HR(d3d12_object->Close());
}

void Graphics_Command_List::reset(Command_Allocator &command_allocator)
{
    HR(d3d12_object->Reset(command_allocator.command_allocator.Get(), NULL));
}

void Graphics_Command_List::reset(Command_Allocator &command_allocator, GPU_Pipeline_State &pipeline_state)
{
    HR(d3d12_object->Reset(command_allocator.command_allocator.Get(), pipeline_state.Get()));
}

void Graphics_Command_List::clear_render_target_view(D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle, const Color &color)
{
    float temp[4] = { color.value.x, color.value.y, color.value.z, color.value.w };
    d3d12_object->ClearRenderTargetView(cpu_descriptor_handle, temp, 0, NULL);
}

void Graphics_Command_List::resource_barrier(const Transition_Resource_Barrier &transition_resource_barrier)
{
    D3D12_RESOURCE_BARRIER d3d12_resource_barrier;
    ZeroMemory(&d3d12_resource_barrier, sizeof(D3D12_RESOURCE_BARRIER));
    d3d12_resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    d3d12_resource_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    d3d12_resource_barrier.Transition.pResource = transition_resource_barrier.resource.d3d12_object.Get();
    d3d12_resource_barrier.Transition.Subresource = transition_resource_barrier.subresource;
    d3d12_resource_barrier.Transition.StateBefore = to_d3d12_resource_state(transition_resource_barrier.state_before);
    d3d12_resource_barrier.Transition.StateAfter = to_d3d12_resource_state(transition_resource_barrier.state_after);
    
    d3d12_object->ResourceBarrier(1, &d3d12_resource_barrier);
}

bool check_tearing_support()
{
    BOOL allow_tearing = FALSE;
    ComPtr<IDXGIFactory4> factory4;
    HRESULT result = CreateDXGIFactory1(IID_PPV_ARGS(&factory4));
    if (SUCCEEDED(result)) {
        ComPtr<IDXGIFactory5> factory5;
        result = factory4.As(&factory5);
        if (SUCCEEDED(result)) {
            result = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(allow_tearing));
        }
    }
    return SUCCEEDED(result) && allow_tearing;
}

void Command_Queue::execute_command_list(Command_List &command_list)
{
    ID3D12CommandList *command_lists[] = { command_list.get_d3d12_command_list() };
    d3d12_object->ExecuteCommandLists(1, command_lists);
}

u64 Command_Queue::signal(u64 &fence_value, const Fence &fence)
{
    u64 fence_value_for_signal = ++fence_value;
    d3d12_object->Signal(fence.Get(), fence_value_for_signal);
    return fence_value_for_signal;
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

void Command_Allocator::reset()
{
    command_allocator->Reset();
}

D12_Rasterizer_Desc::D12_Rasterizer_Desc()
{
    ZeroMemory(&d3d12_rasterizer_desc, sizeof(D3D12_RASTERIZER_DESC));
    d3d12_rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
    d3d12_rasterizer_desc.CullMode = D3D12_CULL_MODE_BACK;
    d3d12_rasterizer_desc.FrontCounterClockwise = FALSE;
    d3d12_rasterizer_desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    d3d12_rasterizer_desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    d3d12_rasterizer_desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    d3d12_rasterizer_desc.DepthClipEnable = TRUE;
    d3d12_rasterizer_desc.MultisampleEnable = FALSE;
    d3d12_rasterizer_desc.AntialiasedLineEnable = FALSE;
    d3d12_rasterizer_desc.ForcedSampleCount = 0;
    d3d12_rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}

D12_Rasterizer_Desc::~D12_Rasterizer_Desc()
{
}

D12_Blend_Desc::D12_Blend_Desc()
{
    ZeroMemory(&d3d12_blend_desc, sizeof(D3D12_BLEND_DESC));
    d3d12_blend_desc.AlphaToCoverageEnable = false;
    d3d12_blend_desc.IndependentBlendEnable = false;
    d3d12_blend_desc.RenderTarget[0].BlendEnable = false;
    d3d12_blend_desc.RenderTarget[0].LogicOpEnable = false;
    d3d12_blend_desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    d3d12_blend_desc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
    d3d12_blend_desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    d3d12_blend_desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    d3d12_blend_desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    d3d12_blend_desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    d3d12_blend_desc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    d3d12_blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

}

D12_Blend_Desc::~D12_Blend_Desc()
{
}

GPU_Resource::GPU_Resource()
{
}

GPU_Resource::~GPU_Resource()
{
}

u8 *GPU_Resource::map()
{
    u8 *memory = NULL;
    d3d12_object->Map(0, NULL, (void **)&memory);
    return memory;
}

u64 GPU_Resource::get_gpu_virtual_address()
{
    return d3d12_object->GetGPUVirtualAddress();
}

void GPU_Resource::unmap()
{
    d3d12_object->Unmap(0, NULL);
}

Texture2D_Desc::Texture2D_Desc()
{
    ZeroMemory(&clear_value, sizeof(Clear_Value));
}

Texture2D_Desc::~Texture2D_Desc()
{
}
