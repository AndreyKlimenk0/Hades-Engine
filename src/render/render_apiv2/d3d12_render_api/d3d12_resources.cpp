#include <assert.h>

#include "to_d3d12_types.h"
#include "d3d12_resources.h"
#include "d3d12_device.h"
#include "d3d12_descriptor_heap.h"
#include "d3d12_functions.h"

#include "../../../sys/utils.h"
#include "../../../libs/memory/base.h"

inline Texture_Dimension to_texture_dimension(D3D12_RESOURCE_DIMENSION d3d12_resource_dimension)
{
	switch (d3d12_resource_dimension) {
		case D3D12_RESOURCE_DIMENSION_UNKNOWN:
			return TEXTURE_DIMENSION_UNKNOWN;
		case D3D12_RESOURCE_DIMENSION_BUFFER:
			return TEXTURE_DIMENSION_UNKNOWN;
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			return TEXTURE_DIMENSION_1D;
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			return TEXTURE_DIMENSION_2D;
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			return TEXTURE_DIMENSION_3D;
	}
	return TEXTURE_DIMENSION_UNKNOWN;
}

Resource_Desc::Resource_Desc(Buffer_Desc *_buffer_desc)
{
	type = RESOURCE_TYPE_BUFFER; 
	
	ZeroMemory(&buffer_desc, sizeof(Buffer_Desc));
	buffer_desc = *_buffer_desc;

	if (buffer_desc.usage == RESOURCE_USAGE_DEFAULT) {
		resource_state = RESOURCE_STATE_COMMON;
	} else if (buffer_desc.usage == RESOURCE_USAGE_UPLOAD) {
		resource_state = RESOURCE_STATE_COMMON;
	}
}

Resource_Desc::Resource_Desc(Buffer_Desc *_buffer_desc, Resource_Usage usage) : Resource_Desc(_buffer_desc)
{
	buffer_desc.usage = usage;

	if (buffer_desc.usage == RESOURCE_USAGE_DEFAULT) {
		resource_state = RESOURCE_STATE_COMMON;
	} else if (buffer_desc.usage == RESOURCE_USAGE_UPLOAD) {
		resource_state = RESOURCE_STATE_COMMON;
	}
}

Resource_Desc::Resource_Desc(Texture_Desc *_texture_desc)
{
	type = RESOURCE_TYPE_TEXTURE;
	
	ZeroMemory(&texture_desc, sizeof(Texture_Desc));
	texture_desc = *_texture_desc;
	
	resource_state = texture_desc.resource_state;
}

Resource_Desc::~Resource_Desc()
{
}

String &Resource_Desc::resource_name()
{
	static String temp;
	if (type == RESOURCE_TYPE_BUFFER) {
		return buffer_desc.name;
	} else if (type == RESOURCE_TYPE_TEXTURE) {
		return texture_desc.name;
	}
	return temp;
}

Resource_Usage Resource_Desc::resource_usage()
{
	switch (type) {
		case RESOURCE_TYPE_BUFFER:
			return buffer_desc.usage;
		case RESOURCE_TYPE_TEXTURE:
			return RESOURCE_USAGE_DEFAULT;
		default:
			assert(true);
	}
	return  (Resource_Usage)0;
}

D3D12_RESOURCE_DESC Resource_Desc::d3d12_resource_desc()
{
	D3D12_RESOURCE_DESC resource_desc;
	ZeroMemory(&resource_desc, sizeof(D3D12_RESOURCE_DESC));
	if (type == RESOURCE_TYPE_BUFFER) {
		resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resource_desc.Alignment = 0;
		resource_desc.Width = buffer_desc.size();
		resource_desc.Height = 1;
		resource_desc.DepthOrArraySize = 1;
		resource_desc.MipLevels = 1;
		resource_desc.Format = DXGI_FORMAT_UNKNOWN;
		resource_desc.SampleDesc.Count = 1;
		resource_desc.SampleDesc.Quality = 0;
		resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	} else if (type == RESOURCE_TYPE_TEXTURE) {
		resource_desc.Dimension = to_d3d12_resource_dimension(texture_desc.dimension);
		//resource_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resource_desc.Alignment = 0;
		resource_desc.Width = static_cast<u64>(texture_desc.width);
		resource_desc.Height = texture_desc.height;
		resource_desc.DepthOrArraySize = texture_desc.depth;
		//resource_desc.MipLevels = texture_desc.miplevels;
		resource_desc.MipLevels = 1;
		resource_desc.Format = texture_desc.format;
		resource_desc.SampleDesc.Count = 1;
		resource_desc.SampleDesc.Quality = 0;
		resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resource_desc.Flags = static_cast<D3D12_RESOURCE_FLAGS>(texture_desc.flags);
	}
	return resource_desc;
}

D3D12_RESOURCE_STATES Resource_Desc::d312_resource_state()
{
	return to_d3d12_resource_state(resource_state);
}

D3D12_Resource::D3D12_Resource(ComPtr<ID3D12Device> &device, Resource_Desc *resource_desc)
{
	D3D12_RESOURCE_DESC d3d12_resource_desc = resource_desc->d3d12_resource_desc();

	D3D12_RESOURCE_ALLOCATION_INFO allocation = device->GetResourceAllocationInfo(0, 1, &d3d12_resource_desc);
	total_size = allocation.SizeInBytes;

	if (resource_desc->type == RESOURCE_TYPE_BUFFER) {
		count = resource_desc->buffer_desc.count;
		stride = resource_desc->buffer_desc.stride;
	} else if (resource_desc->type == RESOURCE_TYPE_TEXTURE) {
		count = 1;
		stride = total_size;
	}

	D3D12_CLEAR_VALUE *d3d12_clear_value_ptr = NULL;
	D3D12_CLEAR_VALUE d3d12_clear_value;
	if ((d3d12_resource_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) || (d3d12_resource_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) {
		assert(resource_desc->type == RESOURCE_TYPE_TEXTURE);
		d3d12_clear_value = to_d3d12_clear_value(resource_desc->texture_desc.clear_value, resource_desc->texture_desc.format);
		d3d12_clear_value_ptr = &d3d12_clear_value;
	}
	
	D3D12_HEAP_PROPERTIES heap_properties;
	ZeroMemory(&heap_properties, sizeof(D3D12_HEAP_PROPERTIES));
	heap_properties.Type = to_d3d12_heap_type(resource_desc->resource_usage());

	HR(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &d3d12_resource_desc, resource_desc->d312_resource_state(), d3d12_clear_value_ptr, IID_PPV_ARGS(d3d12_resource.ReleaseAndGetAddressOf())));
}

D3D12_Resource::D3D12_Resource(ComPtr<ID3D12Device> &device, ComPtr<ID3D12Resource> &existing_resource)
{
	D3D12_RESOURCE_DESC d3d12_resource_desc = existing_resource->GetDesc();

	u32 mask = 0;
	D3D12_RESOURCE_ALLOCATION_INFO allocation = device->GetResourceAllocationInfo(mask, 1, &d3d12_resource_desc);
	total_size = allocation.SizeInBytes;
	count = 1;
	stride = total_size;

	d3d12_resource = existing_resource;
}

D3D12_Resource::D3D12_Resource(ComPtr<ID3D12Device> &device, ComPtr<ID3D12Heap> &heap, u64 heap_offset, Resource_Desc &resource_desc)
{
}

D3D12_Resource::~D3D12_Resource()
{
	unmap();
}

void *D3D12_Resource::map()
{
	if (mapped_memory == NULL) {
		HR(d3d12_resource->Map(0, NULL, &mapped_memory));
	}
	return mapped_memory;
}

void D3D12_Resource::unmap()
{
	if (mapped_memory != NULL) {
		d3d12_resource->Unmap(0, NULL);
	}
}

void D3D12_Resource::resource_footprint(Resource_Footprint *resource_footprint)
{
	u32 subresource_number = subresource_count();
	D3D12_RESOURCE_DESC desc = d3d12_resource_desc();

	u64 total_size = 0;
	Array<u32> row_counts;
	Array<u64> row_sizes;
	Array<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> d3d12_footprints;

	row_counts.resize(subresource_number);
	row_sizes.resize(subresource_number);
	d3d12_footprints.resize(subresource_number);

	ComPtr<ID3D12Device> device;
	d3d12_resource->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	device->GetCopyableFootprints(&desc, 0, subresource_number, 0, d3d12_footprints.items, row_counts.items, row_sizes.items, &total_size);

	resource_footprint->total_size = total_size;

	for (u32 i = 0; i < subresource_number; i++) {
		Subresource_Footprint subresource_footprint;
		subresource_footprint.subresource_index = i;
		subresource_footprint.row_count = row_counts[i];
		subresource_footprint.row_size = row_sizes[i];
		
		subresource_footprint.format = d3d12_footprints[i].Footprint.Format;
		subresource_footprint.width = d3d12_footprints[i].Footprint.Width;
		subresource_footprint.height = d3d12_footprints[i].Footprint.Height;
		subresource_footprint.depth = d3d12_footprints[i].Footprint.Depth;
		subresource_footprint.row_pitch = d3d12_footprints[i].Footprint.RowPitch;
		
		resource_footprint->subresource_footprints.push(subresource_footprint);
	}
}

u32 D3D12_Resource::subresource_count()
{
	return 1;
}

u64 D3D12_Resource::size()
{
	return total_size;
}

u64 D3D12_Resource::gpu_address()
{
	return d3d12_resource->GetGPUVirtualAddress();
}

ID3D12Resource *D3D12_Resource::get()
{
	return d3d12_resource.Get();
}

D3D12_RESOURCE_DESC D3D12_Resource::d3d12_resource_desc()
{
	return d3d12_resource->GetDesc();
}

D3D12_Base_Buffer::D3D12_Base_Buffer(D3D12_Render_Device *render_device, Resource_Desc *resource_desc) : D3D12_Resource(render_device->device, resource_desc), render_device(render_device)
{
}

D3D12_Base_Buffer::~D3D12_Base_Buffer()
{
	Descriptor_Heap_Pool *descriptor_pool = render_device->descriptor_pool;
	descriptor_pool->free(&constant_buffer_descriptor);
	descriptor_pool->free(&shader_resource_descriptor);
	descriptor_pool->free(&unordered_access_descriptor);
}

D3D12_Buffer::D3D12_Buffer(D3D12_Render_Device *render_device, Buffer_Desc *_buffer_desc) : render_device(render_device), buffer_desc(*_buffer_desc)
{
	assert(buffer_desc.count > 0);
	assert(buffer_desc.stride > 0);
	
	if (buffer_desc.usage == RESOURCE_USAGE_DEFAULT) {
		Resource_Desc resource_desc = Resource_Desc(&buffer_desc);
		default_buffer = new D3D12_Base_Buffer(render_device, &resource_desc);
		set_name(default_buffer->get(), "Default Buffer [name {}]", buffer_desc.name);

		if (buffer_desc.data) {
			// Should I just create D3D12_Resource and call Render_Device::safe_release ???
			Resource_Desc resource_desc = Resource_Desc(&buffer_desc, RESOURCE_USAGE_UPLOAD);
			D3D12_Base_Buffer *upload_buffer = new D3D12_Base_Buffer(render_device, &resource_desc);
			//set_name(upload_buffer->get(), "[Upload Buffer] name: {}, frame_number: {}", buffer_desc.name, render_device->frame_number);
			set_name(upload_buffer->get(), "(type: Upload buffer, status: Uploading data for default buffer, name: {})", buffer_desc.name);
			upload_buffers.push({ render_device->frame_number, upload_buffer });

			void *mapped_memory = upload_buffer->map();
			memcpy(mapped_memory, buffer_desc.data, buffer_desc.size());

			D3D12_Command_List *upload_command_list = render_device->upload_command_list();
			upload_command_list->copy(default_buffer, upload_buffer);
		}
	}
}

D3D12_Buffer::~D3D12_Buffer()
{
	if (default_buffer) {
		ComPtr<ID3D12Resource> d3d12_default_buffer = default_buffer->d3d12_resource;
		render_device->safe_release(d3d12_default_buffer);
		DELETE_PTR(default_buffer);
	}
	while (!upload_buffers.empty()) {
		Pair<u64, D3D12_Base_Buffer *> flight_resource = upload_buffers.front();
		render_device->safe_release(static_cast<D3D12_Resource *>(flight_resource.second), flight_resource.first);
		upload_buffers.pop();
		//DELETE_PTR(flight_resource.second);
	}
	while (!completed_upload_buffer.empty()) {
		D3D12_Base_Buffer *upload_buffer = completed_upload_buffer.front();
		completed_upload_buffer.pop();
		DELETE_PTR(upload_buffer);
	}
}

void D3D12_Buffer::begin_frame()
{
	if (buffer_desc.usage == RESOURCE_USAGE_UPLOAD) {
		D3D12_Base_Buffer *upload_buffer = NULL;
		if (!completed_upload_buffer.empty()) {
			upload_buffer = completed_upload_buffer.front();
			completed_upload_buffer.pop();
		} else {
			Resource_Desc resource_desc = Resource_Desc(&buffer_desc);
			upload_buffer = new D3D12_Base_Buffer(render_device, &resource_desc);
		}
		set_name(upload_buffer->get(), "[Upload Buffer] name: {}, frame_number: {}", buffer_desc.name, render_device->frame_number);
		upload_buffers.push({ render_device->frame_number, upload_buffer });
	}
}

void D3D12_Buffer::finish_frame(u64 frame_number)
{
	if (buffer_desc.usage == RESOURCE_USAGE_DEFAULT) {
		while (!upload_buffers.empty() && (upload_buffers.front().first <= frame_number)) {
			D3D12_Base_Buffer *upload_buffer = upload_buffers.front().second;
			upload_buffers.pop();
			DELETE_PTR(upload_buffer);
		}
	} else if (buffer_desc.usage == RESOURCE_USAGE_UPLOAD) {
		while (!upload_buffers.empty() && (upload_buffers.front().first <= frame_number)) {
			D3D12_Base_Buffer *upload_buffer = upload_buffers.front().second;
			upload_buffers.pop();
			set_name(upload_buffer->get(), "[Upload Buffer] name: {}, frame_number: {} completed", buffer_desc.name, frame_number);
			completed_upload_buffer.push(upload_buffer);
		}
	}
}

void D3D12_Buffer::request_write()
{
	if (buffer_desc.usage == RESOURCE_USAGE_DEFAULT) {
		Resource_Desc resource_desc = Resource_Desc(&buffer_desc, RESOURCE_USAGE_UPLOAD);
		D3D12_Base_Buffer *upload_buffer = new D3D12_Base_Buffer(render_device, &resource_desc);
		set_name(upload_buffer->get(), "[Upload Buffer] name: {}, frame_number: {}", buffer_desc.name, render_device->frame_number);
		upload_buffers.push({ render_device->frame_number, upload_buffer });

		D3D12_Command_List *upload_command_list = render_device->upload_command_list();
		upload_command_list->copy(default_buffer, upload_buffer);
	}
}

void D3D12_Buffer::write(void *data, u64 data_size, u64 alignment)
{
	assert(data);
	assert(data_size > 0);
	assert(data_size <= size());

	D3D12_Base_Buffer *upload_buffer = current_upload_buffer();
	void *mapped_memory = upload_buffer->map();
	memcpy(mapped_memory, data, data_size);
}

u64 D3D12_Buffer::size()
{
	D3D12_Base_Buffer *temp = current_buffer();
	return temp->size();
}

u64 D3D12_Buffer::gpu_virtual_address()
{
	D3D12_Base_Buffer *temp = current_buffer();
	return temp->gpu_address();
}

Buffer_Desc D3D12_Buffer::get_buffer_desc()
{
	return buffer_desc;
}

D3D12_Base_Buffer *D3D12_Buffer::current_buffer()
{
	if (buffer_desc.usage == RESOURCE_USAGE_DEFAULT) {
		return default_buffer;
	} else if (buffer_desc.usage == RESOURCE_USAGE_UPLOAD) {
		return current_upload_buffer();
	}
	return NULL;
}

D3D12_Base_Buffer *D3D12_Buffer::current_upload_buffer()
{
	return (!upload_buffers.empty() && (upload_buffers.back().first == render_device->frame_number)) ? upload_buffers.front().second : NULL;
}

CBV_Descriptor *D3D12_Buffer::constant_buffer_descriptor()
{
	D3D12_Base_Buffer *buffer = current_buffer();
	if (!buffer->constant_buffer_descriptor.valid()) {
		Descriptor_Heap_Pool *descriptor_pool = render_device->descriptor_pool;
		buffer->constant_buffer_descriptor = descriptor_pool->allocate_cb_descriptor(buffer);
	}
	return &buffer->constant_buffer_descriptor;
}

SRV_Descriptor *D3D12_Buffer::shader_resource_descriptor(u32 mipmap_level)
{
	D3D12_Base_Buffer *buffer = current_buffer();
	if (!buffer->shader_resource_descriptor.valid()) {
		Descriptor_Heap_Pool *descriptor_pool = render_device->descriptor_pool;
		buffer->shader_resource_descriptor = descriptor_pool->allocate_sr_descriptor(buffer);
	}
	return &buffer->shader_resource_descriptor;
}

UAV_Descriptor *D3D12_Buffer::unordered_access_descriptor(u32 mipmap_level)
{
	D3D12_Base_Buffer *buffer = current_buffer();
	if (!buffer->unordered_access_descriptor.valid()) {
		Descriptor_Heap_Pool *descriptor_pool = render_device->descriptor_pool;
		buffer->unordered_access_descriptor = descriptor_pool->allocate_sr_descriptor(buffer);
	}
	return &buffer->unordered_access_descriptor;
}

D3D12_Texture::D3D12_Texture(D3D12_Render_Device *_render_device, Texture_Desc *_texture_desc)
{
	texture_desc = *_texture_desc;
	render_device = _render_device;

	Resource_Desc resource_desc = { &texture_desc };
	resource = new D3D12_Resource(render_device->device, &resource_desc);
	//set_name(resource->get(), "(type: Texture, name: {})", texture_desc.name);
	set_name(resource->get(), "(type: Texture, name: {}, frame: {})", texture_desc.name, render_device->frame_number);

	if (texture_desc.data) {
		Buffer_Desc buffer_desc;
		buffer_desc.usage = RESOURCE_USAGE_UPLOAD;
		buffer_desc.stride = render_device->resource_allocation_info(&resource_desc).size;
		
		Resource_Desc upload_resource_desc = { &buffer_desc };
		D3D12_Resource *upload_buffer = new D3D12_Resource(render_device->device, &upload_resource_desc);
		set_name(upload_buffer->get(), "(type: Upload buffer, status: Uploading data for texture, texture_name: {})", texture_desc.name);
		void *mapped_memory = upload_buffer->map();

		assert(((dxgi_format_size(texture_desc.format) == 4) || (dxgi_format_size(texture_desc.format) == 8)));

		if (texture_desc.dimension == TEXTURE_DIMENSION_2D) {
			u8 *pointer = static_cast<u8 *>(mapped_memory);
			u8 *data = static_cast<u8 *>(texture_desc.data);

			u32 row_pitch = texture_desc.width * dxgi_format_size(texture_desc.format);
			u32 aligned_row_pitch = align_address<u32>(row_pitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

			for (u32 y = 0; y < texture_desc.height; y++) {
				u8 *buffer_row = pointer + y * aligned_row_pitch;
				u8 *bitmap_row = data + y * row_pitch;
				memcpy((void *)buffer_row, (void *)bitmap_row, row_pitch);
			}
		} else if (texture_desc.dimension == TEXTURE_DIMENSION_3D) {
			u8 *pointer = static_cast<u8 *>(mapped_memory);
			u8 *data = static_cast<u8 *>(texture_desc.data);

			u32 row_pitch = texture_desc.width * dxgi_format_size(texture_desc.format);
			u32 aligned_row_pitch = align_address<u32>(row_pitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

			for (u32 z = 0; z < texture_desc.depth; z++) {
				for (u32 y = 0; y < texture_desc.height; y++) {
					u8 *buffer_row = pointer + y * aligned_row_pitch + (z * aligned_row_pitch * texture_desc.height);
					u8 *bitmap_row = (u8 *)data + y * row_pitch + (z * row_pitch * texture_desc.height);
					memcpy((void *)buffer_row, (void *)bitmap_row, row_pitch);
				}
			}
		} else {
			assert(true);
		}
		Resource_Footprint texture_footprint;
		resource->resource_footprint(&texture_footprint);
		
		Subresource_Footprint temp = texture_footprint.subresource_footprint(0);
		
		D3D12_Command_List *command_list = render_device->upload_command_list();
		command_list->copy_buffer_to_texture(resource, upload_buffer, &temp);

		render_device->safe_release(upload_buffer);
	}
}

D3D12_Texture::D3D12_Texture(D3D12_Render_Device *_render_device, ComPtr<ID3D12Resource> &existing_resource)
{
	render_device = _render_device;
	resource = new D3D12_Resource(render_device->device, existing_resource);
	
	D3D12_RESOURCE_DESC d3d12_resource_desc = resource->d3d12_resource_desc();
	texture_desc.dimension = to_texture_dimension(d3d12_resource_desc.Dimension);
	texture_desc.width = d3d12_resource_desc.Width;
	texture_desc.height = d3d12_resource_desc.Height;
	texture_desc.depth = d3d12_resource_desc.DepthOrArraySize;
	texture_desc.miplevels = d3d12_resource_desc.MipLevels;
	texture_desc.format = d3d12_resource_desc.Format;
	texture_desc.flags = d3d12_resource_desc.Flags;
}

D3D12_Texture::~D3D12_Texture()
{
}

ID3D12Resource *D3D12_Texture::get()
{
	return resource->get();
}

u32 D3D12_Texture::subresource_count()
{
	return texture_desc.miplevels;
}

Subresource_Footprint D3D12_Texture::subresource_footprint(u32 subresource_index)
{
	Resource_Footprint texture_footprint;
	resource->resource_footprint(&texture_footprint);
	return texture_footprint.subresource_footprint(subresource_index);
}

Texture_Desc D3D12_Texture::get_texture_desc()
{
	return texture_desc;
}

SRV_Descriptor *D3D12_Texture::shader_resource_descriptor(u32 mipmap_level)
{
	if (!shader_resource_descriptors[mipmap_level].valid()) {
		Descriptor_Heap_Pool *descriptor_pool = render_device->descriptor_pool;
		shader_resource_descriptors[mipmap_level] = descriptor_pool->allocate_sr_descriptor(resource, mipmap_level);
	}
	return &shader_resource_descriptors[mipmap_level];
}

UAV_Descriptor *D3D12_Texture::unordered_access_descriptor(u32 mipmap_level)
{
	if (!unordered_access_descriptors[mipmap_level].valid()) {
		Descriptor_Heap_Pool *descriptor_pool = render_device->descriptor_pool;
		unordered_access_descriptors[mipmap_level] = descriptor_pool->allocate_ua_descriptor(resource, mipmap_level);
	}
	return &unordered_access_descriptors[mipmap_level];
}

DSV_Descriptor *D3D12_Texture::depth_stencil_descriptor()
{
	if (!_depth_stencil_descriptor.valid()) {
		Descriptor_Heap_Pool *descriptor_pool = render_device->descriptor_pool;
		_depth_stencil_descriptor = descriptor_pool->allocate_ds_descriptor(resource);
	}
	return &_depth_stencil_descriptor;
}

RTV_Descriptor *D3D12_Texture::render_target_descriptor()
{
	if (!_render_target_descriptor.valid()) {
		Descriptor_Heap_Pool *descriptor_pool = render_device->descriptor_pool;
		_render_target_descriptor = descriptor_pool->allocate_rt_descriptor(resource);
	}
	return &_render_target_descriptor;
}

D3D12_Sampler::D3D12_Sampler(D3D12_Render_Device *render_device, Sampler_Filter filter, Address_Mode uvw) : render_device(render_device), filter(filter), u(uvw), v(uvw), w(uvw)
{
}

D3D12_Sampler::~D3D12_Sampler()
{
}

Sampler_Descriptor *D3D12_Sampler::sampler_descriptor()
{
	if (!_sampler_descriptor.valid()) {
		Descriptor_Heap_Pool *descriptor_pool = render_device->descriptor_pool;
		_sampler_descriptor = descriptor_pool->allocate_sampler_descriptor(this);
	}
	return &_sampler_descriptor;
}

D3D12_SAMPLER_DESC D3D12_Sampler::d3d12_sampler_desc()
{
	D3D12_SAMPLER_DESC d3d12_sampler_desc;
	ZeroMemory(&d3d12_sampler_desc, sizeof(D3D12_SAMPLER_DESC));
	d3d12_sampler_desc.Filter = to_d3d12_filter(filter);
	d3d12_sampler_desc.AddressU = to_d3d12_texture_address_mode(u);
	d3d12_sampler_desc.AddressV = to_d3d12_texture_address_mode(v);
	d3d12_sampler_desc.AddressW = to_d3d12_texture_address_mode(w);
	d3d12_sampler_desc.MipLODBias = 0;
	d3d12_sampler_desc.MaxAnisotropy = filter == SAMPLER_FILTER_ANISOTROPIC ? 16 : 0;
	d3d12_sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3d12_sampler_desc.BorderColor[0] = 0;
	d3d12_sampler_desc.BorderColor[1] = 0;
	d3d12_sampler_desc.BorderColor[2] = 0;
	d3d12_sampler_desc.BorderColor[3] = 0;
	d3d12_sampler_desc.MinLOD = 0;
	d3d12_sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
	return d3d12_sampler_desc;
}
