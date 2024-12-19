#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <stdlib.h>

#include "../libs/number_types.h"
#include "../libs/structures/array.h"
#include "../libs/math/structures.h"

#include "render_api/base.h"
#include "render_api/fence.h"
#include "render_api/texture.h"
#include "render_api/resource.h"
#include "render_api/descriptor_heap.h"
#include "render_api/command.h"
#include "render_api/swap_chain.h"

struct Win32_Window;
struct Variable_Service;

struct View {
	float ratio;
	float fov;
	float near_plane;
	float far_plane;

	Matrix4 perspective_matrix;
	Matrix4 orthogonal_matrix;

	void update_proje0ction_matries(u32 fov_in_degrees, u32 width, u32 height, float _near_plane, float _far_plane);
};

struct Projection_Plane {
	u32 width;
	u32 height;
	float ratio;
	float fov_y_ratio;
	float near_plane;
	float far_plane;
};

struct Projection_Matries {
	Matrix4 perspective_matrix;
	Matrix4 orthogonal_matrix;

	void update();
};

struct Box_Render_Pass {

};

struct Command_List_Allocator {

};

struct Descriptor_Heap_Pool {
	Descriptor_Heap_Pool();
	~Descriptor_Heap_Pool();

	u32 rt_descriptor_count = 0;
	u32 ds_descriptor_count = 0;
	u32 sr_descriptor_count = 0;
	u32 samper_descriptor_count = 0;

	RT_Descriptor_Heap rt_descriptor_heap;
	DS_Descriptor_Heap ds_descriptor_heap;
	CBSRUA_Descriptor_Heap cbsrua_descriptor_heap;
	Sampler_Descriptor_Heap sampler_descriptor_heap;

	void allocate_rt_descriptor(Texture &texture);
	void allocate_ds_descriptor(Texture &texture);
	void allocate_sr_descriptor(Texture &texture);
	void allocate_pool(Gpu_Device &device, u32 descriptors_count);
};

#include "render_api/root_signature.h"
#include "render_api/pipeline_state.h"

struct Box_Pass {
	String name;
	Root_Signature root_signature;
	Pipeline_State pipeline_state;

	void setup();
	void setup_pipeline();
	void setup_root_signature();
	void render();
};

struct Render_System {
	struct Window {
		bool vsync = false;
		bool windowed = true;
		u32 width = 0;
		u32 height = 0;
	} window;

	u32 sync_interval = 0;
	u32 present_flags = 0;
	u32 back_buffer_count = 0;
	u32 back_buffer_index = 0;
	u64 frame_fence_value = 0;

	Fence frame_fence;
	Array<u64> frame_fence_values;

	Gpu_Device gpu_device;
	Swap_Chain swap_chain;

	Command_Queue copy_queue;
	Command_Queue compute_queue;
	Command_Queue graphics_queue;
	Graphics_Command_List command_list;

	Texture back_buffer_depth_texture;
	Array<Texture> back_buffer_textures;

	Descriptor_Heap_Pool descriptors_pool;

	void init(Win32_Window *win32_window, Variable_Service *variable_service);
	void init_vars(Win32_Window *win32_window, Variable_Service *variable_service);
	void resize(u32 window_width, u32 window_height);

	void render();

	Size_u32 get_window_size();
};
#endif
