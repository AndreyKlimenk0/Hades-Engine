#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <stdlib.h>

#include "../libs/number_types.h"
#include "../libs/structures/array.h"
#include "../libs/structures/queue.h"
#include "../libs/structures/hash_table.h"
#include "../libs/math/structures.h"

#include "gpu_data.h"
#include "renderer.h"
#include "render_passes.h"
#include "render_api/render.h"

struct Win32_Window;
struct Variable_Service;

struct View_Plane {
	u32 width;
	u32 height;
	float ratio;
	float fov;
	float near_plane;
	float far_plane;

	Matrix4 perspective_matrix;
	Matrix4 orthographic_matrix;

	void update(u32 _fov, u32 _width, u32 _height, float _near_plane, float _far_plane);
};

struct Depth_Stencil_Texture_Desc {
	String name;
	u32 width = 0;
	u32 height = 0;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	Clear_Value clear_value;
};

struct Render_Target_Texture_Desc {
	String name;
	u32 width = 0;
	u32 height = 0;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	Clear_Value clear_value;
};

struct Pipeline_Resource_Manager {
	Render_Device *render_device = NULL;

	Buffer *global_buffer = NULL;
	Buffer *frame_info_buffer = NULL;

	Sampler *anisotropic_sampler = NULL;
	Sampler *linear_sampler = NULL;
	Sampler *point_sampler = NULL;

	Array<Buffer *> buffers;
	Array<Texture *> textures;
	
	Hash_Table<String, Texture *> texture_table;
	Texture_Desc default_texture_desc;
	Depth_Stencil_Texture_Desc default_depth_stencil_desc;
	Render_Target_Texture_Desc default_render_target_desc;

	void init(Render_Device *_render_device, Texture_Desc *back_buffer_texture_desc);
	void update_common_constant_buffers();

	Texture *read_texture(const char *texture_name);
	
	Texture *create_texture(const char *texture_name, Texture_Desc *texture_desc = NULL);
	Texture *create_render_target(const char *texture_name, Render_Target_Texture_Desc *render_target_desc = NULL);
	Texture *create_depth_stencil(const char *texture_name, Depth_Stencil_Texture_Desc *depth_stencil_desc = NULL);
};

struct Command_List_Allocator {
	Command_List_Allocator();
	~Command_List_Allocator();
	
	u64 frame_number = 0;

	Array<Command_List *> *command_list_table[4];

	Queue<Pair<u64, Command_List *>> flight_command_lists;
	Array<Command_List* > copy_command_lists;
	Array<Command_List* > compute_command_lists;
	Array<Command_List* > graphics_command_lists;

	Render_Device *render_device = NULL;

	void init(Render_Device *_render_device, u64 frame_start);
	void finish_frame(u64 completed_frame);

	Command_List *allocate_command_list(Command_List_Type command_list_type);
};

struct Render_Pass_Submission {
	Render_Pass *render_pass = NULL;
	void *context = NULL;
	void *args = NULL;
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

	View_Plane window_view_plane;

	Render_Device *render_device = NULL;
	Swap_Chain *swap_chain = NULL;

	Fence *frame_fence = NULL;
	Command_Queue *compute_queue = NULL;
	Command_Queue *graphics_queue = NULL;

	struct Render_Passes {
		Shadows_Pass shadows_pass;
		Forward_Pass forward_pass;
		Silhouette_Pass silhouette_pass;
		Outlining_Pass outlining_pass;
		Render_2D_Pass render2d_pass;
	} passes;
	
	Array<Render_Pass_Submission> render_pass_submissions;

	Command_List_Allocator command_list_allocator;
	Pipeline_Resource_Manager pipeline_resource_manager;

	Render_2D render_2d;

	void init(Win32_Window *win32_window, Variable_Service *variable_service);
	void init_passes();

	void resize(u32 window_width, u32 window_height);
	void flush();

	void notify_start_frame();
	void notify_end_frame();
	void render();

	Size_u32 get_window_size();
};
#endif
