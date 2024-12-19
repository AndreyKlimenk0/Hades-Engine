#include "base.h"
#include "swap_chain.h"
#include "../../sys/sys.h"
#include "../../sys/utils.h"

void Swap_Chain::create(bool allow_tearing, u32 buffer_count, u32 width, u32 height, HWND handle, Command_Queue &command_queue)
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
