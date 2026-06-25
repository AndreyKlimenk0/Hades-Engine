#ifndef RENDER_WORLD_H
#define RENDER_WORLD_H

#include "mesh.h"
#include "gpu_data.h"
#include "gpu_storages.h"
#include "render_passes.h"
#include "render_system.h"

#include "render_api/render.h"

#include "../game/world.h"
#include "../collision/collision.h"

#include "../libs/str.h"
#include "../libs/color.h"
#include "../libs/number_types.h"
#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"
#include "../libs/math/structures.h"
#include "../libs/structures/array.h"
#include "../libs/structures/hash_table.h"

struct Engine;
struct Render_Pass;
typedef u32 Mesh_Idx;
typedef u32 Render_Entity_Idx;

const u32 CASCADE_COUNT = 3;
const u32 SHADOW_ATLAS_SIZE = 8192;
const u32 CASCADE_SIZE = 1024;

struct Mesh_Instance {
	u32 vertex_offset;
	u32 index_offset;
	u32 material_idx;
	u32 bounding_box_idx;
	u32 transform_idx;
};

struct Render_Entity {
	Entity_Id entity_id;
	u32 mesh_instance;
	Mesh_Storage_Info mesh_info;
};

Render_Entity *find_render_entity(Array<Render_Entity> *render_entities, Entity_Id entity_id, u32 *index = NULL);

struct Shadow_Cascade_Range {
	u32 start = 0;
	u32 end = 0;
	u32 previous_range_length = 0;

	u32 get_length();
	Vector3 get_center_point();
};

struct Shadow_Cascade_Ranges {
	Array<Shadow_Cascade_Range> ranges;

	void add_range(u32 start, u32 end);
};

struct Cascaded_Shadow_Map {
	float cascade_width;
	float cascade_height;
	float cascade_depth;
	u32 view_projection_matrix_index;
	Vector3 view_position;
	Viewport viewport;
	Matrix4 view_projection_matrix;

	void init(float fov, float aspect_ratio, Shadow_Cascade_Range *shadow_cascade_range);
};

struct Cascaded_Shadows {
	Vector3 light_direction;
	Array<Cascaded_Shadow_Map> cascaded_shadow_maps;
};

struct Shadows_Atlas {
	s32 x = 0;
	s32 y = 0;
	void reset();
	bool get_viewport(Viewport *viewport);
};

struct Render_World {
	Render_World();
	~Render_World();

	Game_World *game_world = NULL;
	Render_System *render_sys = NULL;
	Render_Device *render_device = NULL;

	u32 jittering_tile_size = 0;
	u32 jittering_filter_size = 0;
	u32 jittering_scaling = 0;

	Texture *jittering_samples = NULL;

	Shadows_Atlas shadows_atlas;

	Entity_Id camera_id;

	Bounding_Sphere world_bounding_sphere;

	Array<Matrix4> render_entity_world_matrices;
	Array<Matrix4> cascaded_view_projection_matrices;

	Array<Render_Entity> game_render_entities;

	Array<Cascaded_Shadows> cascaded_shadows_list;
	Array<GPU_Cascaded_Shadows_Info> cascaded_shadows_info_list;
	Array<Shadow_Cascade_Range> shadow_cascade_ranges;
	Array<GPU_Light> lights;

	Array<AABB> bounding_boxes;
	Array<Mesh_Instance> mesh_instances;

	Buffer *mesh_instance_buffer = NULL;
	Buffer *bounding_box_buffer = NULL;

	Buffer *world_matrices_buffer = NULL;
	Buffer *casded_view_projection_matrices_buffer = NULL;
	Buffer *cascaded_shadows_info_buffer = NULL;
	Buffer *lights_buffer = NULL;

	void init(Engine *engine);
	void release_all_resources();
	void release_render_entities_resources();

	void update();
	void update_shadows();
	void update_render_entities();

	void upload_lights();

	void prepare_for_rendering();

	void add_render_entity(Entity_Id entity_id, AABB bounding_box, u32 material_idx, Mesh_Storage_Info *mesh_info);
	u32 delete_render_entity(Entity_Id entity_id);

	void set_rendering_view(Entity_Id new_camera_id);

	Vector3 get_light_position(Vector3 light_direction);

	Camera *get_camera();
};
#endif

