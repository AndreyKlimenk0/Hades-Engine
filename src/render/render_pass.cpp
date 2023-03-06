#include "render_pass.h"


Render_Pass::Render_Pass(void *render_context) : render_context(render_context)
{
}

Forwar_Light_Pass::Forwar_Light_Pass(void *render_context) : Render_Pass(render_context)
{
}

void Forwar_Light_Pass::init(Gpu_Device *gpu_device)
{
	gpu_device->create_constant_buffer(sizeof(Forwar_Light_Pass::Pass_Data), &pass_data_cbuffer);
}

bool Forwar_Light_Pass::setup_pipeline_state(Render_System *render_system)
{
	render_pipeline_state.primitive_type = RENDER_PRIMITIVE_TRIANGLES;
	render_pipeline_state.shader_name = "forward_light.hlsl";

	if (!render_pipeline_state.setup(render_system)) {
		print("Forwar_Light_Pass::setup_pipeline_state: Failed to init Pipeline_state");
		return false;
	}
	return true;
}

void Forwar_Light_Pass::render(Render_Pipeline *render_pipeline)
{
	Render_World *render_world = (Render_World *)render_context;

	render_pipeline->apply(&render_pipeline_state);

	render_pipeline->set_vertex_shader_resource(render_world->mesh_struct_buffer);
	render_pipeline->set_vertex_shader_resource(render_world->world_matrix_struct_buffer);
	render_pipeline->set_vertex_shader_resource(render_world->vertex_struct_buffer);
	render_pipeline->set_vertex_shader_resource(render_world->index_struct_buffer);

	render_pipeline->set_pixel_shader_resource(render_world->default_texture.shader_resource);
	render_pipeline->set_pixel_shader_resource(render_world->light_struct_buffer);

	Render_Entity *render_entity = NULL;
	Forwar_Light_Pass::Pass_Data pass_data;
	
	For(render_world->render_entities, render_entity) {
		
		pass_data.mesh_idx = render_entity->mesh_idx;
		pass_data.world_matrix_idx = render_entity->world_matrix_idx;

		render_pipeline->update_constant_buffer(&pass_data_cbuffer, (void *)&pass_data);
		render_pipeline->set_vertex_shader_resource(2, pass_data_cbuffer);

		render_pipeline->draw(render_world->mesh_instances[render_entity->mesh_idx].index_count);
	}
}

