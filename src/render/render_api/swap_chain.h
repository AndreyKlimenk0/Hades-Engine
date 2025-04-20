#ifndef RENDER_API_SWAP_CHAIN_H
#define RENDER_API_SWAP_CHAIN_H

#include <dxgi1_4.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include "command.h"
#include "resource.h"
#include "../../libs/number_types.h"

struct Swap_Chain {
	ComPtr<IDXGISwapChain3> dxgi_swap_chain;

	void create(bool allow_tearing, u32 buffer_count, u32 width, u32 height, HWND handle, Command_Queue &command_queue);
	void resize(u32 width, u32 height);
	void present(u32 sync_interval, u32 flags);
	void get_buffer(u32 buffer_index, GPU_Resource &resource);
	void get_current_buffer(GPU_Resource &resource);
	void release();

	u32 get_current_back_buffer_index();
};

#endif
