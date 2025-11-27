#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "render_world.h"

#include "../sys/sys.h"
#include "../sys/engine.h"

#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/memory/base.h"
#include "../libs/math/functions.h"

#include "render_api\command.h"

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

void Rendering_View::update(Game_World *game_world)
{
	Camera *camera = game_world->get_camera(camera_id);
	position = camera->position;
	direction = normalize(camera->target - camera->position);
	view_matrix = make_look_at_matrix(position, camera->target);
	inverse_view_matrix = inverse(&view_matrix);
}

bool Rendering_View::is_entity_camera_set()
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

Texture *create_texture_from_image(Image *image);

void Model_Storage::init()
{
	u32 width = 200;
	u32 height = 200;

	Image color_buffer;
	color_buffer.allocate_memory(width, height, DXGI_FORMAT_R8G8B8A8_UNORM);

	color_buffer.fill(Color(0.0f, 0.0f, 0.0f));
	default_textures.normal = create_texture_from_image(&color_buffer);

	color_buffer.fill(DEFAULT_MESH_COLOR);
	default_textures.diffuse = create_texture_from_image(&color_buffer);
	
	color_buffer.fill(Color(0.01f, 0.01f, 0.01f));
	//color_buffer.fill(Color(0.2f, 0.2f, 0.2f));
	default_textures.specular = create_texture_from_image(&color_buffer);
	
	color_buffer.fill(Color(0.0f, 0.0f, 0.0f));
	default_textures.displacement = create_texture_from_image(&color_buffer);
	
	color_buffer.fill(Color::White);
	default_textures.white = create_texture_from_image(&color_buffer);
	
	color_buffer.fill(Color::Black);
	default_textures.black = create_texture_from_image(&color_buffer);
	
	color_buffer.fill(Color(0.5f, 0.5f, 1.0f));
	default_textures.green = create_texture_from_image(&color_buffer);
}

void Model_Storage::release_all_resources()
{
	//textures.clear();

	//vertex_struct_buffer.free();
	//index_struct_buffer.free();
	//mesh_struct_buffer.free();
}

bool validate_model_name_and_get_string_id(String_Id *model_string_id, Loading_Model *model)
{
	if (!model->name.is_empty() && !model->file_name.is_empty()) {
		*model_string_id = fast_hash(model->file_name + "_" + model->name);
		return true;
	} else if (exclusive_or(model->name.is_empty(), model->file_name.is_empty())) {
		if (model->name.is_empty()) {
			*model_string_id = fast_hash(model->file_name);
			print("[Mesh storage] Warning: A tringle mesh contains only a file name '{}' it's possible to get the collision.", model->file_name);
		} else {
			*model_string_id = fast_hash(model->name);
			print("[Mesh storage] Warning: A tringle mesh contains only a name '{}' it's possible to get the collision.", model->name);
		}
		return true;
	}
	print("[Mesh storage] Error: Not possible to add a triangle mesh to the storage. the triangle mesh doesn't has a file name and mesh name.");
	return false;
}

void move(Triangle_Mesh *dest, Triangle_Mesh *source)
{
	if (!source->empty()) {
		if (!dest->vertices.is_empty()) { DELETE_ARRAY(dest->vertices.items); };
		if (!dest->indices.is_empty()) { DELETE_ARRAY(dest->indices.items); };

		dest->vertices.items = source->vertices.items;
		dest->vertices.count = source->vertices.count;
		dest->vertices.size = source->vertices.size;

		dest->indices.items = source->indices.items;
		dest->indices.count = source->indices.count;
		dest->indices.size = source->indices.size;

		source->vertices.items = NULL;
		source->vertices.count = 0;
		source->vertices.size = 0;
		
		source->indices.items = NULL;
		source->indices.count = 0;
		source->indices.size = 0;
	}
}

void Model_Storage::add_models(Array<Loading_Model *> &models, Array<Pair<Loading_Model *, u32>> &result)
{
	result.resize(models.count);

	for (u32 i = 0; i < models.count; i++) {
		Loading_Model *loading_model = models[i];

		String_Id model_string_id;
		if (!validate_model_name_and_get_string_id(&model_string_id, loading_model)) {
			continue;
		}

		if (loading_model->mesh.empty()) {
			print("Render_World::add_mesh: {} mesh can be added because doesn't have all necessary data.", loading_model->get_pretty_name());
			continue;
		}

		Pair<Render_Model *, u32> *temp = NULL;
		if (render_models_table.get(model_string_id, temp)) {
			print("[Mesh storage] Info: {} mesh has already been placed in the mesh storage.", loading_model->get_pretty_name());
			result.push({ loading_model, temp->second });
			continue;
		}

		Render_Model *render_model = new Render_Model();
		render_model->name = loading_model->name;
		render_model->file_name = loading_model->file_name;
		render_model->normal_texture = find_texture_or_get_default(loading_model->normal_texture_name, loading_model->file_name, default_textures.normal);
		render_model->diffuse_texture = find_texture_or_get_default(loading_model->diffuse_texture_name, loading_model->file_name, default_textures.diffuse);
		render_model->specular_texture = find_texture_or_get_default(loading_model->specular_texture_name, loading_model->file_name, default_textures.specular);
		render_model->displacement_texture = find_texture_or_get_default(loading_model->displacement_texture_name, loading_model->file_name, default_textures.displacement);
		move(&render_model->mesh, &loading_model->mesh);

		u32 mesh_instance_index = render_models.push(render_model);
		render_models_table.set(model_string_id, { render_model, mesh_instance_index });
		
		result.push({ loading_model, mesh_instance_index });
	}
	upload_models_in_gpu();
}

void Model_Storage::upload_models_in_gpu()
{
	Gpu_Device gpu_device = Engine::get_render_system()->gpu_device;
	Render_System *render_sys = Engine::get_render_system();

	render_sys->flush();

	//auto upload_command_list = &render_sys->upload_command_list;

	Array<Vertex_PNTUV> unified_vertex_list;
	Array<u32> unified_index_list;
	Array<Mesh_Instance> unified_mesh_instances_list;

	u32 vertex_count = 0;
	u32 index_count = 0;
	for (u32 i = 0; i < render_models.count; i++) {
		vertex_count += render_models[i]->mesh.vertex_count();
		index_count += render_models[i]->mesh.index_count();
	}

	unified_vertex_list.resize(vertex_count);
	unified_index_list.resize(index_count);
	unified_mesh_instances_list.resize(render_models.count);

	u32 vertex_offset = 0;
	u32 index_offset = 0;
	for (u32 i = 0; i < render_models.count; i++) {
		merge(&unified_vertex_list, &render_models[i]->mesh.vertices);
		merge(&unified_index_list, &render_models[i]->mesh.indices);

		GPU_Material material;
		material.normal_idx = render_models[i]->normal_texture->get_shader_resource_descriptor().index;
		material.diffuse_idx = render_models[i]->diffuse_texture->get_shader_resource_descriptor().index;
		material.specular_idx = render_models[i]->specular_texture->get_shader_resource_descriptor().index;
		material.displacement_idx = render_models[i]->displacement_texture->get_shader_resource_descriptor().index;

		Mesh_Instance mesh_instance;
		mesh_instance.vertex_count = render_models[i]->mesh.vertex_count();
		mesh_instance.vertex_offset = vertex_offset;
		mesh_instance.index_count = render_models[i]->mesh.index_count();
		mesh_instance.index_offset = index_offset;
		mesh_instance.material = material;
		
		unified_mesh_instances_list.push(mesh_instance);
		
		vertex_offset += render_models[i]->mesh.vertex_count();
		index_offset += render_models[i]->mesh.index_count();
	}

	if (!unified_vertex_buffer || (unified_vertex_buffer->get_size() < unified_vertex_list.get_size())) {
		DELETE_PTR(unified_vertex_buffer);
		Buffer_Desc buffer_desc = Buffer_Desc(unified_vertex_list.count, unified_vertex_list.get_stride());
		unified_vertex_buffer = render_sys->resource_manager.create_buffer(BUFFER_TYPE_DEFAULT, &buffer_desc);
		unified_vertex_buffer->set_debug_name("Unified vertex buffer");
		unified_vertex_buffer->write((void *)unified_vertex_list.items, unified_vertex_list.get_size());
	}

	if (!unified_index_buffer || (unified_index_buffer->get_size() < unified_index_buffer->get_size())) {
		DELETE_PTR(unified_index_buffer);
		Buffer_Desc buffer_desc = Buffer_Desc(unified_index_list.count, unified_index_list.get_stride());
		unified_index_buffer = render_sys->resource_manager.create_buffer(BUFFER_TYPE_DEFAULT, &buffer_desc);
		unified_index_buffer->set_debug_name("Unified index buffer");
		unified_index_buffer->write((void *)unified_index_list.items, unified_index_list.get_size());
	}

	if (!mesh_instance_buffer || (mesh_instance_buffer->get_size() < mesh_instance_buffer->get_size())) {
		DELETE_PTR(mesh_instance_buffer);
		Buffer_Desc buffer_desc = Buffer_Desc(unified_mesh_instances_list.count, unified_mesh_instances_list.get_stride());
		mesh_instance_buffer = render_sys->resource_manager.create_buffer(BUFFER_TYPE_DEFAULT, &buffer_desc);
		mesh_instance_buffer->set_debug_name("Mesh instance buffer");
		mesh_instance_buffer->write((void *)unified_mesh_instances_list.items, unified_mesh_instances_list.get_size());
	}
}

template <typename T>
void upload_data_in_gpu_buffer(Array<T> &array, GPU_Buffer *buffer)
{
	if (array.is_empty()) {
		return;
	}
	Gpu_Device &gpu_device = Engine::get_render_system()->gpu_device;
	Render_System *render_sys = Engine::get_render_system();

	GPU_Buffer temp_buffer;
	temp_buffer.create(gpu_device, GPU_HEAP_TYPE_UPLOAD, RESOURCE_STATE_GENERIC_READ, Buffer_Desc(array.count, array.get_stride()));

	u8 *ptr = temp_buffer.map();
	memcpy((void *)ptr, (void *)array.items, array.get_size());
	temp_buffer.unmap();

	u32 x = buffer->release();
	render_sys->descriptors_pool.free(&buffer->sr_descriptor);

	buffer->create(gpu_device, GPU_HEAP_TYPE_DEFAULT, RESOURCE_STATE_COMMON, Buffer_Desc(array.count, array.get_stride()));

	buffer->sr_descriptor = render_sys->descriptors_pool.allocate_sr_descriptor(buffer);

	Copy_Command_List upload_command_list;
	upload_command_list.create(gpu_device, 1);
	upload_command_list.set_debug_name("Copy buffer command list");
	upload_command_list.reset(0);

	upload_command_list.resource_barrier(Transition_Resource_Barrier(*buffer, RESOURCE_STATE_COMMON, RESOURCE_STATE_COPY_DEST));
	upload_command_list.copy_resources(*buffer, temp_buffer);
	upload_command_list.resource_barrier(Transition_Resource_Barrier(*buffer, RESOURCE_STATE_COPY_DEST, RESOURCE_STATE_COMMON));
	upload_command_list.close();
	
	render_sys->copy_queue.execute_command_list(upload_command_list);
	render_sys->copy_queue.flush_gpu();

	Graphics_Command_List graphics_command_list;
	graphics_command_list.create(gpu_device, 1);
	graphics_command_list.set_debug_name("Graphics bufffer command list");
	graphics_command_list.reset(0);
	graphics_command_list.resource_barrier(Transition_Resource_Barrier(*buffer, RESOURCE_STATE_COMMON, RESOURCE_STATE_ALL_SHADER_RESOURCE));
	graphics_command_list.close();

	Fence fence2;
	fence2.create(render_sys->gpu_device);
	u64 fence_value2 = 0;

	render_sys->graphics_queue.execute_command_list(graphics_command_list);
	auto new_fence_value2 = render_sys->graphics_queue.signal(fence_value2, fence2);
	fence2.wait_for_gpu(new_fence_value2);
	//render_sys->graphics_queue.flush_gpu();
}

inline u32 find_max_mip_level(u32 width, u32 height)
{
	return math::log2(math::max(width, height));
}

Texture *create_texture_from_buffer(Texture_Desc *texture_desc, GPU_Buffer *buffer)
{
	Gpu_Device &gpu_device = Engine::get_render_system()->gpu_device;
	Render_System *render_sys = Engine::get_render_system();
	Resource_Manager *resource_manager = &render_sys->resource_manager;

	Texture *texture = resource_manager->create_texture(texture_desc);

	Copy_Command_List upload_command_list;
	upload_command_list.create(gpu_device, 1);
	upload_command_list.reset(0);

	Subresource_Footprint subresource_footprint = texture->get_subresource_footprint(0);
	upload_command_list.copy_buffer_to_texture(*texture, *buffer, subresource_footprint);
	upload_command_list.resource_barrier(Transition_Resource_Barrier(*texture, RESOURCE_STATE_COPY_DEST, RESOURCE_STATE_COMMON));

	Fence fence;
	fence.create(render_sys->gpu_device, 1);

	upload_command_list.close();
	render_sys->copy_queue.execute_command_list(upload_command_list);
	render_sys->copy_queue.signal(fence);
	fence.wait_for_gpu();

	return texture;
}

template <typename T>
void upload_bitmap_in_buffer(u32 width, u32 height, u32 depth, T *data, DXGI_FORMAT format, GPU_Buffer *buffer)
{
	assert(data);
	assert((width > 0) && (height > 0) && (depth > 0));

	u32 row_pitch = width * dxgi_format_size(format);
	u32 aligned_row_pitch = align_address<u32>(row_pitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	assert(buffer->get_size() >= align_address<u32>(aligned_row_pitch * height * depth, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT));

	u8 *pointer = buffer->map();
	for (u32 z = 0; z < depth; z++) {
		for (u32 y = 0; y < height; y++) {
			u8 *buffer_row = pointer + y * aligned_row_pitch + (z * aligned_row_pitch * height);
			u8 *bitmap_row = (u8 *)data + y * row_pitch + (z * row_pitch * height);
			memcpy((void *)buffer_row, (void *)bitmap_row, row_pitch);
		}
	}
	buffer->unmap();
}

void upload_bitmap_in_buffer(u32 width, u32 height, u8 *data, DXGI_FORMAT format, GPU_Buffer &buffer)
{
	assert(data);
	assert((width > 0) && (height > 0));

	u32 row_pitch = width * dxgi_format_size(format);
	u32 aligned_row_pitch = align_address<u32>(row_pitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	u8 *pointer = buffer.map();
	for (u32 y = 0; y < height; y++) {
		u8 *buffer_row = pointer + y * aligned_row_pitch;
		u8 *bitmap_row = data + y * row_pitch;
		memcpy((void *)buffer_row, (void *)bitmap_row, row_pitch);
	}
	buffer.unmap();
}

Texture *create_texture_from_image(Image *image)
{
	if (!image->valid()) {
		return NULL;
	}
	
	Gpu_Device &gpu_device = Engine::get_render_system()->gpu_device;
	Render_System *render_sys = Engine::get_render_system();
	Resource_Manager *resource_manager = &render_sys->resource_manager;

	Texture_Desc texture_desc;
	texture_desc.name = "Temp texture";
	texture_desc.dimension = TEXTURE_DIMENSION_2D;
	texture_desc.width = image->width;
	texture_desc.height = image->height;
	texture_desc.format = image->format;
	texture_desc.flags = ALLOW_UNORDERED_ACCESS;
	texture_desc.miplevels = find_max_mip_level(image->width, image->height);
	texture_desc.resource_state = RESOURCE_STATE_COMMON;

	Texture *texture = resource_manager->create_texture(&texture_desc);

	GPU_Buffer temp_buffer;
	temp_buffer.create(gpu_device, GPU_HEAP_TYPE_UPLOAD, RESOURCE_STATE_GENERIC_READ, Buffer_Desc(texture->get_size()));
	temp_buffer.set_debug_name("temp buffer");
	upload_bitmap_in_buffer(image->width, image->height, image->data, image->format, temp_buffer);

	Copy_Command_List upload_command_list;
	upload_command_list.create(gpu_device, 1);
	upload_command_list.reset(0);

	Subresource_Footprint subresource_footprint = texture->get_subresource_footprint(0);
	upload_command_list.copy_buffer_to_texture(*texture, temp_buffer, subresource_footprint);
	//upload_command_list.resource_barrier(Transition_Resource_Barrier(*texture, RESOURCE_STATE_COPY_DEST, RESOURCE_STATE_COMMON));
	//upload_command_list.resource_barrier(Transition_Resource_Barrier(*texture, RESOURCE_STATE_COMMON, RESOURCE_STATE_ALL_SHADER_RESOURCE));

	Fence fence;
	fence.create(render_sys->gpu_device, 1);

	upload_command_list.close();
	render_sys->copy_queue.execute_command_list(upload_command_list);
	render_sys->copy_queue.signal(fence);
	fence.wait_for_gpu();

	return texture;
}

Texture *Model_Storage::create_texture_from_file(const char *full_path_to_texture)
{
	Gpu_Device &gpu_device = Engine::get_render_system()->gpu_device;
	Render_System *render_sys = Engine::get_render_system();
	Resource_Manager *resource_manager = &render_sys->resource_manager;

	Image image;
	if (load_image_from_file(full_path_to_texture, DXGI_FORMAT_R8G8B8A8_UNORM, &image)) {
		Texture_Desc texture_desc;
		extract_file_name(full_path_to_texture, texture_desc.name);
		texture_desc.dimension = TEXTURE_DIMENSION_2D;
		texture_desc.width = image.width;
		texture_desc.height = image.height;
		texture_desc.format = image.format;
		texture_desc.flags = ALLOW_UNORDERED_ACCESS;
		texture_desc.miplevels = find_max_mip_level(image.width, image.height);
		//texture_desc.resource_state = RESOURCE_STATE_COPY_DEST;
		texture_desc.resource_state = RESOURCE_STATE_COMMON;
		
		Texture *texture = resource_manager->create_texture(&texture_desc);

		GPU_Buffer temp_buffer;
		temp_buffer.create(gpu_device, GPU_HEAP_TYPE_UPLOAD, RESOURCE_STATE_GENERIC_READ, Buffer_Desc(texture->get_size()));
		upload_bitmap_in_buffer(image.width, image.height, image.data, image.format, temp_buffer);

		Copy_Command_List upload_command_list;
		upload_command_list.create(gpu_device, 1);
		upload_command_list.reset(0);

		Subresource_Footprint subresource_footprint = texture->get_subresource_footprint(0);
		upload_command_list.copy_buffer_to_texture(*texture, temp_buffer, subresource_footprint);
		//upload_command_list.resource_barrier(Transition_Resource_Barrier(*texture, RESOURCE_STATE_COPY_DEST, RESOURCE_STATE_COMMON));

		Fence fence;
		fence.create(render_sys->gpu_device, 1);

		upload_command_list.close();
		render_sys->copy_queue.execute_command_list(upload_command_list);
		render_sys->copy_queue.signal(fence);
		fence.wait_for_gpu();

		Compute_Command_List compute_command_list;
		compute_command_list.create(render_sys->gpu_device, 1);
		compute_command_list.reset(0);

		Array<Texture *> textures;
		textures.push(texture);
		render_sys->generate_mipmaps.generate(&compute_command_list, textures, render_sys);

		fence.increment_expected_value();

		compute_command_list.close();
		render_sys->compute_queue.execute_command_list(compute_command_list);
		render_sys->compute_queue.signal(fence);
		fence.wait_for_gpu();
		
		return texture;
	}
	return NULL;
}

Texture *Model_Storage::find_texture_or_get_default(String &texture_file_name, String &mesh_file_name, Texture *default_texture)
{
	if (!texture_file_name.is_empty()) {
		Texture *texture = NULL;
		String_Id texture_string_id = fast_hash(texture_file_name);

		if (textures_table.get(texture_string_id, texture)) {
			return texture;
		}
		Array<String> paths;
		paths.reserve(2);

		String base_file_name;
		extract_base_file_name(mesh_file_name, base_file_name);
		build_full_path_to_texture_file(texture_file_name, base_file_name, paths[0]);
		build_full_path_to_texture_file(texture_file_name, paths[1]);

		for (u32 i = 0; i < paths.count; i++) {
			Texture *texture = create_texture_from_file(paths[i]);
			if (texture) {
				return texture;
			}
		}
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

Render_World::Render_World()
{
}

Render_World::~Render_World()
{
}

void Render_World::init(Engine *engine)
{
	//@Note: Why don't pass just the engine pointer ?
	game_world = &engine->game_world;
	render_sys = &engine->render_sys;

	model_storage.init();

	u32 x = 128;
	voxel_grid.grid_size = { x, x, x };
	u32 y = 20;
	voxel_grid.ceil_size = { y, y, y };

//	voxels_sb.allocate<Voxel>(voxel_grid.grid_size.find_area());

	Size_f32 grid_size = voxel_grid.total_size();
	float grid_depth = grid_size.depth;
	grid_size *= 0.5f;

	voxel_matrix = XMMatrixOrthographicOffCenterLH(-grid_size.width, grid_size.width, -grid_size.height, grid_size.height, 1.0f, grid_depth + 1.0f);

	if (!rendering_view.is_entity_camera_set()) {
		error("Render Camera was not initialized. There is no a view for rendering.");
	}

	shadow_cascade_ranges.push({ 1, 15 });
	shadow_cascade_ranges.push({ 15, 150 });
	shadow_cascade_ranges.push({ 150, 500 });
	shadow_cascade_ranges.push({ 500, 1000 });
	shadow_cascade_ranges.push({ 1000, 5000 });

	jittering_tile_size = 16;
	jittering_filter_size = 8;
	jittering_scaling = jittering_filter_size / 2;

	Array<Vector2> jittered_samples;
	make_jittering_sampling_filters(jittering_tile_size, jittering_filter_size, jittered_samples);

	Texture_Desc jittering_samples_texture_desc;
	jittering_samples_texture_desc.dimension = TEXTURE_DIMENSION_3D;
	jittering_samples_texture_desc.width = math::pow2(jittering_filter_size);
	jittering_samples_texture_desc.height = jittering_tile_size;
	jittering_samples_texture_desc.depth = jittering_tile_size;
	jittering_samples_texture_desc.format = DXGI_FORMAT_R32G32_FLOAT;
	jittering_samples_texture_desc.miplevels = 1;
	//jittering_samples_texture_desc.resource_state = RESOURCE_STATE_COPY_DEST;
	jittering_samples_texture_desc.resource_state = RESOURCE_STATE_COMMON;

	jittering_samples.create(&jittering_samples_texture_desc, &render_sys->resource_manager.resource_allocator, &render_sys->descriptors_pool);

	GPU_Buffer temp_buffer;
	temp_buffer.create(render_sys->gpu_device, GPU_HEAP_TYPE_UPLOAD, RESOURCE_STATE_GENERIC_READ, Buffer_Desc(jittering_samples.get_size()));
	upload_bitmap_in_buffer(math::pow2(jittering_filter_size), jittering_tile_size, jittering_tile_size, jittered_samples.items, DXGI_FORMAT_R32G32_FLOAT, &temp_buffer);

	Copy_Command_List upload_command_list;
	upload_command_list.create(render_sys->gpu_device, 1);
	upload_command_list.reset(0);

	Subresource_Footprint subresource_footprint = jittering_samples.get_subresource_footprint(0);
	upload_command_list.copy_buffer_to_texture(jittering_samples, temp_buffer, subresource_footprint);
	//upload_command_list.resource_barrier(Transition_Resource_Barrier(jittering_samples, RESOURCE_STATE_COPY_DEST, RESOURCE_STATE_COMMON));

	Fence fence;
	fence.create(render_sys->gpu_device, 1);

	upload_command_list.close();
	render_sys->copy_queue.execute_command_list(upload_command_list);
	render_sys->copy_queue.signal(fence);
	fence.wait_for_gpu();
}

void Render_World::release_all_resources()
{
	release_render_entities_resources();
	shadow_cascade_ranges.clear();
}

void Render_World::release_render_entities_resources()
{
	cascaded_shadows_list.clear();
	cascaded_shadows_info_list.clear();
	shadow_cascade_ranges.clear();
	lights.clear();

	render_entity_world_matrices.clear();
	cascaded_view_projection_matrices.clear();

	game_render_entities.clear();

	cascaded_shadows_list.clear();
	cascaded_shadows_info_list.clear();

	model_storage.release_all_resources();
}

void Render_World::update()
{
	rendering_view.update(game_world);
	update_render_entities();
	update_shadows();
	//update_global_illumination();
}

void Render_World::update_render_entities()
{
	Render_Entity *render_entity = NULL;
	For(game_render_entities, render_entity) {
		Entity *entity = game_world->get_entity(render_entity->entity_id);
		render_entity_world_matrices[render_entity->world_matrix_idx] = get_world_matrix(entity);
	}

	if (!world_matrices_buffer || (world_matrices_buffer->get_size() < render_entity_world_matrices.get_size())) {
		DELETE_PTR(world_matrices_buffer);
		Buffer_Desc buffer_desc = Buffer_Desc(render_entity_world_matrices.count, render_entity_world_matrices.get_stride());
		world_matrices_buffer = render_sys->resource_manager.create_buffer(BUFFER_TYPE_UPLOAD, &buffer_desc);
		world_matrices_buffer->set_debug_name("World matrices");
	}
	world_matrices_buffer->write((void *)render_entity_world_matrices.items, render_entity_world_matrices.get_size());
}

void Render_World::update_global_illumination()
{
	Vector3 voxel_ceil_size = voxel_grid.ceil_size.to_vector3();
	Vector3 voxel_grid_size = voxel_grid.total_size().to_vector3() * 0.5f; // Holdes the half of a total voxel grid size.

	Camera *camera = game_world->get_camera(rendering_view.camera_id);
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

void Render_World::upload_lights()
{
	lights.reset();
	cascaded_shadows_list.reset();
	cascaded_shadows_info_list.reset();

	Light *light = NULL;
	For(game_world->lights, light) {
		if (light->type == DIRECTIONAL_LIGHT_TYPE) {
			Cascaded_Shadows cascaded_shadows;
			cascaded_shadows.light_direction = light->direction;

			bool shadow_atlas_has_space = true;
			for (u32 i = 0; i < shadow_cascade_ranges.count; i++) {
				Cascaded_Shadow_Map cascaded_shadow_map;
				cascaded_shadow_map.view_projection_matrix_index = cascaded_view_projection_matrices.push(Matrix4());
				cascaded_shadow_map.init(render_sys->window_view_plane.fov, render_sys->window_view_plane.ratio, &shadow_cascade_ranges[i]);

				if (!get_shadow_atls_viewport(&cascaded_shadow_map.viewport)) {
					shadow_atlas_has_space = false;
					break;
				}
				cascaded_shadows.cascaded_shadow_maps.push(cascaded_shadow_map);
			}

			if (shadow_atlas_has_space) {
				GPU_Light gpu_light;
				gpu_light.position = light->position;
				gpu_light.direction = normalize(&light->direction);
				gpu_light.color = light->color;
				gpu_light.radius = light->radius;
				gpu_light.range = light->range;
				gpu_light.light_type = light->light_type;
				lights.push(gpu_light);

				cascaded_shadows_list.push(cascaded_shadows);

				GPU_Cascaded_Shadows_Info cascaded_shadows_info;
				cascaded_shadows_info.light_direction = light->direction;
				cascaded_shadows_info.shadow_map_start_index = cascaded_shadows_info_list.count * shadow_cascade_ranges.count;
				cascaded_shadows_info.shadow_map_end_index = cascaded_shadows_info_list.count * shadow_cascade_ranges.count + (shadow_cascade_ranges.count - 1);
				cascaded_shadows_info_list.push(cascaded_shadows_info);
			}
		}
	}
	if (!lights_buffer || (lights_buffer->get_size() < lights.get_size())) {
		DELETE_PTR(lights_buffer);
		Buffer_Desc buffer_desc = Buffer_Desc(lights.count, lights.get_stride());
		lights_buffer = render_sys->resource_manager.create_buffer(BUFFER_TYPE_DEFAULT, &buffer_desc);
		lights_buffer->set_debug_name("Lights");
	}
	lights_buffer->write((void *)lights.items, lights.get_size());

	if (!cascaded_shadows_info_buffer || (cascaded_shadows_info_buffer->get_size() < cascaded_shadows_info_list.get_size())) {
		DELETE_PTR(cascaded_shadows_info_buffer);
		Buffer_Desc buffer_desc = Buffer_Desc(cascaded_shadows_info_list.count, cascaded_shadows_info_list.get_stride());
		cascaded_shadows_info_buffer = render_sys->resource_manager.create_buffer(BUFFER_TYPE_DEFAULT, &buffer_desc);
		cascaded_shadows_info_buffer->set_debug_name("Cascaded shadows info");
	}
	cascaded_shadows_info_buffer->write((void *)cascaded_shadows_info_list.items, cascaded_shadows_info_list.get_size());
}

void Render_World::add_render_entity(Entity_Id entity_id, u32 mesh_idx, void *args)
{
	Render_Entity render_entity;
	render_entity.entity_id = entity_id;
	render_entity.mesh_idx = mesh_idx;
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

template <typename T>
void write_to_upload_buffer(const char *buffer_name, Buffer **buffer, Array<T> &array)
{
	Render_System *render_sys = Engine::get_render_system();

	if (!*buffer || ((*buffer)->get_size() < array.get_size())) {
		DELETE_PTR(*buffer);
		Buffer_Desc buffer_desc = Buffer_Desc(array.count, array.get_stride());
		(*buffer) = render_sys->resource_manager.create_buffer(BUFFER_TYPE_UPLOAD, &buffer_desc);
		(*buffer)->set_debug_name(buffer_name);
	}
	(*buffer)->write((void *)array.items, array.get_size());
}

void Render_World::update_shadows()
{
	for (u32 i = 0; i < cascaded_shadows_list.count; i++) {
		Vector3 light_direction = cascaded_shadows_list[i].light_direction;

		for (u32 j = 0; j < cascaded_shadows_list[i].cascaded_shadow_maps.count; j++) {
			Cascaded_Shadow_Map *cascaded_shadow_map = &cascaded_shadows_list[i].cascaded_shadow_maps[j];

			Vector3 view_position = cascaded_shadow_map->view_position * rendering_view.inverse_view_matrix;
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

	if (!casded_view_projection_matrices_buffer || (casded_view_projection_matrices_buffer->get_size() < cascaded_view_projection_matrices.get_size())) {
		DELETE_PTR(casded_view_projection_matrices_buffer);
		Buffer_Desc buffer_desc = Buffer_Desc(cascaded_view_projection_matrices.count, cascaded_view_projection_matrices.get_stride());
		casded_view_projection_matrices_buffer = render_sys->resource_manager.create_buffer(BUFFER_TYPE_UPLOAD, &buffer_desc);
		casded_view_projection_matrices_buffer->set_debug_name("View projection shadow matrices");
	}
	casded_view_projection_matrices_buffer->write((void *)cascaded_view_projection_matrices.items, cascaded_view_projection_matrices.get_size());
}

void Render_World::set_rendering_view(Entity_Id camera_id)
{
	if (camera_id.type != ENTITY_TYPE_CAMERA) {
		print("Render_World::set_camera_for_rendering: Passed an camera id is not entity camera type.");
		return;
	}
	rendering_view.camera_id = camera_id;
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

Model_Storage *Render_World::get_model_storage()
{
	return &model_storage;
}