#include <assert.h>

#include "engine.h"
#include "commands.h"
#include "profiling.h"
#include "../gui/gui.h"
#include "../sys/level.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/os/event.h"
#include "../libs/mesh_loader.h"
#include "../win32/win_time.h"

#include "../gui/test_gui.h"

#include "../render/new_render_api.h"

#define DRAW_TEST_GUI 0

static Engine *engine = NULL;

static Font *performance_font = NULL;
static Render_Primitive_List render_list;

static const String DEFAULT_LEVEL_NAME = "unnamed_level";
static const String LEVEL_EXTENSION = ".hl";

static void init_performance_displaying()
{
	performance_font = engine->font_manager.get_font("consola", 14);
	if (!performance_font) {
		assert(false);
	}
	Render_Font *render_font = engine->render_sys.render_2d.get_render_font(performance_font);
	render_list = Render_Primitive_List(&engine->render_sys.render_2d, performance_font, render_font);
}

static void display_performance(s64 fps, s64 frame_time)
{
	char *test = format("Fps", fps);
	char *test2 = format("Frame time {} ms", frame_time);
	u32 text_width = performance_font->get_text_width(test2);

	s32 x = Render_System::screen_width - text_width - 10;
	render_list.add_text(100, 5, test);
	render_list.add_text(180, 5, test2);

	free_string(test);
	free_string(test2);

	engine->render_sys.render_2d.add_render_primitive_list(&render_list);
}

void Engine::init_base()
{
	engine = this;
	init_os_path();
	init_commands();
	var_service.load("all.variables");
}

d3d12::Swap_Chain swap_chain;
u32 back_buffer_index = 0;

const u32 MAX_BACK_BUFFER_NUMBER = 3;

#include <windows.h>
#include "../win32/win_helpers.h"


Command_Allocator command_allocators[MAX_BACK_BUFFER_NUMBER];
ComPtr<ID3D12Resource> back_buffers[MAX_BACK_BUFFER_NUMBER];
Graphics_Command_List command_list;
Command_Queue command_queue;
Fence fence;
HANDLE fence_event;
Descriptor_Heap back_buffers_desc_heap;

u64 frame_fence_value = 0;
u64 frame_fence_values[MAX_BACK_BUFFER_NUMBER];

void Engine::init(Win32_Window *window)
{
	bool windowed = true;
	bool vsync = false;
	s32 back_buffer_count = 3;

	Variable_Service *rendering_settings = var_service.find_namespace("rendering");
	ATTACH(rendering_settings, vsync);
	ATTACH(rendering_settings, windowed);
	ATTACH(rendering_settings, back_buffer_count);

	d3d12::Gpu_Device gpu_device;

	if (!gpu_device.init()) {
		return;
	}

	gpu_device.create_fence(fence);

	gpu_device.create_command_queue(command_queue);

	bool allow_tearing = false;
	if (!vsync && windowed && check_tearing_support()) {
		swap_chain_present.sync_interval = 0;
		swap_chain_present.flags = DXGI_PRESENT_ALLOW_TEARING;
		allow_tearing = true;
	}

	swap_chain.init(allow_tearing, back_buffer_count, window->width, window->height, window->handle, command_queue);

	gpu_device.create_rtv_descriptor_heap(2, back_buffers_desc_heap);

	for (u32 i = 0; i < (u32)back_buffer_count; i++) {
		swap_chain.get_buffer(i, back_buffers[i]);
		gpu_device.device->CreateRenderTargetView(back_buffers[i].Get(), nullptr, back_buffers_desc_heap.get_cpu_heap_descriptor_handle(i));

		gpu_device.create_command_allocator(COMMAND_LIST_TYPE_DIRECT, command_allocators[i]);
	}
	back_buffer_index = swap_chain.get_current_back_buffer_index();

	gpu_device.create_command_list(command_allocators[back_buffer_index], command_list);

	fence_event = create_event_handle(); // The event must be close on engine shutdown.

}

void wait_for_gpu(u64 fence_value, HANDLE fence_event)
{
	if (fence->GetCompletedValue() < fence_value) {
		fence->SetEventOnCompletion(fence_value, fence_event);
		WaitForSingleObject(fence_event, INFINITE);
	}
}

#include "sys.h"

void Engine::frame()
{
	BEGIN_FRAME();

	static s64 fps = 60;
	static s64 frame_time = 1000;

	s64 start_time = milliseconds_counter();
	s64 ticks_counter = cpu_ticks_counter();

	BEGIN_TASK("Update");
	pump_events();
	run_event_loop();

	command_allocators[back_buffer_index].reset();

	command_list.reset(command_allocators[back_buffer_index]);

	command_list.resource_barrier(Transition_Resource_Barrier(back_buffers[back_buffer_index], RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET));

	command_list.clear_render_target_view(back_buffers_desc_heap.get_cpu_heap_descriptor_handle(back_buffer_index), Color::Red);

	command_list.resource_barrier(Transition_Resource_Barrier(back_buffers[back_buffer_index], RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT));

	command_list.close();

	command_queue.execute_command_list(command_list);

	frame_fence_values[back_buffer_index] = command_queue.signal(frame_fence_value, fence);

	swap_chain.present(swap_chain_present.sync_interval, swap_chain_present.flags);

	back_buffer_index = swap_chain.get_current_back_buffer_index();
	
	wait_for_gpu(frame_fence_values[back_buffer_index], fence_event);

	clear_event_queue();

	fps = cpu_ticks_per_second() / (cpu_ticks_counter() - ticks_counter);
	frame_time = milliseconds_counter() - start_time;
	
	print("Fps", fps);
}

void Engine::shutdown()
{
	if (current_level_name.is_empty()) {
		int counter = 0;
		String index = "";
		while (true) {
			String full_path_to_map_file;
			build_full_path_to_level_file(DEFAULT_LEVEL_NAME + index + LEVEL_EXTENSION, full_path_to_map_file);
			if (file_exists(full_path_to_map_file.c_str())) {
				char *str_counter = ::to_string(counter++);
				index = str_counter;
				free_string(str_counter);
				continue;
			}
			break;
		}
		current_level_name = DEFAULT_LEVEL_NAME + index + LEVEL_EXTENSION;
	}
	save_game_and_render_world_in_level(current_level_name, &game_world, &render_world);
	gui::shutdown();
	var_service.shutdown();
}

void Engine::set_current_level_name(const String &level_name)
{
	assert(level_name.len > 0);

	current_level_name = level_name + LEVEL_EXTENSION;
}

bool Engine::initialized()
{
	return engine ? engine->is_initialized : false;
}

void Engine::resize_window(u32 window_width, u32 window_height)
{
	engine->render_sys.resize(window_width, window_height);
}

Engine *Engine::get_instance()
{
	return engine;
}

Game_World *Engine::get_game_world()
{
	return &engine->game_world;
}

Render_World *Engine::get_render_world()
{
	return &engine->render_world;
}

Render_System *Engine::get_render_system()
{
	return &engine->render_sys;
}

Font_Manager *Engine::get_font_manager()
{
	return &engine->font_manager;
}

Variable_Service *Engine::get_variable_service()
{
	return &engine->var_service;
}
