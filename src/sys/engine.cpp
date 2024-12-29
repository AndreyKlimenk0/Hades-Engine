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


#define DRAW_TEST_GUI 0

static Engine *engine = NULL;

static Font *performance_font = NULL;
//static Render_Primitive_List render_list;

static const String DEFAULT_LEVEL_NAME = "unnamed_level";
static const String LEVEL_EXTENSION = ".hl";

static void init_performance_displaying()
{
	performance_font = engine->font_manager.get_font("consola", 14);
	if (!performance_font) {
		assert(false);
	}
	//Render_Font *render_font = engine->render_sys.render_2d.get_render_font(performance_font);
	//render_list = Render_Primitive_List(&engine->render_sys.render_2d, performance_font, render_font);
}

static void display_performance(s64 fps, s64 frame_time)
{
	char *test = format("Fps", fps);
	char *test2 = format("Frame time {} ms", frame_time);
	u32 text_width = performance_font->get_text_width(test2);

	//s32 x = Render_System::screen_width - text_width - 10;
	//render_list.add_text(x, 5, test);
	//render_list.add_text(x, 20, test2);

	free_string(test);
	free_string(test2);

	//engine->render_sys.render_2d.add_render_primitive_list(&render_list);
}

void Engine::init_base()
{
	engine = this;
	init_os_path();
	init_commands();
	var_service.load("all.variables");
}

#include "../render/render_api/base.h"
#include "../render/render_api/swap_chain.h"
#include "../render/render_api/command.h"
#include "../render/render_api/fence.h"
#include "../render/render_api/resource.h"
#include "../render/render_api/descriptor_heap.h"
#include "../render/render_api/root_signature.h"
#include "../render/render_api/pipeline_state.h"


#include <windows.h>
#include "../win32/win_helpers.h"


//Command_Allocator command_allocators[MAX_BACK_BUFFER_NUMBER];
//GPU_Resource back_buffers[MAX_BACK_BUFFER_NUMBER];
//Command_Queue command_queue;
//Graphics_Command_List command_list;
//
//Command_Allocator copy_command_allocator;
//Command_Queue copy_command_queue;
//Copy_Command_List copy_command_list;
//
//Fence fence;
//HANDLE fence_event;
//RT_Descriptor_Heap render_target_desc_heap;
//DS_Descriptor_Heap depth_stencil_desc_heap;
//Shader_Descriptor_Heap shader_visible_desc_heap;
//Root_Signature box_shader_signature;
//
////ComPtr<ID3D12PipelineState> pipeline;
//Pipeline_State pipeline;
//
//D3D12_RECT clip_rect;
//D3D12_VIEWPORT viewport;
//
//DS_Descriptor depth_texture_descriptor;
//GPU_Resource depth_texture;
//
//
//
//Buffer vertex_buffer;
//Buffer index_buffer;
//
//u64 frame_fence_value = 0;
//u64 frame_fence_values[MAX_BACK_BUFFER_NUMBER];
//
//Constant_Buffer world_matrix_buffer;
//Constant_Buffer view_matrix_buffer;
//Constant_Buffer pers_matrix_buffer;
//
//struct alignas(256) World_Matrix {
//	Matrix4 world_matrix;
//};
//
//struct alignas(256) View_Matrix {
//	Matrix4 view_matrix;
//};
//
//struct alignas(256) Perspective_Matrix {
//	Matrix4 perspective_matrix;
//};
//
//Descriptor_Heap matrix_buffer_descriptor_heap;
//
//struct Gpu_Input_Layout {
//	u32 offset = 0;
//	Array<D3D12_INPUT_ELEMENT_DESC> input_elements;
//
//	void add_layout(const char *semantic_name, DXGI_FORMAT format);
//	D3D12_INPUT_LAYOUT_DESC d3d12_input_layout();
//};
//
//void Gpu_Input_Layout::add_layout(const char *semantic_name, DXGI_FORMAT format)
//{
//	input_elements.push({ semantic_name, 0, format, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
//	offset += dxgi_format_size(format);
//}
//
//D3D12_INPUT_LAYOUT_DESC Gpu_Input_Layout::d3d12_input_layout()
//{
//	return { input_elements.items, input_elements.count };
//}

//void create_depth_texture(Gpu_Device &device, u32 width, u32 height, GPU_Resource &resource)
//{
//	D3D12_HEAP_PROPERTIES heap_properties = {};
//	ZeroMemory(&heap_properties, sizeof(D3D12_HEAP_PROPERTIES));
//	heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
//
//	D3D12_RESOURCE_DESC resource_desc = {};
//	ZeroMemory(&resource_desc, sizeof(D3D12_RESOURCE_DESC));
//	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
//	resource_desc.Alignment = 0;
//	resource_desc.Width = width;
//	resource_desc.Height = height;
//	resource_desc.DepthOrArraySize = 1;
//	resource_desc.MipLevels = 1;
//	resource_desc.Format = DXGI_FORMAT_D32_FLOAT;
//	resource_desc.SampleDesc.Count = 1;
//	resource_desc.SampleDesc.Quality = 0;
//	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
//	resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
//
//	D3D12_CLEAR_VALUE optimizedClearValue = {};
//	optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
//	optimizedClearValue.DepthStencil = { 1.0f, 0 };
//
//	device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearValue, IID_PPV_ARGS(resource.release_and_get_address()));
//}

void Engine::init(Win32_Window *window)
{
	bool windowed = true;
	bool vsync = false;
	s32 back_buffer_count = 3;

	Variable_Service *rendering_settings = var_service.find_namespace("rendering");
	ATTACH(rendering_settings, vsync);
	ATTACH(rendering_settings, windowed);
	ATTACH(rendering_settings, back_buffer_count);

	shader_manager.init();

	render_sys.init(window, &var_service);

	//Gpu_Device gpu_device;

	//if (!create_d3d12_gpu_device(gpu_device)) {
	//	return;
	//}

	//ZeroMemory(&viewport, sizeof(D3D12_VIEWPORT));
	//viewport.Width = window->width;
	//viewport.Height = window->height;
	//viewport.MinDepth = 0.0f;
	//viewport.MaxDepth = 1.0f;

	//ZeroMemory(&clip_rect, sizeof(D3D12_RECT));
	//clip_rect.right = window->width;
	//clip_rect.bottom = window->height;

	//fence.create(gpu_device);

	//command_queue.create(gpu_device, COMMAND_LIST_TYPE_DIRECT);
	//
	//copy_command_queue.create(gpu_device, COMMAND_LIST_TYPE_COPY);
	//copy_command_allocator.create(gpu_device, COMMAND_LIST_TYPE_COPY);
	//copy_command_list.create(gpu_device, copy_command_allocator);

	//bool allow_tearing = false;
	//if (!vsync && windowed && check_tearing_support()) {
	//	swap_chain_present.sync_interval = 0;
	//	swap_chain_present.flags = DXGI_PRESENT_ALLOW_TEARING;
	//	allow_tearing = true;
	//}

	//swap_chain.create(allow_tearing, back_buffer_count, window->width, window->height, window->handle, command_queue);

	//render_target_desc_heap.create(gpu_device, back_buffer_count);
	//depth_stencil_desc_heap.create(gpu_device, 1);
	//shader_visible_desc_heap.create(gpu_device, 3);

	//create_depth_texture(gpu_device, window->width, window->height, depth_texture);

	//depth_texture_descriptor = depth_stencil_desc_heap.place_descriptor(0, depth_texture);

	//for (u32 i = 0; i < (u32)back_buffer_count; i++) {
	//	swap_chain.get_buffer(i, back_buffers[i]);
	//	render_target_desc_heap.place_descriptor(i, back_buffers[i]);
	//	
	//	command_allocators[i].create(gpu_device, COMMAND_LIST_TYPE_DIRECT);
	//}
	//back_buffer_index = swap_chain.get_current_back_buffer_index();

	//command_list.create(gpu_device, command_allocators[back_buffer_index]);

	//fence_event = create_event_handle(); // The event must be close on engine shutdown.

	//world_matrix_buffer.create(gpu_device, sizeof(World_Matrix));
	//view_matrix_buffer.create(gpu_device, sizeof(View_Matrix));
	//pers_matrix_buffer.create(gpu_device, sizeof(Perspective_Matrix));

	//world_matrix_buffer.get()->SetName(L"World matrix buffer");
	//view_matrix_buffer.get()->SetName(L"View matrix buffer");
	//pers_matrix_buffer.get()->SetName(L"Pers matrix buffer");

	//auto world_matrix = make_identity_matrix();
	//auto position = Vector3(0.0f, 0.0f, -10.0f);
	//auto direction = Vector3::base_z;
	//auto view_matrix = make_look_at_matrix(position, position + direction, Vector3::base_y);
	//auto perspective_matrix = make_perspective_matrix(90, 16.0f / 9.0f, 1.0f, 1000.0f);

	//world_matrix_buffer.update(world_matrix);
	//view_matrix_buffer.update(view_matrix);
	//pers_matrix_buffer.update(perspective_matrix);

	//auto world_matrix_cb_desc = shader_visible_desc_heap.place_cb_descriptor(0, world_matrix_buffer);
	//auto view_matrix_cb_desc = shader_visible_desc_heap.place_cb_descriptor(1, view_matrix_buffer);
	//auto pers_matrix_cb_desc = shader_visible_desc_heap.place_cb_descriptor(2, pers_matrix_buffer);

	//box_shader_signature.begin_descriptor_table_parameter(0, VISIBLE_TO_VERTEX_SHADER);
	//box_shader_signature.add_descriptor_range(1, world_matrix_cb_desc);
	//box_shader_signature.add_descriptor_range(2, view_matrix_cb_desc);
	//box_shader_signature.add_descriptor_range(3, pers_matrix_cb_desc);
	//box_shader_signature.end_parameter();

	//box_shader_signature.create(gpu_device, ALLOW_INPUT_LAYOUT_ACCESS | ALLOW_VERTEX_SHADER_ACCESS);
	//box_shader_signature.get()->SetName(L"Box shader_signature");

	//static Gpu_Input_Layout input_layout;
	//input_layout.add_layout("POSITION", DXGI_FORMAT_R32G32B32_FLOAT);
	//input_layout.add_layout("COLOR", DXGI_FORMAT_R32G32B32_FLOAT);

	//Shader *shader = &shader_manager.shaders.draw_box;

	//Rasterization_Desc rasterizer_desc;
	//Blending_Desc blend_desc;

	//const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	//{
	//	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	//	{ "COLOR",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	//};

	//Render_Pipeline_Desc render_pipeline_desc;
	//render_pipeline_desc.root_signature = &box_shader_signature;
	//render_pipeline_desc.vertex_shader = shader;
	//render_pipeline_desc.pixel_shader = shader;
	//render_pipeline_desc.add_layout("POSITION", DXGI_FORMAT_R32G32B32_FLOAT);
	//render_pipeline_desc.add_layout("COLOR", DXGI_FORMAT_R32G32B32_FLOAT);
	//render_pipeline_desc.depth_stencil_format = DXGI_FORMAT_D32_FLOAT;
	//render_pipeline_desc.add_render_target(DXGI_FORMAT_R8G8B8A8_UNORM);

	//	//D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state;
	//	//ZeroMemory(&pipeline_state, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	//	//pipeline_state.pRootSignature = box_shader_signature.get();
	//	////pipeline_state.InputLayout = { inputLayout , 2};
	//	//pipeline_state.InputLayout = input_layout.d3d12_input_layout();
	//	//pipeline_state.VS = shader->vs_bytecode.d3d12_shader_bytecode();
	//	//pipeline_state.PS = shader->ps_bytecode.d3d12_shader_bytecode();
	//	//pipeline_state.RasterizerState = rasterizer_desc.d3d12_rasterizer_desc;
	//	//pipeline_state.BlendState = blend_desc.d3d12_blend_desc;
	//	//pipeline_state.SampleMask = UINT32_MAX;
	//	//pipeline_state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//	//pipeline_state.NumRenderTargets = 1;
	//	//pipeline_state.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//	//pipeline_state.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	//	//pipeline_state.SampleDesc.Count = 1;

	//	//HR(gpu_device->CreateGraphicsPipelineState(&pipeline_state, IID_PPV_ARGS(pipeline.ReleaseAndGetAddressOf())));

	//pipeline.create(gpu_device, render_pipeline_desc);

	//vertex_buffer.create(gpu_device, D3D12_HEAP_TYPE_DEFAULT, 8, sizeof(VertexP3C3));
	//index_buffer.create(gpu_device, D3D12_HEAP_TYPE_DEFAULT, 36, sizeof(u32));

	//Buffer intermediate_vertex_buffer;
	//Buffer intermediate_index_buffer;
	//intermediate_vertex_buffer.create(gpu_device, D3D12_HEAP_TYPE_UPLOAD, 8, sizeof(VertexP3C3));
	//intermediate_index_buffer.create(gpu_device, D3D12_HEAP_TYPE_UPLOAD, 36, sizeof(u32));

	//intermediate_vertex_buffer.update(vertices);
	//intermediate_index_buffer.update(indices);

	//copy_command_list.reset(copy_command_allocator);

	//copy_command_list.get()->CopyResource(vertex_buffer.get(), intermediate_vertex_buffer.get());
	//copy_command_list.get()->CopyResource(index_buffer.get(), intermediate_index_buffer.get());

	//copy_command_list.close();
	//copy_command_queue.execute_command_list(copy_command_list);

	//Fence copy_fence;
	//copy_fence.create(gpu_device);
	//u64 copy_fence_value = 0;
	//copy_command_queue.signal(copy_fence_value, copy_fence);
	//copy_fence.wait_for_gpu(copy_fence_value);
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

	render_sys.render();

	//command_allocators[back_buffer_index].reset();

	//command_list.reset(command_allocators[back_buffer_index]);

	//command_list.resource_barrier(Transition_Resource_Barrier(back_buffers[back_buffer_index], RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET));

	//command_list.clear_render_target_view(render_target_desc_heap.get_cpu_handle(back_buffer_index), Color::LightSteelBlue);
	//command_list.clear_depth_stencil_view(depth_texture_descriptor);

	//
	//command_list.get()->SetGraphicsRootSignature(box_shader_signature.get());
	//command_list.get()->SetPipelineState(pipeline.get());
	//command_list.get()->SetDescriptorHeaps(1, shader_visible_desc_heap.get_address());
	//command_list.get()->SetGraphicsRootDescriptorTable(0, shader_visible_desc_heap.get_base_gpu_handle());
	//
	//command_list.get()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//command_list.set_vertex_buffer(vertex_buffer);
	//command_list.set_index_buffer(index_buffer);
	//
	//command_list.get()->RSSetViewports(1, &viewport);
	//command_list.get()->RSSetScissorRects(1, &clip_rect);
	//const auto temp = render_target_desc_heap.get_cpu_handle(back_buffer_index);
	//const auto temp2 = depth_texture_descriptor.cpu_handle;
	//command_list.get()->OMSetRenderTargets(1, &temp, FALSE, &temp2);

	//command_list.get()->DrawIndexedInstanced(36, 1, 0, 0, 0);

	//command_list.resource_barrier(Transition_Resource_Barrier(back_buffers[back_buffer_index], RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT));
	//
	//command_list.close();

	//command_queue.execute_command_list(command_list);

	//frame_fence_values[back_buffer_index] = command_queue.signal(frame_fence_value, fence);

	//swap_chain.present(swap_chain_present.sync_interval, swap_chain_present.flags);

	//back_buffer_index = swap_chain.get_current_back_buffer_index();
	//
	//fence.wait_for_gpu(frame_fence_values[back_buffer_index]);

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
	//save_game_and_render_world_in_level(current_level_name, &game_world, &render_world);
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

//Descriptor_Table_Range::Descriptor_Table_Range()
//{
//	ZeroMemory(&d3d12_descriptr_range, sizeof(D3D12_DESCRIPTOR_RANGE));
//}
//
//Descriptor_Table_Range::Descriptor_Table_Range(D3D12_DESCRIPTOR_RANGE_TYPE range_type, u32 descriptor_count, u32, u32 space, u32 descriptrs_table_offset) : Descriptor_Table_Range()
//{
//	d3d12_descriptr_range.RangeType;
//	d3d12_descriptr_range.NumDescriptors;
//	d3d12_descriptr_range.BaseShaderRegister;
//	d3d12_descriptr_range.RegisterSpace;
//	d3d12_descriptr_range.OffsetInDescriptorsFromTableStart;
//}
//
//Descriptor_Table_Range::~Descriptor_Table_Range()
//{
//}


Variable_Service *Engine::get_variable_service()
{
	return &engine->var_service;
}
