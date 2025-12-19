#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <stdlib.h>

#include "../libs/number_types.h"
#include "../libs/structures/array.h"
#include "../libs/structures/queue.h"
#include "../libs/structures/hash_table.h"
#include "../libs/math/structures.h"

#include "gpu_data.h"
#include "render_apiv2/render_device.h"

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
	Depth_Stencil_Texture_Desc() {};
	Depth_Stencil_Texture_Desc(u32 width, u32 height) {};
	~Depth_Stencil_Texture_Desc() {};

	u32 width = 0;
	u32 height = 0;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	Clear_Value clear_value;
};

struct Render_Target_Texture_Desc {
	u32 width = 0;
	u32 height = 0;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	Clear_Value clear_value;
};

struct Resource_Manager {
	//Sampler_Descriptor point_sampler_descriptor;
	//Sampler_Descriptor linear_sampler_descriptor;
	//Sampler_Descriptor anisotropic_sampler_descriptor;

	u64 frame_count = 0;
	Buffer *frame_info_buffer = NULL;

	void init();
	void update_frame_info_constant_buffer(GPU_Frame_Info *frame_info);

	Array<Buffer *> buffers;
	Array<Texture *> textures;
	
	Hash_Table<String, Texture *> texture_table;
	Depth_Stencil_Texture_Desc default_depth_stencil_desc;
	Render_Target_Texture_Desc default_render_target_desc;
	
	//Resource_Allocator resource_allocator;
	//Buffer *create_buffer(Buffer_Type buffer_type, Buffer_Desc *buffer_desc);
	Texture *create_texture(Texture_Desc *texture_desc);
	
	Texture *find_depth_stencil_texture(const char *texture_name);
	Texture *create_depth_stencil_texture(const char *texture_name, Depth_Stencil_Texture_Desc *depth_stencil_desc = NULL);
};

const u32 RENDER_TARGET_BUFFER_BUFFER = 0x1;

struct Render_Command_Buffer {
	Pipeline_State *current_pipeline = NULL;

	Copy_Command_List *copy_command_list = NULL;
	Compute_Command_List *compute_command_list = NULL;
	Graphics_Command_List *graphics_command_list = NULL;

	Resource_Manager *resource_manager = NULL;
	Render_System *render_sys = NULL;

	Rect_u32 last_applied_clip_rect;
	Viewport last_applied_viewport;

	void create(Render_Device *render_device);
	void setup_common_compute_pipeline_resources(Root_Signature *root_signature);
	void setup_common_graphics_pipeline_resources(Root_Signature *root_signature);

	void apply(Pipeline_State *pipeline_state);
	void apply_compute_pipeline(Pipeline_State *pipeline_state);
	void apply_graphics_pipeline(Pipeline_State *pipeline_state);

	void bind_buffer(u32 shader_register, u32 shader_space, Shader_Register type, Buffer *buffer);
	void bind_texture(u32 shader_register, u32 shader_space, Shader_Register type, Texture *texture);

	void clear_depth_stencil(Texture *depth_stencil_texture, float depth = 1.0f, u8 stencil = 0);
	void clear_render_target(Texture *render_target_texture, const Color &color);
	
	void set_depth_stencil(Texture *depth_stencil_texture);
	void set_back_buffer_as_render_target(Texture *depth_stencil_texture);

	void set_clip_rect(Rect_u32 *clip_rect);
	void set_viewport(Viewport *viewport);
	
	void draw(u32 vertex_count);
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

	View_Plane window_view_plane;

	Fence frame_fence;

	Render_Device *render_device = NULL;
	Swap_Chain swap_chain;

	Command_Queue copy_queue;
	Command_Queue compute_queue;
	Command_Queue graphics_queue;

	Resource_Allocator resource_allocator;
	Descriptor_Heap_Pool descriptors_pool;
	Resource_Manager resource_manager;
	Copy_Manager copy_manager;

	Render_Command_Buffer command_buffer;

	Array<Texture> back_buffer_textures;

	Generate_Mipmaps generate_mipmaps;

	Array<Render_Pass *> passes;

	void init(Win32_Window *win32_window, Variable_Service *variable_service);
	void init_vars(Win32_Window *win32_window, Variable_Service *variable_service);
	void init_passes();

	void resize(u32 window_width, u32 window_height);

	void flush();
	void render();

	Size_u32 get_window_size();
};
#endif
