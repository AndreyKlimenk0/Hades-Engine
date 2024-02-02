#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "render_world.h"
#include "../gui/gui.h"
#include "../sys/engine.h"
#include "../collision/collision.h"
#include "../libs/mesh_loader.h"
#include "../libs/math/functions.h"
#include "../libs/math/structures.h"

const Color DEFAULT_MESH_COLOR = Color(105, 105, 105);

static Matrix4 get_world_matrix(Entity *entity) 
{
	if (entity->type == ENTITY_TYPE_CAMERA) {
		Camera *camera = static_cast<Camera *>(entity);
		return inverse(&make_look_at_matrix(camera->position, camera->target));
	}
	return make_scale_matrix(&entity->scaling) * rotate(&entity->rotation) * make_translation_matrix(&entity->position);
}

template <typename T>
static bool copy_array(Array<T> *dst, Array<T> *src, u32 dst_index_offset = 0)
{
	if ((!dst->is_empty() && !src->is_empty()) && (dst->count >= (dst_index_offset + src->count))) {
		memcpy((void *)&dst->items[dst_index_offset], (void *)src->items, sizeof(T) * src->count);
		return true;
	}
	return false;
}

static float generate_random_offset()
{
	return ((float)rand() / ((float)RAND_MAX)) - 0.5f; 	//Generate random offset between -0.5 and 0.5.
}

static void make_jittering_sampling_filters(u32 tile_size, u32 filter_size, Array<Vector2> &jittered_samples)
{
	assert(tile_size > 0);
	assert(filter_size > 0);

	jittered_samples.reserve(math::pow2(tile_size) * math::pow2(filter_size));

	u32 index = 0;
	u32 local_filter_size = filter_size - 1;
	for (u32 tile_row = 0; tile_row < tile_size; tile_row++) {
		for (u32 tile_col = 0; tile_col < tile_size; tile_col++) {
			for (u32 filter_row = 0; filter_row < filter_size; filter_row++) {
				for (u32 filter_col = 0; filter_col < filter_size; filter_col++) {

					float v = ((float)(local_filter_size - filter_row) + 0.5f + generate_random_offset()) / (float)filter_size;
					float u = ((float)(local_filter_size - filter_col) + 0.5f + generate_random_offset()) / (float)filter_size;

					jittered_samples[index].x = math::sqrt(v) * math::cos(2 * PI * u);
					jittered_samples[index].y = math::sqrt(v) * math::sin(2 * PI * u);
					index++;
				}
			}
		}
	}
}

Render_Entity *find_render_entity(Array<Render_Entity> *render_entities, Entity_Id entity_id, u32 *index)
{
	for (u32 i = 0; i < render_entities->count; i++) {
		if (render_entities->get(i).entity_id == entity_id) {
			if (index) {
				*index = i;
			}
			return &render_entities->get(i);
		}
	}
	return NULL;
}

void Render_Camera::update(Camera *camera, Camera *camera_info)
{
	view_matrix = make_look_at_matrix(camera->position, camera->target);
	debug_view_matrix = make_look_at_matrix(camera_info->position, camera_info->target);
}

bool Render_Camera::is_entity_camera_set()
{
	if (camera_id.type == ENTITY_TYPE_CAMERA) {
		return true;
	}
	return false;
}

u32 Shadow_Cascade_Range::get_length()
{
	return end - start;
}

Vector3 Shadow_Cascade_Range::get_center_point()
{
	return Vector3(0.0f, 0.0f, (float)start + (get_length() * 0.5f));
}

void Shadow_Cascade_Ranges::add_range(u32 start, u32 end)
{
	if (start >= end) {
		print("Shadow_Cascade_Ranges::add_range: Failed to add range");
		return;
	}
	Shadow_Cascade_Range range;
	range.start = start;
	range.end = end;
	if (!ranges.is_empty()) {
		range.previous_range_length = ranges.get_last().get_length();
	}
	ranges.push({ start, end });
}


template<typename T>
void Unified_Mesh_Storate<T>::allocate_gpu_memory()
{
	mesh_struct_buffer.allocate<Mesh_Instance>(1000);
	index_struct_buffer.allocate<u32>(100000);
	vertex_struct_buffer.allocate<Vertex_PNTUV>(100000);
}

template<typename T>
bool Unified_Mesh_Storate<T>::add_mesh(const char *mesh_name, Mesh<T> *mesh, Mesh_Idx *_mesh_idx)
{
	assert(mesh_name);

	if ((mesh->vertices.count == 0) || (mesh->indices.count == 0)) {
		print("Render_World::add_mesh: Mesh {} can be added because doesn't have all necessary data.", mesh_name);
		return false;
	}

	String_Id string_id = fast_hash(mesh_name);

	Mesh_Idx mesh_idx;
	if (mesh_table.get(string_id, &mesh_idx)) {
		*_mesh_idx = mesh_idx;
		return true;
	}

	Mesh_Instance mesh_info;
	mesh_info.vertex_count = mesh->vertices.count;
	mesh_info.index_count = mesh->indices.count;
	mesh_info.vertex_offset = unified_vertices.count;
	mesh_info.index_offset = unified_indices.count;

	mesh_idx = mesh_instances.push(mesh_info);
	merge(&unified_vertices, &mesh->vertices);
	merge(&unified_indices, &mesh->indices);

	mesh_struct_buffer.update(&mesh_instances);
	vertex_struct_buffer.update(&unified_vertices);
	index_struct_buffer.update(&unified_indices);

	mesh_table.set(string_id, mesh_idx);
	
	*_mesh_idx = mesh_idx;
	return true;
}

template<typename T>
bool Unified_Mesh_Storate<T>::update_mesh(Mesh_Idx mesh_idx, Mesh<T>* mesh)
{
	Mesh_Instance mesh_instance = mesh_instances[mesh_idx];
	if ((mesh->vertices.count == mesh_instance.vertex_count) && (mesh->indices.count == mesh_instance.index_count)) {
		u32 vertex_offset = mesh_instance.vertex_offset > 0 ? mesh_instance.vertex_offset: 0;
		u32 index_offset = mesh_instance.index_offset > 0 ? mesh_instance.index_offset: 0;
		copy_array(&unified_vertices, &mesh->vertices, vertex_offset);
		copy_array(&unified_indices, &mesh->indices, index_offset);
		
		vertex_struct_buffer.update(&unified_vertices);
		index_struct_buffer.update(&unified_indices);
		return true;
	}
	return false;
}

void Cascaded_Shadow_Map::init(float fov, float aspect_ratio, Shadow_Cascade_Range *shadow_cascade_range)
{
	float half_height = (float)shadow_cascade_range->end * math::tan(fov * 0.5f);
	float half_width = half_height * aspect_ratio;
	float width = half_width * 2.0f;
	float height = half_height * 2.0f;
	float max_value = math::max(width, math::max(height, (float)shadow_cascade_range->get_length()));
	cascade_width = max_value;
	cascade_height = max_value;
	cascade_depth = max_value;
	view_position = shadow_cascade_range->get_center_point();
	view_projection_matrix = make_identity_matrix();
}

void Render_World::init(Engine *engine)
{	
	//@Note: Why don't pass just the engine pointer ?
	game_world = &engine->game_world;
	render_sys = &engine->render_sys;

	init_shadow_rendering();

	init_render_passes(&engine->shader_manager);

	render_entity_texture_storage.init(&render_sys->gpu_device, &render_sys->render_pipeline);
	
	render_sys->gpu_device.create_constant_buffer(sizeof(CB_Frame_Info), &frame_info_cbuffer);

	lights_struct_buffer.allocate<Hlsl_Light>(100);
	world_matrices_struct_buffer.allocate<Matrix4>(100);
	
	Texture2D_Desc texture_desc;
	texture_desc.width = 200;
	texture_desc.height = 200;
	texture_desc.mip_levels = 1;
	
	render_sys->gpu_device.create_texture_2d(&texture_desc, &default_texture);
	render_sys->gpu_device.create_shader_resource_view(&texture_desc, &default_texture);
	fill_texture((void *)&DEFAULT_MESH_COLOR, &default_texture);

	if (!render_camera.is_entity_camera_set()) {
		error("Render Camera was not initialized. There is no a view for rendering.");
	}
}

void Render_World::init_meshes()
{
	if (!render_camera.is_entity_camera_set()) {
		error("Render Camera was not initialized. There is no a view for rendering.");
	}

	u32 entity_index = 0;
	Array<Import_Mesh> meshes;
	String path;
	//build_full_path_to_model_file("mutant.fbx", path);
	//build_full_path_to_model_file("box.fbx", path);
	//build_full_path_to_model_file("walls.fbx", path);
	//build_full_path_to_model_file("scene_demo_unreal.fbx", path);
	//build_full_path_to_model_file("camera.fbx", path);
	//load_fbx_mesh(path, &meshes);
	print("Start make render entities");
	Import_Mesh *imported_mesh = NULL;
	print("Mesh count = ", meshes.count);


	Render_Entity_Textures render_entity_textures;
	render_entity_textures.ambient_texture_idx = render_entity_texture_storage.white_texture_idx;
	render_entity_textures.normal_texture_idx = render_entity_texture_storage.green_texture_idx;
	render_entity_textures.diffuse_texture_idx = render_entity_texture_storage.default_texture_idx;
	render_entity_textures.specular_texture_idx = render_entity_texture_storage.white_texture_idx;
	render_entity_textures.displacement_texture_idx = render_entity_texture_storage.white_texture_idx;

	For(meshes, imported_mesh) {
		Mesh_Idx mesh_idx;
		if (add_mesh(imported_mesh->mesh.name, &imported_mesh->mesh, &mesh_idx)) {
			if (imported_mesh->mesh_instances.count > 0) {
				for (u32 j = 0; j < imported_mesh->mesh_instances.count; j++) {
					Import_Mesh::Transform_Info t = imported_mesh->mesh_instances[j];
					Entity_Id entity_id = game_world->make_entity(t.scaling, t.rotation, t.translation);
					AABB aabb = make_AABB(&imported_mesh->mesh);
					game_world->attach_AABB(entity_id, &aabb);
					add_render_entity(RENDERING_TYPE_FORWARD_RENDERING, entity_id, mesh_idx, &render_entity_textures);
				}
			} else {
				AABB aabb = make_AABB(&imported_mesh->mesh);
				Entity_Id entity_id = game_world->make_entity(Vector3::one, Vector3::zero, Vector3::zero);
				game_world->attach_AABB(entity_id, &aabb);
				add_render_entity(RENDERING_TYPE_FORWARD_RENDERING, entity_id, mesh_idx, &render_entity_textures);
			}
		}
	}
}

void Render_World::init_shadow_rendering()
{	
	Texture2D_Desc depth_stencil_desc;
	depth_stencil_desc.width = SHADOW_ATLAS_SIZE;
	depth_stencil_desc.height = SHADOW_ATLAS_SIZE;
	depth_stencil_desc.format = DXGI_FORMAT_R24G8_TYPELESS;
	depth_stencil_desc.mip_levels = 1;
	depth_stencil_desc.bind |= BIND_DEPTH_STENCIL;

	render_sys->gpu_device.create_texture_2d(&depth_stencil_desc, &shadow_atlas);
	render_sys->gpu_device.create_depth_stencil_view(&depth_stencil_desc, &shadow_atlas);
	render_sys->gpu_device.create_shader_resource_view(&depth_stencil_desc, &shadow_atlas);

	fill_texture((void *)&DEFAULT_DEPTH_VALUE, &shadow_atlas);

	shadow_cascade_ranges.push({ 1, 15 });
	shadow_cascade_ranges.push({ 15, 150 });
	shadow_cascade_ranges.push({ 150, 500 });
	shadow_cascade_ranges.push({ 500, 1000 });

	jittering_tile_size = 16;
	jittering_filter_size = 8;
	jittering_scaling = jittering_filter_size / 2;

	Array<Vector2> jittered_samples;
	make_jittering_sampling_filters(jittering_tile_size, jittering_filter_size, jittered_samples);

	Texture3D_Desc jittering_samples_texture_desc;
	jittering_samples_texture_desc.width = math::pow2(jittering_filter_size);
	jittering_samples_texture_desc.height = jittering_tile_size;
	jittering_samples_texture_desc.depth = jittering_tile_size;
	jittering_samples_texture_desc.format = DXGI_FORMAT_R32G32_FLOAT;
	jittering_samples_texture_desc.bind = BIND_SHADER_RESOURCE;
	jittering_samples_texture_desc.data = (void *)jittered_samples.items;
	jittering_samples_texture_desc.mip_levels = 1;

	render_sys->gpu_device.create_texture_3d(&jittering_samples_texture_desc, &jittering_samples);
	render_sys->gpu_device.create_shader_resource_view(&jittering_samples_texture_desc, &jittering_samples);
}

void Render_World::init_render_passes(Shader_Manager *shader_manager)
{
	assert(shader_manager);

	print("Render_World::init: Initializing render passes.");

	Array<Render_Pass *> render_pass_list;
	render_passes.get_all_passes(&render_pass_list);

	for (u32 i = 0; i < render_pass_list.count; i++) {
		render_pass_list[i]->init(&render_sys->gpu_device, &render_sys->render_pipeline_states);
	}

	Viewport viewport;
	viewport.width = Render_System::screen_width;
	viewport.height = Render_System::screen_height;

	render_passes.shadows.setup_render_pipeline(shader_manager, shadow_atlas.dsv);
	render_passes.draw_lines.setup_render_pipeline(shader_manager, render_sys->multisampling_depth_stencil_texture.dsv, render_sys->multisampling_back_buffer_texture.rtv, &viewport);
	render_passes.draw_vertices.setup_render_pipeline(shader_manager, render_sys->multisampling_depth_stencil_texture.dsv, render_sys->multisampling_back_buffer_texture.rtv, &viewport);
	render_passes.forward_light.setup_render_pipeline(shader_manager, render_sys->multisampling_depth_stencil_texture.dsv, render_sys->multisampling_back_buffer_texture.rtv, &viewport);
	render_passes.debug_cascade_shadows.setup_render_pipeline(shader_manager, render_sys->multisampling_depth_stencil_texture.dsv, render_sys->multisampling_back_buffer_texture.rtv, &viewport);
	
	render_passes.outlining.setup_outlining(2, Color(245, 176, 66));
	render_passes.outlining.setup_render_pipeline(shader_manager, &render_sys->silhouette_buffer, &render_sys->silhouette_depth_stencil_buffer, &render_sys->back_buffer_texture, &viewport);

	Array<Render_Pass *> temp;
	temp.push(&render_passes.shadows);
	temp.push(&render_passes.draw_lines);
	temp.push(&render_passes.draw_vertices);
	temp.push(&render_passes.forward_light);

	for (u32 i = 0; i < render_pass_list.count; i++) {
		if (render_pass_list[i]->is_valid) {
			print("  Render pass '{}' was successfully initialized.", &render_pass_list[i]->name);
		} else {
			print("  Render pass '{}' is not valid. The render pass will not be used for rendering.", &render_pass_list[i]->name);
		}
	}
	for (u32 i = 0; i < temp.count; i++) {
		if (temp[i]->is_valid) {
			every_frame_render_passes.push(temp[i]);
		}
	}
}

void Render_World::update()
{		
	update_render_entities();

	if (light_hash != game_world->light_hash) {
		light_hash = game_world->light_hash;
		update_lights();
	}

	Camera *camera = game_world->get_camera(render_camera.camera_id);
	Camera *camera_info = game_world->get_camera(render_camera.camera_info_id);
	render_camera.update(camera, camera_info);

	frame_info.view_matrix = render_camera.view_matrix;
	frame_info.perspective_matrix = render_sys->view.perspective_matrix;
	frame_info.orthographic_matrix = render_sys->view.orthogonal_matrix;
	frame_info.camera_position = camera_info->position;
	frame_info.camera_direction = camera_info->target;
	frame_info.near_plane = render_sys->view.near_plane;
	frame_info.far_plane = render_sys->view.far_plane;

	update_shadows();
}

void Render_World::update_render_entities()
{
	Render_Entity *render_entity = NULL;
	For (game_render_entities, render_entity) {
		Entity *entity = game_world->get_entity(render_entity->entity_id);
		render_entity_world_matrices[render_entity->world_matrix_idx] = get_world_matrix(entity);
	}

	For (line_render_entities, render_entity) {
		Entity *entity = game_world->get_entity(render_entity->entity_id);
		render_entity_world_matrices[render_entity->world_matrix_idx] = get_world_matrix(entity);
	}

	For (vertex_render_entities, render_entity) {
		Entity *entity = game_world->get_entity(render_entity->entity_id);
		render_entity_world_matrices[render_entity->world_matrix_idx] = get_world_matrix(entity);
	}
	world_matrices_struct_buffer.update(&render_entity_world_matrices);
}

void Render_World::update_lights()
{
	frame_info.light_count = game_world->lights.count;

	Array<Hlsl_Light> shader_lights;

	Light *light = NULL;
	For(game_world->lights, light) {
		Hlsl_Light hlsl_light;
		hlsl_light.position = light->position;
		hlsl_light.direction = normalize(&light->direction);
		hlsl_light.color = light->color;
		hlsl_light.radius = light->radius;
		hlsl_light.range = light->range;
		hlsl_light.light_type = light->light_type;
		add_shadow(light);
		shader_lights.push(hlsl_light);
	}
	lights_struct_buffer.update(&shader_lights);
}

void Render_World::add_render_entity(Rendering_Type rendering_type, Entity_Id entity_id, Mesh_Idx mesh_idx, Render_Entity_Textures *render_entity_textures, void *args)
{
	assert(render_entity_textures);

	Render_Entity render_entity;
	render_entity.entity_id = entity_id;
	render_entity.mesh_idx = mesh_idx;
	render_entity.world_matrix_idx = render_entity_world_matrices.push(Matrix4());
	render_entity.ambient_texture_idx = render_entity_textures->ambient_texture_idx;
	render_entity.normal_texture_idx = render_entity_textures->normal_texture_idx;
	render_entity.diffuse_texture_idx = render_entity_textures->diffuse_texture_idx;
	render_entity.specular_texture_idx = render_entity_textures->specular_texture_idx;
	render_entity.displacement_texture_idx = render_entity_textures->displacement_texture_idx;

	switch (rendering_type) {
		case RENDERING_TYPE_FORWARD_RENDERING: {
			game_render_entities.push(render_entity);
			break;
		}
		case RENDERING_TYPE_LINES_RENDERING: {
			assert(args);
			line_render_entities.push(render_entity);
			line_render_entity_colors.push(*((Color *)args));
			break;
		}
		case RENDERING_TYPE_VERTICES_RENDERING: {
			assert(args);
			vertex_render_entities.push(render_entity);
			vertex_render_entity_colors.push(*((Color *)args));
			break;
		}
		default: {
			print("Render_World::add_render_entity: Unable to add a render entity, unknown rendering type was passed.");
			break;
		}
	}
}

u32 Render_World::delete_render_entity(Entity_Id entity_id)
{
	u32 render_entity_index;
	find_render_entity(&game_render_entities, entity_id, &render_entity_index);
	game_render_entities.remove(render_entity_index);

	for (u32 i = 0; i < game_render_entities.count; i++) {
		Render_Entity *render_entity = &game_render_entities[i];
		if (render_entity->entity_id.index > entity_id.index) {
			render_entity->entity_id.index -= 1;
		}
	}
	return render_entity_index;
}

bool Render_World::add_shadow(Light *light)
{
	Cascaded_Shadows_Info cascaded_shadows_info;
	cascaded_shadows_info.light_direction = light->direction;
	cascaded_shadows_info.shadow_map_start_index = cascaded_shadows_info_list.count * shadow_cascade_ranges.count;
	cascaded_shadows_info.shadow_map_end_index = cascaded_shadows_info_list.count * shadow_cascade_ranges.count + (shadow_cascade_ranges.count - 1);
	cascaded_shadows_info_list.push(cascaded_shadows_info);
	cascaded_shadows_info_sb.update(&cascaded_shadows_info_list);
	
	Cascaded_Shadows cascaded_shadows;
	cascaded_shadows.light_direction = light->direction;
	
	for (u32 i = 0; i < shadow_cascade_ranges.count; i++) {
		Cascaded_Shadow_Map cascaded_shadow_map;
		cascaded_shadow_map.view_projection_matrix_index = cascaded_view_projection_matrices.push(Matrix4());
		cascaded_shadow_map.init(render_sys->view.fov, render_sys->view.ratio, &shadow_cascade_ranges[i]);
		
		if (!get_shadow_atls_viewport(&cascaded_shadow_map.viewport)) {
			assert(false);
			return false;
		}
		cascaded_shadows.cascaded_shadow_maps.push(cascaded_shadow_map);
		cascaded_shadow_map_count++;
	}
	cascaded_shadows_list.push(cascaded_shadows);
	return true;
}

void Render_World::update_shadows()
{
	for (u32 i = 0; i < cascaded_shadows_list.count; i++) {
		Vector3 light_direction = cascaded_shadows_list[i].light_direction;

		for (u32 j = 0; j < cascaded_shadows_list[i].cascaded_shadow_maps.count; j++) {
			Cascaded_Shadow_Map *cascaded_shadow_map = &cascaded_shadows_list[i].cascaded_shadow_maps[j];
			
			Vector3 view_position = cascaded_shadow_map->view_position * inverse(&render_camera.debug_view_matrix);
			Vector3 temp_view_position = view_position;

			//float w = cascaded_shadow->cascade_width / CASCADE_WIDTH;
			//float h = cascaded_shadow->cascade_width / CASCADE_WIDTH;
			//float d = cascaded_shadow->cascade_width / CASCADE_WIDTH;

			//temp_view_position.x /= w;
			//temp_view_position.x = std::floor(temp_view_position.x);
			//temp_view_position.x *= w;

			//temp_view_position.y /= h;
			//temp_view_position.y = std::floor(temp_view_position.y);
			//temp_view_position.y *= h;

			//temp_view_position.z /= d;
			//temp_view_position.z = std::floor(temp_view_position.z);
			//temp_view_position.z *= d;

			//Vector3 old_view_position = view_position;

			float radius = cascaded_shadow_map->cascade_width / 2.0f;
			//float texel_per_unit = CASCADE_WIDTH / (radius * 2.0f);

			//Matrix4 scalar = make_scale_matrix(texel_per_unit);
			//Matrix4 look_at = make_look_at_matrix(Vector3::zero, negate(&cascaded_shadow->light_direction)) * scalar;
			//Matrix4 inverse_look_at = inverse(&look_at);

			//view_position = view_position * look_at;
			//view_position.x = std::floor(view_position.x);
			//view_position.y = std::floor(view_position.y);
			//view_position.z = std::floor(view_position.z);
			//view_position = view_position * inverse_look_at;
			

			Vector3 view_direction = view_position + light_direction;
			Matrix4 light_view_matrix = make_look_at_matrix(view_position, view_direction);

			//auto r = cascaded_shadow->cascade_width / CASCADE_WIDTH;
			//radius /= r;
			//radius = std::floor(radius);
			//radius *= r;

			Matrix4 projection_matrix = XMMatrixOrthographicOffCenterLH(-radius, radius, -radius, radius,  -5000.0f, 5000.0f);
			
			cascaded_shadow_map->view_projection_matrix = light_view_matrix * projection_matrix;

			XMMATRIX shadowMatrix = XMLoadFloat4x4(&cascaded_shadow_map->view_projection_matrix);
			XMVECTOR shadowOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			shadowOrigin = XMVector4Transform(shadowOrigin, shadowMatrix);
			shadowOrigin = XMVectorScale(shadowOrigin, (float)CASCADE_SIZE / 2.0f);

			XMVECTOR roundedOrigin = XMVectorRound(shadowOrigin);
			XMVECTOR roundOffset = XMVectorSubtract(roundedOrigin, shadowOrigin);
			roundOffset = XMVectorScale(roundOffset, 2.0f / (float)CASCADE_SIZE);
			roundOffset = XMVectorSetZ(roundOffset, 0.0f);
			roundOffset = XMVectorSetW(roundOffset, 0.0f);

			Matrix4 matrix = cascaded_shadow_map->view_projection_matrix;
			Vector4 vector = roundOffset;
			vector.x += matrix.m[3][0];
			vector.y += matrix.m[3][1];
			vector.z += matrix.m[3][2];
			vector.w += matrix.m[3][3];
			matrix.set_row_3(vector);
			cascaded_shadow_map->view_projection_matrix = matrix;
			cascaded_view_projection_matrices[cascaded_shadow_map->view_projection_matrix_index] = matrix;
		}
	}
	world_matrices_struct_buffer.update(&render_entity_world_matrices);
	cascaded_view_projection_matrices_sb.update(&cascaded_view_projection_matrices);
}

bool Render_World::add_mesh(const char *mesh_name, Mesh<Vertex_PNTUV> *mesh, Mesh_Idx *mesh_idx)
{
	Bounding_Sphere temp = make_bounding_sphere(Vector3(0.0f, 0.0f, 0.0f), mesh);
	if (temp.radious > world_bounding_sphere.radious) {
		world_bounding_sphere = temp;
	}
	return triangle_meshes.add_mesh(mesh_name, mesh, mesh_idx);
}

bool Render_World::add_mesh(const char *mesh_name, Mesh<Vector3> *mesh, Mesh_Idx *mesh_idx)
{
	return line_meshes.add_mesh(mesh_name, mesh, mesh_idx);
}

void Render_World::set_camera_for_rendering(Entity_Id camera_id)
{
	if (camera_id.type != ENTITY_TYPE_CAMERA) {
		print("Render_World::set_camera_for_rendering: Passed an camera id is not entity camera type.");
		return;
	}
	render_camera.camera_id = camera_id;
	render_camera.camera_info_id = camera_id;
}

void Render_World::set_camera_for_debuging(Entity_Id camera_info_id)
{
	if (camera_info_id.type != ENTITY_TYPE_CAMERA) {
		print("Render_World::set_camera_for_debuging: Passed an camera id is not entity camera type.");
		return;
	}
	render_camera.camera_info_id = camera_info_id;
}

bool Render_World::get_shadow_atls_viewport(Viewport *viewport)
{
	static u32 x = 0;
	static u32 y = 0;
	
	Point_u32 shadow_map_position;
	shadow_map_position.x = CASCADE_SIZE + x;
	shadow_map_position.y = CASCADE_SIZE + y;

	if ((shadow_map_position.x <= SHADOW_ATLAS_SIZE) && (shadow_map_position.y <= SHADOW_ATLAS_SIZE)) {
		viewport->x = x;
		viewport->y = y;
		viewport->width = CASCADE_SIZE;
		viewport->height = CASCADE_SIZE;
		x += CASCADE_SIZE;
	} else if ((shadow_map_position.x > SHADOW_ATLAS_SIZE) && (shadow_map_position.y < SHADOW_ATLAS_SIZE)) {
		viewport->x = 0;
		viewport->y = y;
		viewport->width = CASCADE_SIZE;
		viewport->height = CASCADE_SIZE;
		x = 0;
		y += CASCADE_SIZE;
	} else {
		print("Render_World::get_shadow_atls_view_port: The shadow atlas is out of space.");
		return false;
	}
	return true;
}

void Render_World::render()
{
	render_sys->render_pipeline.update_constant_buffer(&frame_info_cbuffer, (void *)&frame_info);

	render_sys->render_pipeline.set_vertex_shader_resource(CB_FRAME_INFO_REGISTER, frame_info_cbuffer);
	render_sys->render_pipeline.set_pixel_shader_resource(CB_FRAME_INFO_REGISTER, frame_info_cbuffer);
	
	Render_Pass *render_pass = NULL;
	For(every_frame_render_passes, render_pass) {
		render_pass->render(this, &render_sys->render_pipeline);
	}
}

void Render_Entity_Texture_Storage::init(Gpu_Device *_gpu_device, Render_Pipeline *_render_pipeline)
{
	assert(_gpu_device);
	
	gpu_device = _gpu_device;
	render_pipeline = _render_pipeline;

	Texture2D_Desc texture_desc;
	texture_desc.width = 200;
	texture_desc.height = 200;
	texture_desc.mip_levels = 1;

	Texture2D default_texture;
	gpu_device->create_texture_2d(&texture_desc, &default_texture);
	gpu_device->create_shader_resource_view(&texture_desc, &default_texture);
	fill_texture((void *)&DEFAULT_MESH_COLOR, &default_texture);
	
	default_texture_idx = textures.push(default_texture);

	Texture2D white_texture;
	gpu_device->create_texture_2d(&texture_desc, &white_texture);
	gpu_device->create_shader_resource_view(&texture_desc, &white_texture);
	fill_texture((void *)&Color::White, &white_texture);

	white_texture_idx = textures.push(white_texture);

	Texture2D black_texture;
	gpu_device->create_texture_2d(&texture_desc, &black_texture);
	gpu_device->create_shader_resource_view(&texture_desc, &black_texture);
	fill_texture((void *)&Color::Black, &black_texture);

	black_texture_idx = textures.push(black_texture);

	Color temp = { 0.5f, 0.5f, 1.0f };
	Texture2D green_texture;
	gpu_device->create_texture_2d(&texture_desc, &green_texture);
	gpu_device->create_shader_resource_view(&texture_desc, &green_texture);
	fill_texture((void *)&temp, &green_texture);

	green_texture_idx = textures.push(green_texture);

	temp = { 0.2f, 0.2f, 0.2f };
	Texture2D default_specular_texture;
	gpu_device->create_texture_2d(&texture_desc, &default_specular_texture);
	gpu_device->create_shader_resource_view(&texture_desc, &default_specular_texture);
	fill_texture((void *)&temp, &default_specular_texture);

	default_specular_texture_idx = textures.push(default_specular_texture);
}

Texture_Idx Render_Entity_Texture_Storage::add_texture(const char *name, u32 width, u32 height, void *data)
{
	assert(name);
	assert(data);

	Texture_Idx texture_idx;
	if (texture_table.get(name, &texture_idx)) {
		return texture_idx;
	}

	Texture2D_Desc texture_desc;
	texture_desc.width = width;
	texture_desc.height = height;
	texture_desc.data = data;
	texture_desc.mip_levels = 0;

	Texture2D texture;
	gpu_device->create_texture_2d(&texture_desc, &texture);
	gpu_device->create_shader_resource_view(&texture_desc, &texture);
	render_pipeline->update_subresource(&texture, data, width * 4);
	render_pipeline->generate_mips(texture.srv);

	Texture2D_Desc desc;
	texture.get_desc(&desc);

	texture_idx = textures.push(texture);
	texture_table.set(name, texture_idx);

	return texture_idx;
}

void Render_World::Render_Passes::get_all_passes(Array<Render_Pass *> *render_passes_list)
{
	render_passes_list->push(&shadows);
	render_passes_list->push(&draw_lines);
	render_passes_list->push(&draw_vertices);
	render_passes_list->push(&forward_light);
	render_passes_list->push(&debug_cascade_shadows);
	render_passes_list->push(&outlining);
}
