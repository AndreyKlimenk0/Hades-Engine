#include <stdint.h>

#include "render_world.h"
#include "../gui/gui.h"
#include "../sys/engine.h"
#include "../collision/collision.h"

const Color DEFAULT_MESH_COLOR = Color(105, 105, 105);

inline Matrix4 get_world_matrix(Entity *entity) 
{
	if (entity->type == ENTITY_TYPE_CAMERA) {
		Camera *camera = static_cast<Camera *>(entity);
		return inverse(&make_view_matrix(&camera->position, &camera->target));
	}
	return make_translation_matrix(&entity->position);
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

template<typename T>
void Unified_Mesh_Storate<T>::allocate_gpu_memory()
{
	mesh_struct_buffer.allocate<Mesh_Instance>(1000);
	index_struct_buffer.allocate<u32>(100000);
	vertex_struct_buffer.allocate<Vertex_XNUV>(100000);
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

void Render_World::init()
{	
	//@Note: Why don't pass just the engine pointer ?
	game_world = Engine::get_game_world();
	render_sys = Engine::get_render_system();

	init_shadow_rendering();

	init_render_passes();
	
	render_sys->gpu_device.create_constant_buffer(sizeof(Frame_Info), &frame_info_cbuffer);

	lights_struct_buffer.allocate<Shader_Light>(100);
	world_matrices_struct_buffer.allocate<Matrix4>(100);
	cascaded_view_projection_matrices_sb.allocate<Matrix4>(100);
	
	Texture_Desc texture_desc;
	texture_desc.width = 200;
	texture_desc.height = 200;
	texture_desc.mip_levels = 1;
	
	render_sys->gpu_device.create_texture_2d(&texture_desc, &default_texture);
	render_sys->gpu_device.create_shader_resource_view(&default_texture);
	fill_texture_with_value((void *)&DEFAULT_MESH_COLOR, &default_texture);

	//game_world->make_direction_light(Vector3(0.5f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));
	game_world->make_direction_light(Vector3(1.0f, -1.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f));

	Mesh_Loader *mesh_loader = Engine::get_mesh_loader();
	mesh_loader->load("mutant.fbx");
	//mesh_loader->load("Scene_Demo2.fbx");

	Mesh_Loader::Mesh_Instance *mesh_instance = NULL;
	For(mesh_loader->mesh_instances, mesh_instance) {
		Mesh_Idx mesh_idx;
		if (add_mesh(mesh_instance->name, &mesh_instance->mesh, &mesh_idx)) {
			for (u32 j = 0; j < mesh_instance->transform_matrices.count; j++) {
				Entity_Id entity_id = game_world->make_entity(Vector3(0.0f, 0.0f, 0.0f));
				auto m = rotate_about_y(XMConvertToRadians(90.0f));
				make_render_entity(entity_id, mesh_idx, &mesh_instance->transform_matrices[j]);
			}
		}
	}

	//for (u32 i = 0; i < 10000; i++) {
	//	Entity_Id entity_id = game_world->make_entity(Vector3(0.0f, 0.0f, 0.0f));
	//}

	if (!render_camera.is_entity_camera_set()) {
		error("Render Camera was not initialized. There is no a view for rendering.");
	}
}

void Render_World::init_shadow_rendering()
{	
	Texture_Desc depth_stencil_desc;
	depth_stencil_desc.width = SHADOW_ATLAS_WIDTH;
	depth_stencil_desc.height = SHADOW_ATLAS_HEIGHT;
	depth_stencil_desc.format = DXGI_FORMAT_R24G8_TYPELESS;
	depth_stencil_desc.mip_levels = 1;
	depth_stencil_desc.bind |= BIND_DEPTH_STENCIL;
	depth_stencil_desc.multisampling = { 1, 0 };

	render_sys->gpu_device.create_texture_2d(&depth_stencil_desc, &shadow_atlas);
	render_sys->gpu_device.create_depth_stencil_view(&shadow_atlas);
	render_sys->gpu_device.create_shader_resource_view(&shadow_atlas);

	fill_texture_with_value((void *)&DEFAULT_DEPTH_VALUE, &shadow_atlas);

	//shadow_cascade_ranges.push({ 0, 15 });
	//shadow_cascade_ranges.push({ 15, 150 });
	//shadow_cascade_ranges.push({ 150, 500 });
	//shadow_cascade_ranges.push({ 500, 1000 });

	//shadow_cascade_ranges.push({ 0, 15 });
	//shadow_cascade_ranges.push({ 15, 100 });
	//shadow_cascade_ranges.push({ 200, 300 });
	//shadow_cascade_ranges.push({ 300, 400 });

	shadow_cascade_ranges.push({ 0, 15 });
	shadow_cascade_ranges.push({ 10, 50 });
	shadow_cascade_ranges.push({ 40, 120 });
	shadow_cascade_ranges.push({ 100, 320 });

	//shadow_cascade_ranges.push({ 0, 50 });
	//shadow_cascade_ranges.push({ 50, 100 });
	//shadow_cascade_ranges.push({ 100, 150 });
	//shadow_cascade_ranges.push({ 150, 200 });
}

void Render_World::init_render_passes()
{
	if (!render_passes.debug_cascade_shadows.init((void *)this, render_sys)) {
		print("Render_World::init_render_passes: Failed to initialize render pass {}.", render_passes.debug_cascade_shadows.name);
	}

	render_passes_array.push(&render_passes.shadows);
	render_passes_array.push(&render_passes.draw_lines);
	render_passes_array.push(&render_passes.forward_light);

	for (u32 i = 0; i < render_passes_array.count; i++) {
		if (!render_passes_array[i]->init((void *)this, render_sys)) {
			print("Render_World::init_render_passes: Failed to initialize render pass {}.", render_passes_array[i]->name);
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

	update_shadows();

	Camera *camera = game_world->get_camera(render_camera.camera_id);
	render_camera.update(camera);

	frame_info.view_matrix = render_camera.view_matrix;
	frame_info.perspective_matrix = render_sys->view.perspective_matrix;
	frame_info.orthographic_matrix = render_sys->view.orthogonal_matrix;
	frame_info.camera_position = camera->position;
	frame_info.camera_direction = camera->target;
	frame_info.near_plane = render_sys->view.near_plane;
	frame_info.far_plane = render_sys->view.far_plane;
}

void Render_World::update_render_entities()
{
	Render_Entity *render_entity = NULL;
	For (render_entities, render_entity) {
		Entity *entity = game_world->get_entity(render_entity->entity_id);
		world_matrices[render_entity->world_matrix_idx] = get_world_matrix(entity);
	}

	For (bounding_box_entities, render_entity) {
		Entity *entity = game_world->get_entity(render_entity->entity_id);
		world_matrices[render_entity->world_matrix_idx] = get_world_matrix(entity);
	}
	world_matrices_struct_buffer.update(&world_matrices);
}


void Render_World::update_lights()
{
	frame_info.light_count = game_world->lights.count;

	Array<Shader_Light> shader_lights;

	Light *light = NULL;
	For(game_world->lights, light) {
		Shader_Light hlsl_light;
		hlsl_light.position = light->position;
		hlsl_light.direction = normalize(&light->direction);
		hlsl_light.color = light->color;
		hlsl_light.radius = light->radius;
		hlsl_light.range = light->range;
		hlsl_light.light_type = light->light_type;
		hlsl_light.shadow_map_idx = 0;
		make_shadow(light);
		shader_lights.push(hlsl_light);
	}
	lights_struct_buffer.update(&shader_lights);
}

Render_Entity *Render_World::find_render_entity(Entity_Id entity_id)
{
	Render_Entity *render_entity = NULL;
	For(render_entities, render_entity) {
		if (render_entity->entity_id == entity_id) {
			return render_entity;
		}
	}
	return NULL;
}

void Render_World::make_render_entity(Entity_Id entity_id, Mesh_Idx mesh_idx)
{
	Matrix4 world_matrix = get_world_matrix(game_world->get_entity(entity_id));
	
	Render_Entity render_entity;
	render_entity.entity_id = entity_id;
	render_entity.mesh_idx = mesh_idx;
	render_entity.world_matrix_idx = world_matrices.push(world_matrix);
	render_entities.push(render_entity);
	
	world_matrices_struct_buffer.update(&world_matrices);
}

void Render_World::make_line_render_entity(Entity_Id entity_id, Mesh_Idx mesh_idx)
{
	Matrix4 world_matrix = get_world_matrix(game_world->get_entity(entity_id));

	Render_Entity render_entity;
	render_entity.entity_id = entity_id;
	render_entity.mesh_idx = mesh_idx;
	render_entity.world_matrix_idx = world_matrices.push(world_matrix);
	bounding_box_entities.push(render_entity);
	
	world_matrices_struct_buffer.update(&world_matrices);
}

void Render_World::make_render_entity(Entity_Id entity_id, Mesh_Idx mesh_idx, Matrix4 *matrix)
{
	Render_Entity render_entity;
	render_entity.entity_id = entity_id;
	render_entity.mesh_idx = mesh_idx;
	render_entity.world_matrix_idx = world_matrices.push(*matrix);
	render_entities.push(render_entity);

	world_matrices_struct_buffer.update(&world_matrices);
}

void Frustum_Box::Plane::setup(float plane_width, float plane_height, float z_position)
{
	plane_width *= 0.5f;
	plane_height *= 0.5f;
	origin_top_left = Vector3(-plane_width, plane_height, z_position);
	origin_top_right = Vector3(plane_width, plane_height, z_position);
	origin_bottom_left = Vector3(-plane_width, -plane_height, z_position);
	origin_bottom_right = Vector3(plane_width, -plane_height, z_position);
	
	top_left = origin_top_left;
	top_right = origin_top_right;
	bottom_left = origin_bottom_left;
	bottom_right = origin_bottom_right;
}

void Frustum_Box::Plane::transform_plane(Matrix4 *transform_matrix)
{
	top_left = transform(&origin_top_left, transform_matrix);
	top_right = transform(&origin_top_right, transform_matrix);
	bottom_left = transform(&origin_bottom_left, transform_matrix);
	bottom_right = transform(&origin_bottom_right, transform_matrix);
}

void Frustum_Box::Plane::get_vertices(Array<Vector3> *vertices)
{
	vertices->clear();
	vertices->push(top_left);
	vertices->push(top_right);
	vertices->push(bottom_left);
	vertices->push(bottom_right);
}

void Frustum_Box::calculate_length()
{
	//auto r1 = (first_plane.origin_bottom_right - second_plane.origin_top_left);
	//auto r2 = length(first_plane.origin_bottom_right - second_plane.origin_top_left).length();
	//auto s1 = (second_plane.origin_bottom_right - second_plane.origin_top_left);
	//auto s2 = (second_plane.origin_bottom_right - second_plane.origin_top_left).length();
	auto x = near_plane.origin_bottom_right - far_plane.origin_top_left;
	auto y = far_plane.origin_bottom_right - far_plane.origin_top_left;
	len = (u32)math::max(get_length(&x), get_length(&y));
}

#include <limits.h>

void Frustum_Box::update_min_max_values()
{
	Array<Vector3> vertices;
	Array<Vector3> second_plane_vertices;

	near_plane.get_vertices(&vertices);
	far_plane.get_vertices(&second_plane_vertices);

	max_x = std::numeric_limits<float>::lowest();
	max_y = std::numeric_limits<float>::lowest();
	max_z = std::numeric_limits<float>::lowest();

	min_x = std::numeric_limits<float>::max();
	min_y = std::numeric_limits<float>::max();
	min_z = std::numeric_limits<float>::max();

	merge(&vertices, &second_plane_vertices);

	for (u32 i = 0; i < vertices.count; i++) {
		Vector3 vertex = vertices[i];
		max_x = math::max(max_x, vertex.x);
		max_y = math::max(max_y, vertex.y);
		max_z = math::max(max_z, vertex.z);
		min_x = math::min(min_x, vertex.x);
		min_y = math::min(min_y, vertex.y);
		min_z = math::min(min_z, vertex.z);
	}
}

const u32 CASCADE_SHADOW_MAP_SIZE = 1024;

Vector3 Frustum_Box::get_view_position()
{
	//return Vector3((max_x + min_x) / (length / (float)CASCADE_SHADOW_MAP_SIZE) * 2.0f, (max_y + min_y) / (length / (float)CASCADE_SHADOW_MAP_SIZE) * 2.0f, min_z);
	//float x_result = (max_x + min_x);
	//float y_result = (max_y + min_y);
	//float x = (length / (float)CASCADE_SHADOW_MAP_SIZE);
	//float xx = (length / (float)CASCADE_SHADOW_MAP_SIZE) * 2.0f;
	//float y = (length / (float)CASCADE_SHADOW_MAP_SIZE);
	//float yy = (length / (float)CASCADE_SHADOW_MAP_SIZE) * 2.0f;
	//float xxx = x_result / xx;
	//float yyy = y_result / yy;
	//auto temp = Vector3((max_x + min_x) / ((length / (float)CASCADE_SHADOW_MAP_SIZE) * 2.0f), (max_y + min_y) / ((length / (float)CASCADE_SHADOW_MAP_SIZE) * 2.0f), min_z);
	//return Vector3((max_x + min_x) / ((length / (float)CASCADE_SHADOW_MAP_SIZE) * 2.0f), (max_y + min_y) / ((length / (float)CASCADE_SHADOW_MAP_SIZE) * 2.0f), min_z);
	return Vector3((max_x + min_x) / (2.0f), (max_y + min_y) / (2.0f), min_z);
}

void Cascaded_Shadow::init(float fov, Shadow_Cascade_Range *shadow_cascade_range)
{
	range = *shadow_cascade_range;
	frustum_box.near_plane.setup((float)range.start * fov, (float)range.start * fov, (float)range.start);
	frustum_box.far_plane.setup((float)range.end * fov, (float)range.end * fov, (float)range.end);
	frustum_box.calculate_length();
	frustum_box.update_min_max_values();
}

void Cascaded_Shadow::transform(Matrix4 *transform_matrix)
{
	frustum_box.near_plane.transform_plane(transform_matrix);
	frustum_box.far_plane.transform_plane(transform_matrix);
	frustum_box.update_min_max_values();
}

Matrix4 make_rotation_matrix(Vector3 *direction, Vector3 *up_direction = NULL)
{
	assert(direction);

	Vector3 z_axis = normalize(direction);
	Vector3 up = { 0.0f, 1.0f, 0.0f };
	if (up_direction) {
		up = *up_direction;
	}
	Vector3 x_axis = normalize(&cross(&up, &z_axis));
	Vector3 y_axis = normalize(&cross(&z_axis, &x_axis));

	Matrix4 rotation_matrix = make_identity_matrix();
	rotation_matrix.set_row_0(Vector4(x_axis, 0.0f));
	rotation_matrix.set_row_1(Vector4(y_axis, 0.0f));
	rotation_matrix.set_row_2(Vector4(z_axis, 0.0f));
	return rotation_matrix;
}

Matrix4 Cascaded_Shadow::get_cascade_view_matrix()
{
	//Matrix4 cascade_space_matrix = light_matrix;
	//cascade_space_matrix[3] = Vector4(frustum_box.get_view_position(), 1.0f);
	//cascade_space_matrix = XMMatrixInverse(NULL, cascade_space_matrix);
	//cascade_space_matrix = cascade_space_matrix.inverse();
	//cascade_space_matrix.translate(&frustum_box.get_view_position());
	//return cascade_space_matrix;
	//return XMMatrixLookAtLH(frustum_box.get_view_position(), light_direction, Vector3(0.0f, 1.0f, 0.0f));
	//Matrix4 m;
	//m.indentity();
	//return Engine::get_render_world()->camera.get_view_matrix();
	
	//Matrix4 cascade_space_matrix;
	//cascade_space_matrix.indentity();
	//cascade_space_matrix.translate(&frustum_box.get_view_position());
	//Matrix4 x = Engine::get_render_world()->camera.get_view_matrix();
	Vector3 p = frustum_box.get_view_position();
	//Matrix4 x = light_matrix * make_translation_matrix(&frustum_box.get_view_position());
	Matrix4 x = light_matrix;
	Matrix4 result = light_matrix;
	return inverse(&result);
}

Matrix4 Cascaded_Shadow::get_cascade_projection_matrix()
{
	Matrix4 projection_matrix = make_identity_matrix();
	projection_matrix.m[0][0] = 2.0f / frustum_box.len;
	projection_matrix.m[1][1] = 2.0f / frustum_box.len;
	projection_matrix.m[2][2] = 1.0f / (frustum_box.max_z - frustum_box.min_z);
	return projection_matrix;
	//return XMMatrixOrthographicLH(1024.0f, 1024.0f, 1.0f, 1000.0f);
	//return Engine::get_render_system()->view.perspective_matrix;
}

//inline bool is_closed_interval_value(float value, float interval_start = 0.0f, float interval_end = 1.0f)
//{
//	assert(interval_end > interval_start);
//	if ((value >= interval_start) && (value <= interval_end)) {
//		return true;
//	}
//	return false;
//}
//
//inline bool is_normalized(Vector3 *vec)
//{
//	assert(vec);
//	bool x = is_closed_interval_value(vec->x);
//	bool y = is_closed_interval_value(vec->y);
//	bool z = is_closed_interval_value(vec->z);
//	return (x && y && z);
//}

bool Render_World::make_shadow(Light *light)
{
	Cascaded_Shadow_Map cascaded_shadow_map;

	light->direction = Vector3( 1.0f, -1.0f, 0.0f );
	auto temp = make_rotation_matrix(&light->direction);
	
	float fov = render_sys->view.fov;
	//Matrix4 light_matrix = make_rotation_matrix(&light->direction);
	//Matrix4 light_matrix = rotate_about_y(XMConvertToRadians(45.0f));
	Matrix4 light_matrix = rotate_about_y(XMConvertToRadians(00.0f));
	

	String s = "tasdfasdg";
	for (u32 i = 0; i < shadow_cascade_ranges.count; i++) {
		Cascaded_Shadow cascaded_shadow;
		cascaded_shadow.view_projection_matrix_index = cascaded_view_projection_matrices.push(Matrix4());
		cascaded_shadow.light_direction = light->direction;
		cascaded_shadow.light_matrix = light_matrix;
		cascaded_shadow.init(fov, &shadow_cascade_ranges[i]);

		cascaded_shadow.transform(&light_matrix);
		Vector3 min = Vector3(cascaded_shadow.frustum_box.min_x, cascaded_shadow.frustum_box.min_y, cascaded_shadow.frustum_box.min_z);
		Vector3 max = Vector3(cascaded_shadow.frustum_box.max_x, cascaded_shadow.frustum_box.max_y, cascaded_shadow.frustum_box.max_z);

		Line_Mesh line_mesh;
		make_AABB_mesh(&min, &max, &line_mesh);
		Mesh_Idx mesh_idx;
		add_mesh(s, &line_mesh, &mesh_idx);
		Entity_Id id = game_world->make_entity(cascaded_shadow.frustum_box.get_view_position());
		
		Render_Entity new_render_entity;
		new_render_entity.entity_id = id;

		new_render_entity.world_matrix_idx = world_matrices.push(light_matrix);
		new_render_entity.mesh_idx = mesh_idx;
		cascaded_shadow.matrix_index = new_render_entity.world_matrix_idx;

		//bounding_box_entities.push(new_render_entity);

		s.append("A");
		
		if (!get_shadow_atls_viewport(&cascaded_shadow.viewport)) {
			return false;
		}
		cascaded_shadow_map.cascaded_shadows.push(cascaded_shadow);
		cascaded_shadow_count++;
	}
	world_matrices_struct_buffer.update(&world_matrices);
	cascaded_shadow_maps.push(cascaded_shadow_map);
	return true;
}

#include "../win32/win_time.h"

void Render_World::update_shadows()
{
	static s64 i = 0;
	static s64 x = 0;
	static s64 start = 0;
	static s64 end = 0;
	static s64 ac = 0;

	start = milliseconds_counter();
	ac += start - end;
	if (ac > 250) {
		ac = 0;
		if (i > 360) {
			i = 0;
		}		
		x = i++;
		//print("X = ", x);
	}
	for (u32 i = 0; i < cascaded_shadow_maps.count; i++) {
		for (u32 j = 0; j < cascaded_shadow_maps[i].cascaded_shadows.count; j++) {
			Cascaded_Shadow *cascaded_shadow = &cascaded_shadow_maps[i].cascaded_shadows[j];
			//Matrix4 view_matrix = camera.get_view_matrix();
			Matrix4 light_matrix = cascaded_shadow_maps[i].cascaded_shadows[j].light_matrix;
			//Matrix4 light_view_matrix = light_matrix * XMMatrixInverse(NULL, camera.get_view_matrix());
			//Matrix4 light_view_matrix = inverse(&light_matrix) * camera.get_view_matrix();
			//Matrix4 light_view_matrix = inverse(&camera.get_view_matrix());
			//Matrix4 light_view_matrix = make_translation_matrix(&camera.position) * camera.rotation_matrix
			//Matrix4 light_view_matrix = light_matrix;
			Matrix4 light_view_matrix = (rotate_about_x(degress_to_radians(45)) * rotate_about_y(degress_to_radians(90))) * inverse(&render_camera.view_matrix);
			//Matrix4 light_view_matrix = make_identity_matrix();
			//Matrix4 light_view_matrix = XMMatrixInverse(NULL, camera.get_view_matrix());
			cascaded_shadow_maps[i].cascaded_shadows[j].transform(&light_view_matrix);
			world_matrices[cascaded_shadow->matrix_index] = light_view_matrix;


			cascaded_view_projection_matrices[cascaded_shadow->view_projection_matrix_index] = cascaded_shadow->get_cascade_view_matrix() * cascaded_shadow->get_cascade_projection_matrix();
		}
	}

	world_matrices_struct_buffer.update(&world_matrices);

	cascaded_view_projection_matrices_sb.update(&cascaded_view_projection_matrices);

	end = milliseconds_counter();
}

bool Render_World::add_mesh(const char *mesh_name, Mesh<Vertex_XNUV> *mesh, Mesh_Idx *mesh_idx)
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
}

bool Render_World::get_shadow_atls_viewport(Viewport *viewport)
{
	static u32 x = 0;
	static u32 y = 0;
	
	Point_u32 shadow_map_position;
	shadow_map_position.x = CASCADE_WIDTH + x;
	shadow_map_position.y = CASCADE_HEIGHT + y;

	if ((shadow_map_position.x <= SHADOW_ATLAS_WIDTH) && (shadow_map_position.y <= SHADOW_ATLAS_HEIGHT)) {
		viewport->x = x;
		viewport->y = y;
		viewport->width = CASCADE_WIDTH;
		viewport->height = CASCADE_HEIGHT;
		x += CASCADE_WIDTH;
	} else if ((shadow_map_position.x > SHADOW_ATLAS_WIDTH) && (shadow_map_position.y < SHADOW_ATLAS_HEIGHT)) {
		viewport->x = 0;
		viewport->y = y;
		viewport->width = CASCADE_WIDTH;
		viewport->height = CASCADE_HEIGHT;
		x = 0;
		y += CASCADE_HEIGHT;
	} else {
		print("Render_World::get_shadow_atls_view_port: The shadow atlas is out of space.");
		return false;
	}
	return true;
}

void Render_World::render()
{
	render_sys->render_pipeline.update_constant_buffer(&frame_info_cbuffer, (void *)&frame_info);

	render_sys->render_pipeline.set_vertex_shader_resource(1, frame_info_cbuffer);
	render_sys->render_pipeline.set_pixel_shader_resource(1, frame_info_cbuffer);
	
	Render_Pass *render_pass = NULL;
	For(render_passes_array, render_pass) {
		render_pass->render(&render_sys->render_pipeline);
	}
}

template<typename T>
void Gpu_Struct_Buffer::allocate(u32 elements_count)
{	
	Gpu_Buffer_Desc desc;
	desc.usage = RESOURCE_USAGE_DYNAMIC;
	desc.data = NULL;
	desc.data_size = sizeof(T);
	desc.struct_size = sizeof(T);
	desc.data_count = elements_count;
	desc.bind_flags = BIND_SHADER_RESOURCE;
	desc.cpu_access = CPU_ACCESS_WRITE;
	desc.misc_flags = RESOURCE_MISC_BUFFER_STRUCTURED;

	Gpu_Device *gpu_device = &Engine::get_render_system()->gpu_device;
	gpu_device->create_gpu_buffer(&desc, &gpu_buffer);
	gpu_device->create_shader_resource_view(&gpu_buffer);
}

template<typename T>
void Gpu_Struct_Buffer::update(Array<T> *array)
{
	if (array->count == 0) {
		return;
	}

	Render_Pipeline *render_pipeline = &Engine::get_render_system()->render_pipeline;
	
	if (array->count > gpu_buffer.data_count) {
		free();
		allocate<T>(array->count);
	}

	T *buffer = (T *)render_pipeline->map(gpu_buffer);
	memcpy((void *)buffer, (void *)array->items, sizeof(T) * array->count);
	render_pipeline->unmap(gpu_buffer);
}

void Gpu_Struct_Buffer::free()
{
	if (!gpu_buffer.is_empty()) {
		gpu_buffer.free();
	}
}

void Render_Camera::update(Camera *camera)
{
	view_matrix = make_view_matrix(&camera->position, &camera->target);
}

bool Render_Camera::is_entity_camera_set()
{
	if (camera_id.type == ENTITY_TYPE_CAMERA) {
		return true;
	}
	return false;
}
