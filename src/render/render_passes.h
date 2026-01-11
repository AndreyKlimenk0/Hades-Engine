#ifndef RENDER_PASS_H
#define RENDER_PASS_H

//#include "hlsl.h"
//#include "../libs/str.h"
//#include "../libs/color.h"
//#include "../libs/number_types.h"
//#include "../libs/math/vector.h"
//#include "../libs/math/matrix.h"
//#include "../libs/math/structures.h"

struct Shader_Manager;
struct Pipeline_Resource_Manager;

#include "render_api/render.h"

//For generating mipmaps
#include "../libs/structures/array.h"

struct Render_Pass {
	Render_Pass();
	virtual ~Render_Pass();

	u32 access = 0;
	String name;
	Root_Signature *root_signature = NULL;
	Pipeline_State *pipeline_state = NULL;

	virtual void init(const char *pass_name, Render_Device *device, Shader_Manager *shader_manager, Pipeline_Resource_Manager *resource_manager);
	virtual void setup_root_signature(Render_Device *device);
	virtual void schedule_resources(Pipeline_Resource_Manager *resource_manager);
	virtual void setup_pipeline(Render_Device *device, Shader_Manager *shader_manager) = 0;
	virtual void render(Graphics_Command_List *graphics_command_list, void *context, void *args = NULL) = 0;
};

struct Shadows_Pass : Render_Pass {
	Texture *shadow_atlas = NULL;

	void init(Render_Device *device, Shader_Manager *shader_manager, Pipeline_Resource_Manager *resource_manager);
	void schedule_resources(Pipeline_Resource_Manager *resource_manager);
	void setup_root_signature(Render_Device *device);
	void setup_pipeline(Render_Device *device, Shader_Manager *shader_manager);
	void render(Graphics_Command_List *graphics_command_list, void *context, void *args = NULL);
};

struct Forward_Pass : Render_Pass {
	Texture *shadow_atlas = NULL;
	
	void init(Render_Device *device, Shader_Manager *shader_manager, Pipeline_Resource_Manager *resource_manager);
	void schedule_resources(Pipeline_Resource_Manager *resource_manager);
	void setup_root_signature(Render_Device *device);
	void setup_pipeline(Render_Device *device, Shader_Manager *shader_manager);
	void render(Graphics_Command_List *graphics_command_list, void *context, void *args = NULL);
};

struct Render_2D_Pass : Render_Pass {
	void init(Render_Device *device, Shader_Manager *shader_manager, Pipeline_Resource_Manager *resource_manager);
	void schedule_resources(Pipeline_Resource_Manager *resource_manager);
	void setup_root_signature(Render_Device *device);
	void setup_pipeline(Render_Device *render_device, Shader_Manager *shader_manager);
	void render(Graphics_Command_List *graphics_command_list, void *context, void *args = NULL);
};

struct Silhouette_Pass : Render_Pass {
	Texture *silhouette = NULL;
	Texture *silhouette_depth = NULL;
	
	Array<u32> render_entity_indices;
	void add_render_entity_index(u32 entity_index);
	void delete_render_entity_index(u32 entity_index);
	void reset_render_entity_indices();

	void init(Render_Device *device, Shader_Manager *shader_manager, Pipeline_Resource_Manager *resource_manager);
	void schedule_resources(Pipeline_Resource_Manager *resource_manager);
	void setup_root_signature(Render_Device *device);
	void setup_pipeline(Render_Device *render_device, Shader_Manager *shader_manager);
	void render(Graphics_Command_List *graphics_command_list, void *context, void *args = NULL);
};

struct Outlining_Pass : Render_Pass {
	struct Pass_Data {
		s32 offset_range = 0;
		Pad3 pad;
		Vector4 color;
	};
	Texture *outlining = NULL;
	Texture *silhouette = NULL;
	Texture *silhouette_depth = NULL;
	Pass_Data pass_data;

	void setup_outlining(u32 outlining_size_in_pixels, const Color &color);

	void init(Render_Device *device, Shader_Manager *shader_manager, Pipeline_Resource_Manager *resource_manager);
	void schedule_resources(Pipeline_Resource_Manager *resource_manager);
	void setup_root_signature(Render_Device *device);
	void setup_pipeline(Render_Device *render_device, Shader_Manager *shader_manager);
	void render(Graphics_Command_List *graphics_command_list, void *context, void *args = NULL);
};

struct Back_Buffer_Output : Render_Pass {
	void init(Render_Device *device, Shader_Manager *shader_manager, Pipeline_Resource_Manager *resource_manager);
	void schedule_resources(Pipeline_Resource_Manager *resource_manager);
	void setup_root_signature(Render_Device *device);
	void setup_pipeline(Render_Device *render_device, Shader_Manager *shader_manager);
	void render(Graphics_Command_List *graphics_command_list, void *context, void *args = NULL);
};
#endif
