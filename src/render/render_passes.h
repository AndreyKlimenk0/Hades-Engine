#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include "hlsl.h"
#include "render_api.h"
#include "../libs/str.h"
#include "../libs/color.h"
#include "../libs/number_types.h"
#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"

struct Shader;
struct Gpu_Device;
struct Shader_Manager;
struct Render_World;
struct Render_Pipeline_States;

struct Render_Pass {
	struct Pass_Data {
		u32 mesh_idx;
		u32 world_matrix_idx;
		u32 pad1;
		u32 pad2;
	};
	bool is_valid = false;
	Render_Pipeline_States *render_pipeline_states = NULL;
	String name;
	Gpu_Buffer pass_data_cbuffer;
	Render_Pipeline_State render_pipeline_state;

	virtual void init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states);
	virtual void render(Render_World *render_world, Render_Pipeline *render_pipeline) = 0;
};

struct Forwar_Light_Pass : Render_Pass {
	Gpu_Buffer shadow_atlas_info_cbuffer;

	void init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states);
	void render(Render_World *render_world, Render_Pipeline *render_pipeline);
	void setup_render_pipeline(Shader_Manager *shader_manager, const Depth_Stencil_View &depth_stencil_view, const Render_Target_View &render_target_view, Viewport *viewport);
};

struct Shadows_Pass : Render_Pass {
	void init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states);
	void render(Render_World *render_world, Render_Pipeline *render_pipeline);
	void setup_render_pipeline(Shader_Manager *shader_manager, const Depth_Stencil_View &depth_stencil_view);
};

struct Debug_Cascade_Shadows_Pass : Render_Pass {
	Gpu_Buffer shadow_atlas_info_cbuffer;

	void init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states);
	void render(Render_World *render_world, Render_Pipeline *render_pipeline);
	void setup_render_pipeline(Shader_Manager *shader_manager, const Depth_Stencil_View &depth_stencil_view, const Render_Target_View &render_target_view, Viewport *viewport);
};

struct Outlining_Pass : Render_Pass {
	struct Outlining_Info {
		s32 offset_range = 0;
		Pad3 pad;
		Vector4 color;
	};

	u32 thread_group_count_x = 0;
	u32 thread_group_count_y = 0;
	Shader *outlining_compute_shader = NULL;
	Texture2D *screen_back_buffer = NULL;
	Texture2D *screen_depth_stencil_back_buffer = NULL;
	Texture2D *silhouette_back_buffer = NULL;
	Texture2D *silhoueete_depth_stencil_buffer = NULL;
	Outlining_Info outlining_info;
	Gpu_Buffer outlining_info_cbuffer;
	Array<u32> render_entity_indices;

	void add_render_entity_index(u32 entity_index);
	void delete_render_entity_index(u32 entity_index);
	void reset_render_entity_indices();

	void setup_outlining(u32 outlining_size_in_pixels, const Color &color);

	void init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states);
	void render(Render_World *render_world, Render_Pipeline *render_pipeline);
	void setup_render_pipeline(Shader_Manager *shader_manager, Texture2D *_silhouette_back_buffer, Texture2D *_silhouette_depth_stencil_buffer, Texture2D *_screen_back_buffer, Texture2D *_screen_depth_stencil_back_buffer, Viewport *viewport);
};

struct Voxelization : Render_Pass {
	struct Voxelization_Info {
		u32 voxel_grid_width;
		u32 voxel_grid_height;
		u32 voxel_grid_depth;
		
		u32 voxel_grid_ceil_width;
		u32 voxel_grid_ceil_height;
		u32 voxel_grid_ceil_depth;
		Pad2 pad2;
		Vector3 grid_center;
		Pad1 pad;
		Matrix4 voxel_orthographic_matrix;
		Matrix4 voxel_view_matrix;
	};

	Gpu_Buffer voxelization_info_cbuffer;
	
	void init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states);
	void render(Render_World *render_world, Render_Pipeline *render_pipeline);
	void setup_render_pipeline(Shader_Manager *shader_manager, const Unordered_Access_View &voxel_buffer_view, const Render_Target_View &render_target_view);
};

#endif
