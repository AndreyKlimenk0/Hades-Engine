#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include "render_api.h"
#include "render_world.h"


struct Render_Pass {
	Render_Pass(void *render_context);

	void *render_context = NULL;
	Render_Pipeline_State render_pipeline_state;

	virtual void init(Gpu_Device *gpu_device) = 0;
	virtual bool setup_pipeline_state(Render_System *render_system) = 0;
	virtual void render(Render_Pipeline *render_pipeline) = 0;
};

struct Forwar_Light_Pass : Render_Pass {
	Forwar_Light_Pass(void *render_context);

	Gpu_Buffer pass_data_cbuffer;
	struct Pass_Data {
		u32 mesh_idx;
		u32 world_matrix_idx;
		u32 pad1;
		u32 pad2;
	};

	void init(Gpu_Device *gpu_device);
	bool setup_pipeline_state(Render_System *render_system);
	void render(Render_Pipeline *render_pipeline);
};

struct Draw_Lines_Pass : Render_Pass {
	Draw_Lines_Pass(void *render_context);

	Gpu_Buffer pass_data_cbuffer;
	struct Pass_Data {
		u32 mesh_idx;
		u32 world_matrix_idx;
		u32 pad1;
		u32 pad2;
	};

	void init(Gpu_Device *gpu_device);
	bool setup_pipeline_state(Render_System *render_system);
	void render(Render_Pipeline *render_pipeline);
};

#endif