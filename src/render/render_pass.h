#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include "render_api.h"
#include "../libs/str.h"
#include "../libs/color.h"
#include "../libs/ds/array.h"
#include "../libs/math/matrix.h"
#include "../libs/math/vector.h"
#include "../win32/win_types.h"

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

struct Draw_Lines_Pass : Render_Pass {
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

struct Draw_Vertices_Pass : Render_Pass {
	Gpu_Buffer mesh_color_cbuffer;

	void init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states);
	void render(Render_World *render_world, Render_Pipeline *render_pipeline);
	void setup_render_pipeline(Shader_Manager *shader_manager, const Depth_Stencil_View &depth_stencil_view, const Render_Target_View &render_target_view, Viewport *viewport);
};

struct Outlining_Pass : Render_Pass {
	struct Outlining_Info {
		Color outlining_color;
		Matrix4 scaling_matrix;
	};
	Gpu_Buffer outlining_info_cbuffer;
	Gpu_Buffer depth_map_pass_data_cbuffer;
	Array<u32> render_entity_indices;
	Render_Pipeline_State pre_render_pipeline_state;
	
	void add_render_entity_index(u32 entity_index);

	void init(Gpu_Device *gpu_device, Render_Pipeline_States *_render_pipeline_states);
	void render(Render_World *render_world, Render_Pipeline *render_pipeline);
	void setup_render_pipeline(Shader_Manager *shader_manager, const Depth_Stencil_View &depth_stencil_view, const Render_Target_View &render_target_view, Viewport *viewport);
};

#endif