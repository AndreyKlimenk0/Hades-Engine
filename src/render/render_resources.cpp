#include <assert.h>

#include "../libs/str.h"
#include "render_system.h"
#include "render_helpers.h"
#include "render_resources.h"

// Use to get render system
#include "../sys/engine.h"

inline D3D12_RESOURCE_DIMENSION to_d3d12_resource_dimension(Texture_Dimension texture_dimension)
{
	switch (texture_dimension) {
		case TEXTURE_DIMENSION_UNKNOWN:
			return D3D12_RESOURCE_DIMENSION_UNKNOWN;
		case TEXTURE_DIMENSION_1D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		case TEXTURE_DIMENSION_2D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		case TEXTURE_DIMENSION_3D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	}
	assert(false);
	return static_cast<D3D12_RESOURCE_DIMENSION>(0);
}

 Buffer::Buffer()
{
	 debug_name = "Unknown";
}

 Buffer::~Buffer()
{
	 free();
}

 void Buffer::create(Buffer_Type buffer_type, Buffer_Desc *buffer_desc, Resource_Allocator *resource_allocator, Descriptor_Heap_Pool *descriptor_heap_pool, Copy_Manager *resource_copy_manager)
{
	desc = *buffer_desc;
	type = buffer_type;
	allocator = resource_allocator;
	descriptor_pool = descriptor_heap_pool;
	copy_manager = resource_copy_manager;
	
	if (type == BUFFER_TYPE_DEFAULT) {
		Resource_Allocation allocation = allocator->allocate_buffer(buffer_type, desc.get_size());
		default_buffer.create(Engine::get_render_system()->gpu_device, *allocation.heap, allocation.heap_offset, RESOURCE_STATE_COMMON, desc);
		default_buffer.set_debug_name(format_string("Default Buffer [name {}]", debug_name));
		current_buffer = &default_buffer;
	}
}

 void Buffer::free()
 {
	 default_buffer.free();
	 assert(working_upload_buffers.empty());
	 while (!free_upload_buffers.empty()) {
		 GPU_Buffer *buffer = free_upload_buffers.front();
		 DELETE_PTR(buffer);
		 free_upload_buffers.pop();
	 }
 }

 void Buffer::begin_frame(u64 _frame_number)
{
	frame_number = _frame_number;

	if (type == BUFFER_TYPE_UPLOAD) {
		if (!free_upload_buffers.empty()) {
			current_buffer = free_upload_buffers.front();
			current_buffer->set_debug_name(format_string("Upload Buffer [name {}] [frame {}]", debug_name, frame_number));
			working_upload_buffers.push({ current_buffer, frame_number });
			free_upload_buffers.pop();
		} else {
			current_buffer = allocate_new_upload_buffer();
		}
	}
}

 void Buffer::end_frame(u64 _frame_number)
{
	if (type == BUFFER_TYPE_DEFAULT) {
		// Assume that all upload buffers are used only once so they can be deleted.
		while (!working_upload_buffers.empty() && (working_upload_buffers.front().second <= _frame_number)) {
			working_upload_buffers.pop();
		}
	} else if (type == BUFFER_TYPE_UPLOAD) {
		while (!working_upload_buffers.empty() && (working_upload_buffers.front().second <= _frame_number)) {
			GPU_Buffer *buffer = working_upload_buffers.front().first;
			buffer->set_debug_name(format_string("Upload buffer [name {}] [frame {} completed]", debug_name, _frame_number));
			free_upload_buffers.push(buffer);
			working_upload_buffers.pop();
		}
	}
}

 void Buffer::copy_upload_buffer()
{
	if (type == BUFFER_TYPE_DEFAULT) {
		Copy_Command copy_command = Copy_Command(&default_buffer, current_frame_upload_buffer());
		copy_manager->add_copy_command(&copy_command);
	}
}

 void Buffer::set_debug_name(const char *name)
 {
	 debug_name = name;
	 if (type == BUFFER_TYPE_DEFAULT) {
		default_buffer.set_debug_name(format_string("Default Buffer [name {}]", debug_name));
	 }
 }

 u32 Buffer::get_size()
{
	//assert(type == BUFFER_TYPE_DEFAULT);
	return current_buffer->get_size();
}

 GPU_Buffer *Buffer::current_frame_upload_buffer()
{
	return (!working_upload_buffers.empty() && working_upload_buffers.back().second == frame_number) ? working_upload_buffers.back().first : NULL;
}

 GPU_Buffer *Buffer::allocate_new_upload_buffer()
{
	GPU_Buffer *upload_buffer = new GPU_Buffer();
	Resource_Allocation allocation = allocator->allocate_buffer(BUFFER_TYPE_UPLOAD, desc.get_size());
	upload_buffer->create(Engine::get_render_system()->gpu_device, *allocation.heap, allocation.heap_offset, RESOURCE_STATE_GENERIC_READ, desc);
	upload_buffer->set_debug_name(format_string("Upload Buffer [name {}] [frame {}]", debug_name, frame_number));
	working_upload_buffers.push({ upload_buffer, frame_number });
	return working_upload_buffers.back().first;
}

 CB_Descriptor Buffer::get_constant_buffer_descriptor()
{
	GPU_Buffer *buffer = current_buffer;
	if (!buffer->cb_descriptor.valid()) {
		buffer->cb_descriptor = descriptor_pool->allocate_cb_descriptor(buffer);
	}
	return buffer->cb_descriptor;
}

 SR_Descriptor Buffer::get_shader_resource_descriptor()
 {
	 GPU_Buffer *buffer = current_buffer;
	 if (!buffer->sr_descriptor.valid()) {
		 buffer->sr_descriptor = descriptor_pool->allocate_sr_descriptor(buffer);
	 }
	 return buffer->sr_descriptor;
 }

 Texture::Texture()
 {
 }

 Texture::~Texture()
 {
	 free();
 }

 void Texture::create(Texture_Desc *texture_desc, Resource_Allocator *resource_allocator, Descriptor_Heap_Pool *descriptor_heap_pool)
 {
	 assert(texture_desc->valid());
	
	 set_resource_parameters(1, texture_desc->width * texture_desc->height * texture_desc->depth);
	 allocator = resource_allocator;
	 descriptor_pool = descriptor_heap_pool;

	 shader_resource_descriptors.reserve(texture_desc->miplevels);
	 unordered_access_descriptors.reserve(texture_desc->miplevels);

	 D3D12_RESOURCE_DESC resource_desc;
	 ZeroMemory(&resource_desc, sizeof(D3D12_RESOURCE_DESC));
	 resource_desc.Dimension = to_d3d12_resource_dimension(texture_desc->dimension);
	 resource_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	 resource_desc.Width = static_cast<u64>(texture_desc->width);
	 resource_desc.Height = texture_desc->height;
	 resource_desc.DepthOrArraySize = texture_desc->depth;
	 resource_desc.MipLevels = texture_desc->miplevels;
	 resource_desc.Format = texture_desc->format;
	 resource_desc.SampleDesc.Count = 1;
	 resource_desc.SampleDesc.Quality = 0;
	 resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	 resource_desc.Flags = static_cast<D3D12_RESOURCE_FLAGS>(texture_desc->flags);

	 auto &gpu_device = Engine::get_render_system()->gpu_device;
	 D3D12_RESOURCE_ALLOCATION_INFO allocation_info = gpu_device.Get()->GetResourceAllocationInfo(0, 1, &resource_desc);
	 Resource_Allocation allocation = allocator->allocate_texture(allocation_info.SizeInBytes);
	 GPU_Resource::create(gpu_device, *allocation.heap, allocation.heap_offset, texture_desc->resource_state, resource_desc, texture_desc->clear_value);
	 //GPU_Resource::create(*resource_allocator->gpu_device, GPU_HEAP_TYPE_DEFAULT, texture_desc->resource_state, resource_desc, texture_desc->clear_value);
 }

 void Texture::free()
 {
	 descriptor_pool->free(&depth_stencil_descriptor);
	 descriptor_pool->free(&render_target_descriptor);
	 
	 for (u32 i = 0; i < shader_resource_descriptors.count; i++) {
		 descriptor_pool->free(&shader_resource_descriptors[i]);
	 }

	 for (u32 i = 0; i < unordered_access_descriptors.count; i++) {
		 descriptor_pool->free(&unordered_access_descriptors[i]);
	 }
 }

 u32 Texture::get_subresource_count()
 {
	 D3D12_RESOURCE_DESC d3d12_texture_desc = d3d12_resource_desc();
	 assert(d3d12_texture_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D);
	 return static_cast<u32>(d3d12_texture_desc.MipLevels);
 }

 u32 Texture::get_size()
 {
	 D3D12_RESOURCE_DESC d3d12_texture_desc = d3d12_resource_desc();
	 assert(d3d12_texture_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D);
	 return align_address<u32>(d3d12_texture_desc.Width * dxgi_format_size(d3d12_texture_desc.Format), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * d3d12_texture_desc.Height;
 }

 Texture_Desc Texture::get_texture_desc()
 {
	 D3D12_RESOURCE_DESC d3d12_texture_desc = d3d12_resource_desc();
	 assert(d3d12_texture_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D);

	 Texture_Desc texture_desc;
	 texture_desc.dimension = TEXTURE_DIMENSION_2D;
	 texture_desc.width = static_cast<u32>(d3d12_texture_desc.Width);
	 texture_desc.height = d3d12_texture_desc.Height;
	 texture_desc.miplevels = d3d12_texture_desc.MipLevels;
	 texture_desc.flags = d3d12_texture_desc.Flags;
	 texture_desc.format = d3d12_texture_desc.Format;
	 
	 return texture_desc;
 }

 SR_Descriptor Texture::get_shader_resource_descriptor(u32 mipmap_level)
 {
	 if (!shader_resource_descriptors[mipmap_level].valid()) {
		 shader_resource_descriptors[mipmap_level] = descriptor_pool->allocate_sr_descriptor(this, mipmap_level);
	 }
	 return shader_resource_descriptors[mipmap_level];
 }

 UA_Descriptor Texture::get_unordered_access_descriptor(u32 mipmap_level)
 {
	 if (!unordered_access_descriptors[mipmap_level].valid()) {
		 unordered_access_descriptors[mipmap_level] = descriptor_pool->allocate_ua_descriptor(this, mipmap_level);
	 }
	 return unordered_access_descriptors[mipmap_level];
 }

 DS_Descriptor Texture::get_depth_stencil_descriptor()
 {
	 if (!depth_stencil_descriptor.valid()) {
		 depth_stencil_descriptor = descriptor_pool->allocate_ds_descriptor(this);
	 }
	 return depth_stencil_descriptor;
 }

 RT_Descriptor Texture::get_render_target_descriptor()
 {
	 if (!render_target_descriptor.valid()) {
		 render_target_descriptor = descriptor_pool->allocate_rt_descriptor(this);
	 }
	 return render_target_descriptor;
 }

 bool Texture_Desc::valid()
 {
	 switch (dimension) {
		 case TEXTURE_DIMENSION_UNKNOWN:
			 return false;
		 case TEXTURE_DIMENSION_1D:
			 return (width > 0) && (height == 1) && (depth == 1) && (format != DXGI_FORMAT_UNKNOWN);
		 case TEXTURE_DIMENSION_2D:
			 return (width > 0) && (height > 0) && (depth == 1) && (format != DXGI_FORMAT_UNKNOWN);
		 case TEXTURE_DIMENSION_3D:
			 return (width > 0) && (height > 0) && (depth > 0) && (format != DXGI_FORMAT_UNKNOWN);
	 }
	 assert(false);
	 return false;
 }
