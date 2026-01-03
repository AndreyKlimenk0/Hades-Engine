#include "render.h"
#include "d3d12_render_api/d3d12_device.h"

Render_Device *create_render_device(u64 initial_expected_value)
{
	return create_d3d12_render_device(initial_expected_value);
}

Swap_Chain *create_swap_chain(bool allow_tearing, u32 buffer_count, u32 width, u32 height, HWND handle, Command_Queue *command_queue)
{
	return new D3D12_Swap_Chain(allow_tearing, buffer_count, width, height, handle, command_queue);
}
