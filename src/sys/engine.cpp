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
static Render_Primitive_List render_list;

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

	s32 x = Render_System::screen_width - text_width - 10;
	render_list.add_text(100, 5, test);
	render_list.add_text(180, 5, test2);

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

Swap_Chain swap_chain;
u32 back_buffer_index = 0;

const u32 MAX_BACK_BUFFER_NUMBER = 3;

#include <windows.h>
#include "../win32/win_helpers.h"


Command_Allocator command_allocators[MAX_BACK_BUFFER_NUMBER];
GPU_Resource back_buffers[MAX_BACK_BUFFER_NUMBER];
Command_Queue command_queue;
Graphics_Command_List command_list;

Command_Allocator copy_command_allocator;
Command_Queue copy_command_queue;
Copy_Command_List copy_command_list;

Fence fence;
HANDLE fence_event;
RT_Descriptor_Heap render_target_desc_heap;
ComPtr<ID3D12RootSignature> signature;
ComPtr<ID3D12PipelineState> pipeline;

GPU_Resource depth_texture;

struct VertexP3C3 {
	Vector3 vertex;
	Vector3 color;
};

static VertexP3C3 vertices[8] = {
	{ Vector3(-1.0f, -1.0f, -1.0f), Vector3(0.0f, 0.0f, 0.0f) }, // 0
	{ Vector3(-1.0f,  1.0f, -1.0f), Vector3(0.0f, 1.0f, 0.0f) }, // 1
	{ Vector3(1.0f,  1.0f, -1.0f), Vector3(1.0f, 1.0f, 0.0f) }, // 2
	{ Vector3(1.0f, -1.0f, -1.0f), Vector3(1.0f, 0.0f, 0.0f) }, // 3
	{ Vector3(-1.0f, -1.0f,  1.0f), Vector3(0.0f, 0.0f, 1.0f) }, // 4
	{ Vector3(-1.0f,  1.0f,  1.0f), Vector3(0.0f, 1.0f, 1.0f) }, // 5
	{ Vector3(1.0f,  1.0f,  1.0f), Vector3(1.0f, 1.0f, 1.0f) }, // 6
	{ Vector3(1.0f, -1.0f,  1.0f), Vector3(1.0f, 0.0f, 1.0f) }  // 7
};

static u32 indices[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};


GPU_Resource vertex_buffer;
GPU_Resource index_buffer;

u64 frame_fence_value = 0;
u64 frame_fence_values[MAX_BACK_BUFFER_NUMBER];

GPU_Resource world_matrix_buffer;
GPU_Resource view_matrix_buffer;
GPU_Resource pers_matrix_buffer;

struct alignas(256) World_Matrix {
	Matrix4 world_matrix;
};

struct alignas(256) View_Matrix {
	Matrix4 view_matrix;
};

struct alignas(256) Perspective_Matrix {
	Matrix4 perspective_matrix;
};

Descriptor_Heap matrix_buffer_descriptor_heap;

struct Gpu_Input_Layout {
	u32 offset = 0;
	Array<D3D12_INPUT_ELEMENT_DESC> input_elements;

	void add_layout(const char *semantic_name, DXGI_FORMAT format);
	D3D12_INPUT_LAYOUT_DESC d3d12_input_layout();
};

void Gpu_Input_Layout::add_layout(const char *semantic_name, DXGI_FORMAT format)
{
	input_elements.push({ semantic_name, 0, format, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
	offset += dxgi_format_size(format);
}

D3D12_INPUT_LAYOUT_DESC Gpu_Input_Layout::d3d12_input_layout()
{
	return { input_elements.items, input_elements.count };
}

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

	Gpu_Device gpu_device;

	if (!create_d3d12_gpu_device(gpu_device)) {
		return;
	}

	fence.create(gpu_device);

	command_queue.create(gpu_device, COMMAND_LIST_TYPE_DIRECT);
	
	copy_command_queue.create(gpu_device, COMMAND_LIST_TYPE_COPY);
	copy_command_allocator.create(gpu_device, COMMAND_LIST_TYPE_COPY);
	copy_command_list.create(gpu_device, copy_command_allocator);

	bool allow_tearing = false;
	if (!vsync && windowed && check_tearing_support()) {
		swap_chain_present.sync_interval = 0;
		swap_chain_present.flags = DXGI_PRESENT_ALLOW_TEARING;
		allow_tearing = true;
	}

	swap_chain.init(allow_tearing, back_buffer_count, window->width, window->height, window->handle, command_queue);

	render_target_desc_heap.create(gpu_device, back_buffer_count);

	for (u32 i = 0; i < (u32)back_buffer_count; i++) {
		swap_chain.get_buffer(i, back_buffers[i]);
		render_target_desc_heap.place_decriptor(i, back_buffers[i]);
		
		command_allocators[i].create(gpu_device, COMMAND_LIST_TYPE_DIRECT);
	}
	back_buffer_index = swap_chain.get_current_back_buffer_index();

	command_list.create(gpu_device, command_allocators[back_buffer_index]);

	fence_event = create_event_handle(); // The event must be close on engine shutdown.

	//D3D12_RESOURCE_DESC cbuffer_resource_desc;
	//ZeroMemory(&cbuffer_resource_desc, sizeof(D3D12_RESOURCE_DESC));

	//cbuffer_resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	//cbuffer_resource_desc.Width = sizeof(Frame);
	//cbuffer_resource_desc.Height = 1;
	//cbuffer_resource_desc.DepthOrArraySize = 1;
	//cbuffer_resource_desc.Format = DXGI_FORMAT_UNKNOWN;

	//gpu_device.device->CreateCommittedResource();

	//Texture2D_Desc depth_texture_desc;
	//depth_texture_desc.width = window->width;
	//depth_texture_desc.width = window->height;
	//depth_texture_desc.format = DXGI_FORMAT_D32_FLOAT;
	//depth_texture_desc.options = ALLOW_DEPTH_STENCIL;

	//gpu_device.create_texture2d(depth_texture_desc, RESOURCE_STATE_DEPTH_WRITE, depth_texture);


	//gpu_device.create_commited_resource(sizeof(World_Matrix), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, world_matrix_buffer);
	//gpu_device.create_commited_resource(sizeof(View_Matrix), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, view_matrix_buffer);
	//gpu_device.create_commited_resource(sizeof(Perspective_Matrix), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, pers_matrix_buffer);


	//gpu_device.create_cbv_srv_uav_descriptor_heap(3, matrix_buffer_descriptor_heap);

	//D3D12_CONSTANT_BUFFER_VIEW_DESC world_matrix_cbuffer_view_desc;
	//world_matrix_cbuffer_view_desc.BufferLocation = world_matrix_buffer.get_gpu_address();
	//world_matrix_cbuffer_view_desc.SizeInBytes = sizeof(World_Matrix);

	//gpu_device.device->CreateConstantBufferView(&world_matrix_cbuffer_view_desc, matrix_buffer_descriptor_heap.get_cpu_descriptor_heap_handle(0));

	//D3D12_CONSTANT_BUFFER_VIEW_DESC view_matrix_cbuffer_view_desc;
	//view_matrix_cbuffer_view_desc.BufferLocation = view_matrix_buffer.get_gpu_address();
	//view_matrix_cbuffer_view_desc.SizeInBytes = sizeof(View_Matrix);

	//gpu_device.device->CreateConstantBufferView(&view_matrix_cbuffer_view_desc, matrix_buffer_descriptor_heap.get_cpu_descriptor_heap_handle(1));

	//D3D12_CONSTANT_BUFFER_VIEW_DESC pers_matrix_cbuffer_view_desc;
	//pers_matrix_cbuffer_view_desc.BufferLocation = pers_matrix_buffer.get_gpu_address();
	//pers_matrix_cbuffer_view_desc.SizeInBytes = sizeof(Perspective_Matrix);

	//gpu_device.device->CreateConstantBufferView(&pers_matrix_cbuffer_view_desc, matrix_buffer_descriptor_heap.get_cpu_descriptor_heap_handle(2));


	D3D12_DESCRIPTOR_RANGE1 world_matrix_range;
	ZeroMemory(&world_matrix_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
	world_matrix_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	world_matrix_range.NumDescriptors = 1;
	world_matrix_range.BaseShaderRegister = 1;
	world_matrix_range.RegisterSpace = 0;
	world_matrix_range.OffsetInDescriptorsFromTableStart = 0;

	D3D12_DESCRIPTOR_RANGE1 view_matrix_range;
	ZeroMemory(&view_matrix_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
	view_matrix_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	view_matrix_range.NumDescriptors = 1;
	view_matrix_range.BaseShaderRegister = 2;
	view_matrix_range.RegisterSpace = 0;
	view_matrix_range.OffsetInDescriptorsFromTableStart = 1;

	D3D12_DESCRIPTOR_RANGE1 pers_matrix_range;
	ZeroMemory(&pers_matrix_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
	pers_matrix_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	pers_matrix_range.NumDescriptors = 1;
	pers_matrix_range.BaseShaderRegister = 3;
	pers_matrix_range.RegisterSpace = 0;
	pers_matrix_range.OffsetInDescriptorsFromTableStart = 2;

	D3D12_DESCRIPTOR_RANGE1 ranges[] = { world_matrix_range, view_matrix_range, pers_matrix_range };

	D3D12_ROOT_PARAMETER1 param;
	ZeroMemory(&param, sizeof(D3D12_ROOT_PARAMETER1));
	param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param.DescriptorTable.NumDescriptorRanges = 3;
	param.DescriptorTable.pDescriptorRanges = ranges;
	param.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;


	D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
	ZeroMemory(&root_signature_desc, sizeof(D3D12_VERSIONED_ROOT_SIGNATURE_DESC));
	root_signature_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	root_signature_desc.Desc_1_1.NumParameters = 1;
	root_signature_desc.Desc_1_1.pParameters = &param;
	root_signature_desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | 
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errors;
	D3D12SerializeVersionedRootSignature(&root_signature_desc, &signatureBlob, &errors);
	//HR(gpu_device.device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(signature.ReleaseAndGetAddressOf())));

	static Gpu_Input_Layout input_layout;
	input_layout.add_layout("POSITION", DXGI_FORMAT_R32G32B32_FLOAT);
	input_layout.add_layout("COLOR", DXGI_FORMAT_R32G32B32_FLOAT);

	Shader *shader = &shader_manager.shaders.draw_box;

	//D12_Rasterizer_Desc rasterizer_desc;
	//D12_Blend_Desc blend_desc;

	const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	//D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state;
	//ZeroMemory(&pipeline_state, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	//pipeline_state.pRootSignature = signature.Get();
	//pipeline_state.InputLayout = { inputLayout , 2};
	////pipeline_state.InputLayout = input_layout.d3d12_input_layout();
	//pipeline_state.VS = shader->vs_bytecode.d3d12_shader_bytecode();
	//pipeline_state.PS = shader->ps_bytecode.d3d12_shader_bytecode();
	//pipeline_state.RasterizerState = rasterizer_desc.d3d12_rasterizer_desc;
	//pipeline_state.BlendState = blend_desc.d3d12_blend_desc;
	//pipeline_state.SampleMask = UINT32_MAX;
	//pipeline_state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//pipeline_state.NumRenderTargets = 1;
	//pipeline_state.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//pipeline_state.SampleDesc.Count = 1;

	//HR(gpu_device.device->CreateGraphicsPipelineState(&pipeline_state, IID_PPV_ARGS(pipeline.ReleaseAndGetAddressOf())));


	//gpu_device.create_default_resource(sizeof(VertexP3C3) * 8, vertex_buffer);
	//gpu_device.create_default_resource(sizeof(u32) * 36, index_buffer);

	//copy_command_list.reset(copy_command_allocator);

	
		//GPU_Resource intermediate_resource1;
		//gpu_device.create_upload_resource(sizeof(VertexP3C3) * 8, intermediate_resource1);
		//u8 *ptr1 = intermediate_resource1.map();
		//memcpy((void *)ptr1, (void *)&vertices, sizeof(VertexP3C3) * 8);
		//intermediate_resource1.unmap();

		//copy_command_list.get()->CopyResource(vertex_buffer.get(), intermediate_resource1.get());
	
	
		//GPU_Resource intermediate_resource2;
		//gpu_device.create_upload_resource(sizeof(u32) * 36, intermediate_resource2);
		//u8 *ptr2 = intermediate_resource2.map();
		//memcpy((void *)ptr2, (void *)&vertices, sizeof(u32) * 36);
		//intermediate_resource2.unmap();

		//copy_command_list.get()->CopyResource(index_buffer.get(), intermediate_resource2.get());
	//copy_command_list.close();
	//
	//copy_command_queue.execute_command_list(copy_command_list);
	//Fence temp_fence;
	//gpu_device.create_fence(temp_fence);
	//u64 value = 0;
	//u64 fence_value = copy_command_queue.signal(value, temp_fence);
	//HANDLE fence_handle = create_event_handle();
	//wait_for_gpu(temp_fence, fence_value, fence_handle);
	//close_event_handle(fence_handle);
}

//struct Descriptor_Table_Range {
//	Descriptor_Table_Range();
//	Descriptor_Table_Range(D3D12_DESCRIPTOR_RANGE_TYPE range_type, u32 descriptor_count, u32 shader_register, u32 shader_space, u32 descriptrs_table_offset = 0);
//	virtual ~Descriptor_Table_Range();
//
//	D3D12_DESCRIPTOR_RANGE d3d12_descriptr_range;
//};
//
//struct CBV_Descriptor_Table_Range : Descriptor_Table_Range {
//	CBV_Descriptor_Table_Range() : Descriptor_Table_Range(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, );
//	~CBV_Descriptor_Table_Range() {};
//};
//
//struct SRV_Descriptor_Table_Range : Descriptor_Table_Range {
//	SRV_Descriptor_Table_Range() {};
//	~SRV_Descriptor_Table_Range() {};
//};
//
//struct UAV_Descriptor_Table_Range : Descriptor_Table_Range {
//	UAV_Descriptor_Table_Range() {};
//	~UAV_Descriptor_Table_Range() {};
//};
//
//struct Sampler_Descriptor_Table_Range : Descriptor_Table_Range {
//	Sampler_Descriptor_Table_Range() {};
//	~Sampler_Descriptor_Table_Range() {};
//};
//
//struct Root_Signature {
//	Root_Signature() {};
//	~Root_Signature() {};
//
//	void add_parameter();
//};

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

	command_list.clear_render_target_view(render_target_desc_heap.get_cpu_handle(back_buffer_index), Color::Red);

	command_list.resource_barrier(Transition_Resource_Barrier(back_buffers[back_buffer_index], RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT));

	//command_list.get()->SetGraphicsRootDescriptorTable(1, matrix_buffer_descriptor_heap.get_base_gpu_descriptor_handle());

	command_list.close();

	command_queue.execute_command_list(command_list);

	frame_fence_values[back_buffer_index] = command_queue.signal(frame_fence_value, fence);

	swap_chain.present(swap_chain_present.sync_interval, swap_chain_present.flags);

	back_buffer_index = swap_chain.get_current_back_buffer_index();
	
	fence.wait_for_gpu(frame_fence_values[back_buffer_index]);

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
