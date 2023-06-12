#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include "render_api.h"
#include "render_helpers.h"
#include "../libs/str.h"
#include "../libs/math/matrix.h"
#include "../libs/math/vector.h"

struct Render_Pass {
	struct Pass_Data {
		u32 mesh_idx;
		u32 world_matrix_idx;
		u32 pad1;
		u32 pad2;
	};
	bool initialized = false;
	void *render_context = NULL;
	String name;
	Gpu_Buffer pass_data_cbuffer;
	Render_Pipeline_State render_pipeline_state;

	virtual bool init(void *_render_context, Render_System *render_sys);
	virtual bool validate_render_pipeline(const char *render_pass_name, Render_System *render_system);
	virtual bool setup_pipeline_state(Render_System *render_system) = 0;
	virtual void render(Render_Pipeline *render_pipeline) = 0;
};

struct Forwar_Light_Pass : Render_Pass {
	bool init(void *_render_context, Render_System *render_sys);
	bool setup_pipeline_state(Render_System *render_system);
	void render(Render_Pipeline *render_pipeline);
};

struct Draw_Lines_Pass : Render_Pass {
	bool init(void *_render_context, Render_System *render_sys);
	bool setup_pipeline_state(Render_System *render_system);
	void render(Render_Pipeline *render_pipeline);
};

struct Shadows_Pass : Render_Pass {
	struct Pass_Data {
		u32 mesh_idx;
		u32 world_matrix_idx;
		u32 cascade_view_projection_matrix_idx;
		u32 pad;
	};
	bool init(void *_render_context, Render_System *render_sys);
	bool setup_pipeline_state(Render_System *render_system);
	void render(Render_Pipeline *render_pipeline);
};

struct Debug_Cascade_Shadows_Pass : Render_Pass {
	struct Shadow_Atlas_Info {
		u32 shadow_atlas_width;
		u32 shadow_atlas_height;
		u32 shadow_cascade_width;
		u32 shadow_cascade_height;
		u32 cascaded_shadow_count;
		Pad3 pad;
	};

	Gpu_Buffer shadow_atlas_info_cbuffer;

	bool init(void *_render_context, Render_System *render_sys);
	bool setup_pipeline_state(Render_System *render_system);
	void render(Render_Pipeline *render_pipeline);
};

#endif