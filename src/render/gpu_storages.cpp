#include "helpers.h"
#include "gpu_storages.h"
#include "render_system.h"
#include "d3d12_render_api/d3d12_functions.h"

#include "../sys/engine.h"
#include "../libs/os/file.h"
#include "../libs/os/path.h"
#include "../libs/image/image.h"

void resolve_texture_file_path(const char *texture_file_name, const char *textures_subdirectory, String &full_path_to_texture)
{
	if (texture_file_name && textures_subdirectory) {
		build_full_path_to_texture_file(texture_file_name, textures_subdirectory, full_path_to_texture);
	} else if (texture_file_name) {
		build_full_path_to_texture_file(texture_file_name, full_path_to_texture);
	}
}

void Texture_Storage::init(Render_System *_render_sys)
{
	render_sys = _render_sys;

	u32 width = 256;
	u32 height = 256;

	Image color_buffer;
	color_buffer.create(width, height, DXGI_FORMAT_R8G8B8A8_UNORM);

	color_buffer.name = DEFAULT_NORMAL_TEXTURE;
	color_buffer.fill(Color(0.0f, 0.0f, 1.0f));
	textures_table.set(DEFAULT_NORMAL_TEXTURE, create_texture_from_image(&color_buffer));

	color_buffer.name = DEFAULT_ALBEDO_TEXTURE;
	color_buffer.fill(Color(242, 242, 242));
	textures_table.set(DEFAULT_ALBEDO_TEXTURE, create_texture_from_image(&color_buffer));

	color_buffer.name = DEFAULT_ROUGHNESS_METALIC_TEXTURE;
	color_buffer.fill(Color(0.2f, 0.2f, 0.01f));
	textures_table.set(DEFAULT_ROUGHNESS_METALIC_TEXTURE, create_texture_from_image(&color_buffer));

	//color_buffer.fill(Color::White);
	//default_textures.white = create_texture_from_image(&color_buffer);

	//color_buffer.fill(Color::Black);
	//default_textures.black = create_texture_from_image(&color_buffer);

	//color_buffer.fill(Color(0.5f, 0.5f, 1.0f));
	//default_textures.green = create_texture_from_image(&color_buffer);
}

void Texture_Storage::create_textures(Array<String> &texture_file_names, const char *textures_subdirectory)
{
	if (texture_file_names.is_empty()) {
		return;
	}
	Render_Device *render_device = render_sys->render_device;

	Fence *copy_fence = render_device->create_fence(1, "Textures copy fence");

	Copy_Command_List *copy_command_list = (Copy_Command_List *)render_sys->command_list_allocator.allocate_command_list(COMMAND_LIST_TYPE_COPY);

	u32 textures_number = texture_file_names.count;
	u32 textures_uploading_chunk = 100;
	u32 uploaded_textures_number = 0;

	assert(textures_uploading_chunk > 0);

	Array<Buffer *> staging_buffers;
	staging_buffers.reserve(math::min(textures_number, textures_uploading_chunk));
	zero_memory(&staging_buffers);

	while (uploaded_textures_number < textures_number) {
		copy_command_list->reset();
		u32 textures_uploading_number = math::min(textures_number - uploaded_textures_number, textures_uploading_chunk);
		for (u32 j = 0; j < textures_uploading_number; j++) {
			u32 texture_name_index = uploaded_textures_number + j;
			String &texture_name = texture_file_names[texture_name_index];
			if (textures_table.key_in_table(texture_name)) {
				continue;
			}

			String full_path_to_texture;
			resolve_texture_file_path(texture_name, textures_subdirectory, full_path_to_texture);

			Image image;
			if (load_image_from_file(full_path_to_texture, DXGI_FORMAT_R8G8B8A8_UNORM, &image)) {
				Texture_Desc texture_desc;
				extract_file_name(full_path_to_texture, texture_desc.name);
				texture_desc.dimension = TEXTURE_DIMENSION_2D;
				texture_desc.width = image.width;
				texture_desc.height = image.height;
				texture_desc.format = image.format;
				texture_desc.miplevels = find_max_mip_level(image.width, image.height);
				texture_desc.resource_state = RESOURCE_STATE_COMMON;
				texture_desc.name = texture_name;

				Texture *new_texture = render_device->create_texture(&texture_desc);
				textures_table.set(texture_name, new_texture);

				Buffer *staging_buffer = staging_buffers[j];
				if (!staging_buffer || staging_buffer->size() < new_texture->size()) {
					DELETE_PTR(staging_buffer);
					Buffer_Desc buffer_desc;
					buffer_desc.usage = RESOURCE_USAGE_UPLOAD;
					buffer_desc.size = align_address<u32>(texture_desc.width * dxgi_format_size(texture_desc.format), get_texture_pitch_alignment()) * texture_desc.height;
					buffer_desc.size = align_address<u32>(buffer_desc.size, get_texture_placement_alignment());
					buffer_desc.name = texture_name;
					staging_buffer = render_device->create_buffer(&buffer_desc);
					staging_buffers[j] = staging_buffer;
				}
				u8 *mapped_memory = static_cast<u8 *>(staging_buffer->write_only_ptr());

				u32 row_pitch = texture_desc.width * dxgi_format_size(texture_desc.format);
				u32 aligned_row_pitch = align_address<u32>(row_pitch, get_texture_pitch_alignment());

				for (u32 y = 0; y < texture_desc.height; y++) {
					u8 *buffer_row = mapped_memory + y * aligned_row_pitch;
					u8 *bitmap_row = image.data + y * row_pitch;
					memcpy((void *)buffer_row, (void *)bitmap_row, row_pitch);
				}
				Subresource_Footprint footprint = new_texture->subresource_footprint(0);
				copy_command_list->copy_buffer_to_texture(new_texture, staging_buffer, &footprint);
			}
		}
		uploaded_textures_number += textures_uploading_chunk;

		copy_command_list->close();
		render_sys->copy_queue->execute_command_list(copy_command_list);
		render_sys->copy_queue->signal(copy_fence);
		copy_fence->wait_for_gpu();
		copy_fence->increment_expected_value();
	}
	DELETE_PTR(copy_fence);
	free_memory(&staging_buffers);
}

Texture *Texture_Storage::find_texture_or_get_default(const char *texture_name, const char *default_texture_name)
{
	Texture *texture = NULL;
	if (texture_name) {
		if (textures_table.get(texture_name, texture)) {
			return texture;
		}
	}
	if (default_texture_name) {
		textures_table.get(default_texture_name, &texture);
		return texture;
	}
	return NULL;
}

void Material_Storage::init(Render_Device *_render_device)
{
	render_device = _render_device;
}

void Material_Storage::prepare_for_rendering()
{
	if (!upload_data_to_gpu) {
		return;
	}
	upload_data_to_gpu = false;

	if (!material_buffer || (material_buffer->count() < (u64)materials.count)) {
		DELETE_PTR(material_buffer);
		Buffer_Desc buffer_desc;
		buffer_desc.size = materials.get_size();
		buffer_desc.stride = materials.stride;
		buffer_desc.data = materials.to_void_ptr();
		buffer_desc.name = "Unified vertex buffer";

		material_buffer = render_device->create_buffer(&buffer_desc);
	} else {
		material_buffer->request_write();
		material_buffer->write(materials.to_void_ptr(), materials.get_size());
	}
}

u32 Material_Storage::add_material(u32 normal_texture_idx, u32 albedo_texture_idx, u32 roughness_metalic_texture_idx)
{
	Material material;
	material.normal_idx = normal_texture_idx;
	material.albedo_idx = albedo_texture_idx;
	material.roughness_metalic_idx = roughness_metalic_texture_idx;
	upload_data_to_gpu = true;

	return materials.push(material);
}

void Mesh_Storage::init(Render_Device *_render_device)
{
	render_device = _render_device;
}

void Mesh_Storage::pre_allocate_memory(u32 vertex_count, u32 index_count)
{
	unified_vertices.resize(unified_vertices.count + vertex_count);
	unified_points.resize(unified_vertices.count + vertex_count);
	unified_indices.resize(unified_indices.count + index_count);
}

void Mesh_Storage::prepare_for_rendering()
{
	if (!upload_data_to_gpu) {
		return;
	}
	upload_data_to_gpu = false;

	if (!unified_vertex_buffer || (unified_vertex_buffer->count() < (u64)unified_vertices.count)) {
		DELETE_PTR(unified_vertex_buffer);
		Buffer_Desc buffer_desc;
		buffer_desc.size = unified_vertices.get_size();
		buffer_desc.stride = unified_vertices.stride;
		buffer_desc.data = unified_vertices.to_void_ptr();
		buffer_desc.name = "Unified vertex buffer";

		unified_vertex_buffer = render_device->create_buffer(&buffer_desc);
	} else {
		unified_vertex_buffer->request_write();
		unified_vertex_buffer->write(unified_vertices.to_void_ptr(), unified_vertices.get_size());
	}

	if (!unified_point_buffer || (unified_point_buffer->count() < (u64)unified_points.count)) {
		DELETE_PTR(unified_point_buffer);
		Buffer_Desc buffer_desc;
		buffer_desc.size = unified_points.get_size();
		buffer_desc.stride = unified_points.stride;
		buffer_desc.data = unified_points.to_void_ptr();
		buffer_desc.name = "Unified point buffer";

		unified_point_buffer = render_device->create_buffer(&buffer_desc);
	} else {
		unified_point_buffer->request_write();
		unified_point_buffer->write(unified_points.to_void_ptr(), unified_points.get_size());
	}

	if (!unified_index_buffer || (unified_index_buffer->count() < (u64)unified_indices.count)) {
		DELETE_PTR(unified_index_buffer);
		Buffer_Desc buffer_desc;
		buffer_desc.size = unified_indices.get_size();
		buffer_desc.stride = unified_indices.stride;
		buffer_desc.data = unified_indices.to_void_ptr();
		buffer_desc.name = "Unified index buffer";

		unified_index_buffer = render_device->create_buffer(&buffer_desc);
	} else {
		unified_index_buffer->request_write();
		unified_index_buffer->write(unified_indices.to_void_ptr(), unified_indices.get_size());
	}
}

Vertex_PNTUV *Mesh_Storage::get_base_vertex(Mesh_Storage_Info *mesh_info)
{
	return &unified_vertices[mesh_info->vertex_offset];
}

Mesh_Storage_Info Mesh_Storage::add_mesh(const char *name, Triangle_Mesh *mesh, Array<Vector3> &mesh_points)
{
	Mesh_Storage_Info mesh_info;
	mesh_info.vertex_count = mesh->vertex_count();
	mesh_info.index_count = mesh->index_count();
	mesh_info.vertex_offset = unified_vertices.count;
	mesh_info.index_offset = unified_indices.count;
    
	merge(&unified_vertices, &mesh->vertices);
	merge(&unified_points, &mesh_points);
	merge(&unified_indices, &mesh->indices);

	upload_data_to_gpu = true;

	return mesh_info;
}