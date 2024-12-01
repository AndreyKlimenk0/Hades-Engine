#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "../sys/sys.h"
#include "../sys/engine.h"

#include "render_world.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/math/functions.h"

const Color DEFAULT_MESH_COLOR = Color(105, 105, 105);

Matrix4 get_world_matrix(Entity *entity) 
{
	if (entity->type == ENTITY_TYPE_CAMERA) {
		Camera *camera = static_cast<Camera *>(entity);
		Matrix4 view_matrix = make_look_at_matrix(camera->position, camera->target);
		return inverse(&view_matrix);
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
		range.previous_range_length = ranges.last().get_length();
	}
	ranges.push({ start, end });
}

//inline Texture2D create_color_texture(Gpu_Device *gpu_device, u32 texture_width, u32 texture_height, const Color &color)
//{
//	assert(gpu_device);
//	assert(texture_width > 0);
//	assert(texture_height > 0);
//
//	Texture2D_Desc texture_desc;
//	texture_desc.width = texture_width;
//	texture_desc.height = texture_height;
//	texture_desc.mip_levels = 1;
//
//	Texture2D texture;
//	gpu_device->create_texture_2d(&texture_desc, &texture);
//	gpu_device->create_shader_resource_view(&texture_desc, &texture);
//	fill_texture((void *)&color, &texture);
//
//	return texture;
//}

//void Mesh_Storate::init(Gpu_Device *gpu_device)
//{
//	assert(gpu_device);
//
//	u32 width = 200;
//	u32 height = 200;
//
//	default_textures.normal = textures.push(create_color_texture(gpu_device, width, height, Color(0.0f, 0.0f, 0.0f)));
//	default_textures.diffuse = textures.push(create_color_texture(gpu_device, width, height, DEFAULT_MESH_COLOR));
//	default_textures.specular = textures.push(create_color_texture(gpu_device, width, height, Color(0.2f, 0.2f, 0.2f)));
//	default_textures.displacement = textures.push(create_color_texture(gpu_device, width, height, Color(0.0f, 0.0f, 0.0f)));
//	default_textures.white = textures.push(create_color_texture(gpu_device, width, height, Color::White));
//	default_textures.black = textures.push(create_color_texture(gpu_device, width, height, Color::Black));
//	default_textures.green = textures.push(create_color_texture(gpu_device, width, height, Color(0.5f, 0.5f, 1.0f)));
//}

void Mesh_Storate::release_all_resources()
{
	//unified_vertices.clear();
	//unified_indices.clear();
	//textures.clear();
	//mesh_instances.clear();
	//meshes_textures.clear();
	//loaded_meshes.clear();

	//mesh_table.clear();
	//texture_table.clear();

	//vertex_struct_buffer.free();
	//index_struct_buffer.free();
	//mesh_struct_buffer.free();
}

void Model_Storage::add_models_file(const char *file_name)
{
	//mesh_struct_buffer.allocate<Mesh_Instance>(1000);
	//index_struct_buffer.allocate<u32>(100000);
	//vertex_struct_buffer.allocate<Vertex_PNTUV>(100000);
}

void Model_Storage::reserve_memory_for_new_models(u32 mesh_count, u32 total_vertex_count, u32 total_index_count)
{
	//assert(texture_name);
	//assert(texture_idx);

	//Gpu_Device *gpu_device = &Engine::get_render_system()->gpu_device;
	//Render_Pipeline *render_pipeline = &Engine::get_render_system()->render_pipeline;

	//if (strlen(texture_name) == 0) {
	//	return false;
	//}

	//String_Id string_id = fast_hash(texture_name);
	//if (!texture_table.get(string_id, texture_idx)) {
	//	u32 width;
	//	u32 height;
	//	u8 *data = NULL;
	//	String full_path_to_texture;
	//	build_full_path_to_texture_file(texture_name, full_path_to_texture);
	//	bool result = load_png_file(full_path_to_texture, &data, &width, &height);
	//	if (result) {
	//		Texture2D_Desc texture_desc;
	//		texture_desc.width = width;
	//		texture_desc.height = height;
	//		texture_desc.data = data;
	//		texture_desc.mip_levels = 0;

	//		Texture2D texture;
	//		gpu_device->create_texture_2d(&texture_desc, &texture);
	//		gpu_device->create_shader_resource_view(&texture_desc, &texture);
	//		render_pipeline->update_subresource(&texture, data, width * dxgi_format_size(texture_desc.format));
	//		render_pipeline->generate_mips(texture.srv);

	//		*texture_idx = textures.push(texture);
	//		texture_table.set(string_id, *texture_idx);
	//	}
	//	DELETE_PTR(data);
	//	return result;
	//}
	return true;
}

void Model_Storage::add_models(Array<Loading_Model *> &models, Array<Pair<Loading_Model *, Mesh_Id>> &result)
{
	//assert(triangle_mesh);
	//assert(mesh_id);

	//Gpu_Device *gpu_device = &Engine::get_render_system()->gpu_device;
	//Render_Pipeline *render_pipeline = &Engine::get_render_system()->render_pipeline;

	//String_Id mesh_string_id;
	//if (!triangle_mesh->name.is_empty() && !triangle_mesh->file_name.is_empty()) {
	//	mesh_string_id = fast_hash(triangle_mesh->file_name + "_" + triangle_mesh->name);
	//} else if (exclusive_or(triangle_mesh->name.is_empty(), triangle_mesh->file_name.is_empty())) {
	//	if (triangle_mesh->name.is_empty()) {
	//		mesh_string_id = fast_hash(triangle_mesh->file_name);
	//		print("[Mesh storage] Warning: A tringle mesh contains only a file name '{}' it's possible to get the collision.", triangle_mesh->file_name);
	//	} else {
	//		mesh_string_id = fast_hash(triangle_mesh->name);
	//		print("[Mesh storage] Warning: A tringle mesh contains only a name '{}' it's possible to get the collision.", triangle_mesh->name);
	//	}
	//} else {
	//	print("[Mesh storage] Error: Not possible to add a triangle mesh to the storage. the triangle mesh doesn't has a file name and mesh name.");
	//	return false;
	//}

	//if (triangle_mesh->empty()) {
	//	print("Render_World::add_mesh: Mesh {} from {} can be added because doesn't have all necessary data.", triangle_mesh->name, triangle_mesh->file_name);
	//	return false;
	//}

	//if (mesh_table.get(mesh_string_id, mesh_id)) {
	//	print("[Mesh storage] Info: {} mesh has already been placed in the mesh storage.", triangle_mesh->get_pretty_name());
	//	return true;
	//}

	//Mesh_Textures mesh_textures;
	//mesh_textures.normal_idx = find_texture_or_get_default(triangle_mesh->normal_texture_name, default_textures.normal);
	//mesh_textures.diffuse_idx = find_texture_or_get_default(triangle_mesh->diffuse_texture_name, default_textures.diffuse);
	//mesh_textures.specular_idx = find_texture_or_get_default(triangle_mesh->specular_texture_name, default_textures.specular);
	//mesh_textures.displacement_idx = find_texture_or_get_default(triangle_mesh->displacement_texture_name, default_textures.displacement);

	//mesh_id->textures_idx = meshes_textures.push(mesh_textures);

	//Mesh_Instance mesh_info;
	//mesh_info.vertex_count = triangle_mesh->vertices.count;
	//mesh_info.index_count = triangle_mesh->indices.count;
	//mesh_info.vertex_offset = unified_vertices.count;
	//mesh_info.index_offset = unified_indices.count;

	//mesh_id->instance_idx = mesh_instances.push(mesh_info);

	//merge(&unified_vertices, &triangle_mesh->vertices);
	//merge(&unified_indices, &triangle_mesh->indices);

	//mesh_struct_buffer.update(&mesh_instances);
	//vertex_struct_buffer.update(&unified_vertices);
	//index_struct_buffer.update(&unified_indices);

	//mesh_table.set(mesh_string_id, *mesh_id);
	//
	return true;
}

bool Model_Storage::update_mesh(Mesh_Id mesh_id, Triangle_Mesh *triangle_mesh)
{
	//Mesh_Instance mesh_instance = mesh_instances[mesh_id.instance_idx];
	//if ((triangle_mesh->vertices.count == mesh_instance.vertex_count) && (triangle_mesh->indices.count == mesh_instance.index_count)) {
	//	u32 vertex_offset = mesh_instance.vertex_offset > 0 ? mesh_instance.vertex_offset : 0;
	//	u32 index_offset = mesh_instance.index_offset > 0 ? mesh_instance.index_offset : 0;
	//	copy_array(&unified_vertices, &triangle_mesh->vertices, vertex_offset);
	//	copy_array(&unified_indices, &triangle_mesh->indices, index_offset);

	//	vertex_struct_buffer.update(&unified_vertices);
	//	index_struct_buffer.update(&unified_indices);
	//	return true;
	//}
	return false;
}

Texture_Idx Model_Storage::find_texture_or_get_default(String &texture_file_name, String &mesh_file_name, Texture_Idx default_texture)
{
	Texture_Idx texture_idx;
	if (!texture_file_name.is_empty()) {
		String full_path_to_texture_file;
		if (!mesh_file_name.is_empty()) {
			String base_file_name;
			extract_base_file_name(mesh_file_name, base_file_name);

			build_full_path_to_texture_file(texture_file_name, base_file_name, full_path_to_texture_file);
			if (file_exists(full_path_to_texture_file) && add_texture(texture_file_name, full_path_to_texture_file, &texture_idx)) {
				return texture_idx;
			}
		}
		build_full_path_to_texture_file(texture_file_name, full_path_to_texture_file);
		if (file_exists(full_path_to_texture_file) && add_texture(texture_file_name, full_path_to_texture_file, &texture_idx)) {
			return texture_idx;
		}
		print(" Mesh_Storate::find_texture_or_get_default: The engine can not find texture {}.", texture_file_name);
	}
	return default_texture;
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
	////@Note: Why don't pass just the engine pointer ?
	//game_world = &engine->game_world;
	//render_sys = &engine->render_sys;

	//triangle_meshes.init(&render_sys->gpu_device);

	//init_shadow_rendering();

	//u32 x = 128;
	//voxel_grid.grid_size = { x, x, x };
	//u32 y = 20;
	//voxel_grid.ceil_size = { y, y, y };

	//voxels_sb.allocate<Voxel>(voxel_grid.grid_size.find_area());

	//Size_f32 grid_size = voxel_grid.total_size();
	//float grid_depth = grid_size.depth;
	//grid_size *= 0.5f;

	//voxel_matrix = XMMatrixOrthographicOffCenterLH(-grid_size.width, grid_size.width, -grid_size.height, grid_size.height, 1.0f, grid_depth + 1.0f);

	//init_render_passes(&engine->shader_manager);

	//render_sys->gpu_device.create_constant_buffer(sizeof(CB_Frame_Info), &frame_info_cbuffer);

	//lights_struct_buffer.allocate<Hlsl_Light>(100);
	//world_matrices_struct_buffer.allocate<Matrix4>(100);

	//if (!render_camera.is_entity_camera_set()) {
	//	error("Render Camera was not initialized. There is no a view for rendering.");
	//}
}

void Render_World::init_shadow_rendering()
{
	//Texture2D_Desc depth_stencil_desc;
	//depth_stencil_desc.width = SHADOW_ATLAS_SIZE;
	//depth_stencil_desc.height = SHADOW_ATLAS_SIZE;
	//depth_stencil_desc.format = DXGI_FORMAT_R24G8_TYPELESS;
	//depth_stencil_desc.mip_levels = 1;
	//depth_stencil_desc.bind |= BIND_DEPTH_STENCIL;

	//render_sys->gpu_device.create_texture_2d(&depth_stencil_desc, &shadow_atlas);
	//render_sys->gpu_device.create_depth_stencil_view(&depth_stencil_desc, &shadow_atlas);
	//render_sys->gpu_device.create_shader_resource_view(&depth_stencil_desc, &shadow_atlas);

	//fill_texture((void *)&DEFAULT_DEPTH_VALUE, &shadow_atlas);

	//shadow_cascade_ranges.push({ 1, 15 });
	//shadow_cascade_ranges.push({ 15, 150 });
	//shadow_cascade_ranges.push({ 150, 500 });
	//shadow_cascade_ranges.push({ 500, 1000 });

	//jittering_tile_size = 16;
	//jittering_filter_size = 8;
	//jittering_scaling = jittering_filter_size / 2;

	//Array<Vector2> jittered_samples;
	//make_jittering_sampling_filters(jittering_tile_size, jittering_filter_size, jittered_samples);

	//Texture3D_Desc jittering_samples_texture_desc;
	//jittering_samples_texture_desc.width = math::pow2(jittering_filter_size);
	//jittering_samples_texture_desc.height = jittering_tile_size;
	//jittering_samples_texture_desc.depth = jittering_tile_size;
	//jittering_samples_texture_desc.format = DXGI_FORMAT_R32G32_FLOAT;
	//jittering_samples_texture_desc.bind = BIND_SHADER_RESOURCE;
	//jittering_samples_texture_desc.data = (void *)jittered_samples.items;
	//jittering_samples_texture_desc.mip_levels = 1;

	//render_sys->gpu_device.create_texture_3d(&jittering_samples_texture_desc, &jittering_samples);
	//render_sys->gpu_device.create_shader_resource_view(&jittering_samples_texture_desc, &jittering_samples);
}

void Render_World::init_render_passes(Shader_Manager *shader_manager)
{
	//assert(shader_manager);

	//print("Render_World::init: Initializing render passes.");

	//Array<Render_Pass *> render_pass_list;
	//render_passes.get_all_passes(&render_pass_list);

	//for (u32 i = 0; i < render_pass_list.count; i++) {
	//	render_pass_list[i]->init(&render_sys->gpu_device, &render_sys->render_pipeline_states);
	//}

	//Viewport viewport;
	//viewport.width = Render_System::screen_width;
	//viewport.height = Render_System::screen_height;

	//render_passes.shadows.setup_render_pipeline(shader_manager, shadow_atlas.dsv);
	//render_passes.forward_light.setup_render_pipeline(shader_manager, render_sys->multisampling_depth_stencil_texture.dsv, render_sys->multisampling_back_buffer_texture.rtv, &viewport);
	//render_passes.debug_cascade_shadows.setup_render_pipeline(shader_manager, render_sys->multisampling_depth_stencil_texture.dsv, render_sys->multisampling_back_buffer_texture.rtv, &viewport);
	//
	//render_passes.voxelization.setup_render_pipeline(shader_manager, voxels_sb.gpu_buffer.uav, render_sys->voxel_render_target.rtv);

	//render_passes.outlining.setup_outlining(2, Color(245, 176, 66));
	//render_passes.outlining.setup_render_pipeline(shader_manager, &render_sys->silhouette_buffer, &render_sys->silhouette_depth_stencil_buffer, &render_sys->back_buffer_texture, &render_sys->multisampling_depth_stencil_texture, &viewport);

	//Array<Render_Pass *> temp;
	//temp.push(&render_passes.shadows);
	//temp.push(&render_passes.forward_light);
	//temp.push(&render_passes.voxelization);

	//for (u32 i = 0; i < render_pass_list.count; i++) {
	//	if (render_pass_list[i]->is_valid) {
	//		print("  Render pass '{}' was successfully initialized.", &render_pass_list[i]->name);
	//	} else {
	//		print("  Render pass '{}' is not valid. The render pass will not be used for rendering.", &render_pass_list[i]->name);
	//	}
	//}
	//for (u32 i = 0; i < temp.count; i++) {
	//	if (temp[i]->is_valid) {
	//		frame_render_passes.push(temp[i]);
	//	}
	//}
}

void Render_World::release_all_resources()
{
	release_render_entities_resources();
	shadow_cascade_ranges.clear();

	//shadow_atlas.release();
	//jittering_samples.release();
	//frame_info_cbuffer.free();
}

void Render_World::release_render_entities_resources()
{
	light_info_list.clear();
	shader_lights.clear();

	render_entity_world_matrices.clear();
	light_view_matrices.clear();
	cascaded_view_projection_matrices.clear();

	game_render_entities.clear();

	cascaded_shadows_list.clear();
	cascaded_shadows_info_list.clear();

	model_storage.release_all_resources();

	//lights_struct_buffer.free();
	//cascaded_shadows_info_sb.free();
	//world_matrices_struct_buffer.free();
	//cascaded_view_projection_matrices_sb.free();
}

void Render_World::update()
{
	update_render_entities();

	Camera *camera = game_world->get_camera(render_camera.camera_id);
	Camera *camera_info = game_world->get_camera(render_camera.camera_info_id);
	render_camera.update(camera, camera_info);

	//frame_info.view_matrix = render_camera.view_matrix;
	//frame_info.perspective_matrix = render_sys->view.perspective_matrix;
	//frame_info.orthographic_matrix = render_sys->view.orthogonal_matrix;
	//frame_info.camera_position = camera_info->position;
	//frame_info.camera_direction = camera_info->target;
	//frame_info.near_plane = render_sys->view.near_plane;
	//frame_info.far_plane = render_sys->view.far_plane;

	update_shadows();
	update_global_illumination();
}

void Render_World::update_render_entities()
{
	Render_Entity *render_entity = NULL;
	For(game_render_entities, render_entity) {
		Entity *entity = game_world->get_entity(render_entity->entity_id);
		render_entity_world_matrices[render_entity->world_matrix_idx] = get_world_matrix(entity);
	}
	//world_matrices_struct_buffer.update(&render_entity_world_matrices);
}

void Render_World::update_global_illumination()
{	
	Vector3 voxel_ceil_size = voxel_grid.ceil_size.to_vector3();
	Vector3 voxel_grid_size = voxel_grid.total_size().to_vector3() * 0.5f; // Holdes the half of a total voxel grid size.

	Camera *camera = game_world->get_camera(render_camera.camera_id);
	auto dir = camera->target - camera->position;
	voxel_grid_center = camera->position + (normalize(&dir) * voxel_grid_size);
	voxel_grid_center /= voxel_ceil_size;
	voxel_grid_center = floor(voxel_grid_center);
	voxel_grid_center *= voxel_ceil_size;
	
	Vector3 left_to_right_view_position = { voxel_grid_center.x - voxel_grid_size.x, voxel_grid_center.y, voxel_grid_center.z };
	Vector3 top_to_down_view_position = { voxel_grid_center.x, voxel_grid_center.y + voxel_grid_size.y, voxel_grid_center.z };
	Vector3 back_to_front_view_position = { voxel_grid_center.x, voxel_grid_center.y, voxel_grid_center.z - voxel_grid_size.z };
	
	left_to_right_voxel_view_matrix = make_look_to_matrix(left_to_right_view_position, Vector3::base_x);
	top_to_down_voxel_view_matrix = make_look_to_matrix(top_to_down_view_position, negate(&Vector3::base_y), negate(&Vector3::base_z));
	back_to_front_voxel_view_matrix = make_look_to_matrix(back_to_front_view_position, Vector3::base_z);
}

void Render_World::add_light(Entity_Id light_id)
{
	Light *light = (Light *)game_world->get_entity(light_id);
	if (!light) {
		return;
	}
	frame_info.light_count++;

	Light_Info light_info;
	light_info.light_id = get_entity_id(light);
	light_info.cascade_shadows_index = cascaded_shadows_list.count;
	light_info.cascaded_shadows_info_index = cascaded_shadows_info_list.count;
	light_info.hlsl_light_index = shader_lights.count;

	if (add_shadow(light)) {
		Hlsl_Light hlsl_light;
		hlsl_light.position = light->position;
		hlsl_light.direction = normalize(&light->direction);
		hlsl_light.color = light->color;
		hlsl_light.radius = light->radius;
		hlsl_light.range = light->range;
		hlsl_light.light_type = light->light_type;

		shader_lights.push(hlsl_light);
		lights_struct_buffer.update(&shader_lights);

		light_info_list.push(light_info);
	}
	//lights_struct_buffer.update(&shader_lights);
}

void Render_World::add_render_entity(Entity_Id entity_id, Mesh_Id mesh_id, void *args)
{
	Render_Entity render_entity;
	render_entity.entity_id = entity_id;
	render_entity.mesh_id = mesh_id;
	render_entity.world_matrix_idx = render_entity_world_matrices.push(Matrix4());

	game_render_entities.push(render_entity);
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
	/*Cascaded_Shadows_Info cascaded_shadows_info;
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
	cascaded_shadows_list.push(cascaded_shadows);*/
	return true;
}

static bool find_cascade_shadows(const Light_Info &light_info, const Entity_Id &light_id)
{
	return light_info.light_id == light_id;
}

void Render_World::update_light(Light *light)
{
	Find_Result<Light_Info> result = find_in_array(light_info_list, get_entity_id(light), find_cascade_shadows);
	if (result.found) {
		Light_Info light_info = result.data;
		cascaded_shadows_list[light_info.cascade_shadows_index].light_direction = light->direction;
		
		cascaded_shadows_info_list[light_info.cascaded_shadows_info_index].light_direction = light->direction;
		cascaded_shadows_info_sb.update(&cascaded_shadows_info_list);

		Hlsl_Light *hlsl_light = &shader_lights[light_info.hlsl_light_index];
		hlsl_light->position = light->position;
		hlsl_light->direction = normalize(&light->direction);
		hlsl_light->color = light->color;
		hlsl_light->radius = light->radius;
		hlsl_light->range = light->range;
		hlsl_light->light_type = light->light_type;
		lights_struct_buffer.update(&shader_lights);
	} else {
		print("Render_World::update_cascade_shadows: A cascade shadows was not found. Can not update The cascade shadows");
	}
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

			Matrix4 projection_matrix = XMMatrixOrthographicOffCenterLH(-radius, radius, -radius, radius, -5000.0f, 5000.0f);

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
	//world_matrices_struct_buffer.update(&render_entity_world_matrices);
	//cascaded_view_projection_matrices_sb.update(&cascaded_view_projection_matrices);
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

//bool Render_World::get_shadow_atls_viewport(Viewport *viewport)
//{
//	static u32 x = 0;
//	static u32 y = 0;
//
//	Point_u32 shadow_map_position;
//	shadow_map_position.x = CASCADE_SIZE + x;
//	shadow_map_position.y = CASCADE_SIZE + y;
//
//	if ((shadow_map_position.x <= SHADOW_ATLAS_SIZE) && (shadow_map_position.y <= SHADOW_ATLAS_SIZE)) {
//		viewport->x = x;
//		viewport->y = y;
//		viewport->width = CASCADE_SIZE;
//		viewport->height = CASCADE_SIZE;
//		x += CASCADE_SIZE;
//	} else if ((shadow_map_position.x > SHADOW_ATLAS_SIZE) && (shadow_map_position.y < SHADOW_ATLAS_SIZE)) {
//		viewport->x = 0;
//		viewport->y = y;
//		viewport->width = CASCADE_SIZE;
//		viewport->height = CASCADE_SIZE;
//		x = 0;
//		y += CASCADE_SIZE;
//	} else {
//		print("Render_World::get_shadow_atls_view_port: The shadow atlas is out of space.");
//		return false;
//	}
//	return true;
//}

void Render_World::render()
{
	//render_sys->render_pipeline.update_constant_buffer(&frame_info_cbuffer, (void *)&frame_info);

	//render_sys->render_pipeline.set_vertex_shader_resource(CB_FRAME_INFO_REGISTER, frame_info_cbuffer);
	//render_sys->render_pipeline.set_pixel_shader_resource(CB_FRAME_INFO_REGISTER, frame_info_cbuffer);

	//Render_Pass *render_pass = NULL;
	//For(frame_render_passes, render_pass) {
	//	render_pass->render(this, &render_sys->render_pipeline);
	//}
}

void Render_World::Render_Passes::get_all_passes(Array<Render_Pass *> *render_passes_list)
{
	//render_passes_list->push(&shadows);
	//render_passes_list->push(&forward_light);
	//render_passes_list->push(&debug_cascade_shadows);
	//render_passes_list->push(&outlining);
	//render_passes_list->push(&voxelization);
}
