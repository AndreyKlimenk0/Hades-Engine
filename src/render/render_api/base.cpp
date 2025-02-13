#include <dxgi1_5.h>

#include "base.h"
#include "../../sys/sys.h"
#include "../../sys/utils.h"

static char *to_string(const wchar_t *unicode_string)
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

bool create_d3d12_gpu_device(Gpu_Device &device)
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
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            //D3D12_MESSAGE_ID_COMMAND_LIST_DRAW_VERTEX_BUFFER_NOT_SET,
            //D3D12_MESSAGE_ID_COMMAND_LIST_DRAW_INDEX_BUFFER_NOT_SET
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

Viewport::Viewport()
{
}

Viewport::Viewport(const Size_f32 &size)
{
    width = size.width;
    height = size.height;
}

Viewport::~Viewport()
{
}

D3D12_RESOURCE_STATES to_d3d12_resource_state(const Resource_State &resource_state)
{
    switch (resource_state) {
        case RESOURCE_STATE_COMMON:
            return D3D12_RESOURCE_STATE_COMMON;
        case RESOURCE_STATE_GENERIC_READ:
            return D3D12_RESOURCE_STATE_GENERIC_READ;
        case RESOURCE_STATE_COPY_DEST:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case RESOURCE_STATE_COPY_SOURCE:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case RESOURCE_STATE_RENDER_TARGET:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case RESOURCE_STATE_PRESENT:
            return D3D12_RESOURCE_STATE_PRESENT;
        case RESOURCE_STATE_DEPTH_WRITE:
            return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
            return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case RESOURCE_STATE_ALL_SHADER_RESOURCE:
            return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
    assert(false);
    return (D3D12_RESOURCE_STATES)0;
}


Clear_Value::Clear_Value() : type(CLEAR_VALUE_UNKNOWN)
{
}

Clear_Value::Clear_Value(Color &_color)
{
    type = CLEAR_VALUE_COLOR;
    color = _color;
}

Clear_Value::Clear_Value(float _depth, u8 _stencil)
{
    type = CLEAR_VALUE_DEPTH_STENCIL;
    depth = _depth;
    stencil = _stencil;
}

Clear_Value::~Clear_Value()
{
}

Subresource_Footprint::Subresource_Footprint()
{
    ZeroMemory(&places_subresource_footprint, sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT));
}

Subresource_Footprint::Subresource_Footprint(u32 subresource_index, u32 row_count, u64 row_size, D3D12_PLACED_SUBRESOURCE_FOOTPRINT &places_subresource_footprint)
    : subresource_index(subresource_index), row_count(row_count), row_size(row_size), places_subresource_footprint(places_subresource_footprint)
{
}

Subresource_Footprint::~Subresource_Footprint()
{
}

D3D12_SUBRESOURCE_FOOTPRINT Subresource_Footprint::d3d12_subresource_footprint()
{
    return places_subresource_footprint.Footprint;
}

Resource_Footprint::Resource_Footprint()
{
}

Resource_Footprint::~Resource_Footprint()
{
}

Subresource_Footprint Resource_Footprint::get_subresource_footprint(u32 index)
{
    return subresource_footprints[index];
}
