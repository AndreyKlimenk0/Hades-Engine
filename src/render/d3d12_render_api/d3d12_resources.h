#ifndef D3D12_RESOURCES_H
#define D3D12_RESOURCES_H

#include <d3d12.h>
#include <wrl/client.h>

#include "d3d12_descriptors.h"

#include "../render_api/render.h"
#include "../render_api/base_structs.h"
#include "../../libs/str.h"
#include "../../libs/math/structures.h"
#include "../../libs/number_types.h"
#include "../../libs/structures/queue.h"

using Microsoft::WRL::ComPtr;

struct D3D12_Render_Device;
struct Descriptor_Heap_Pool;

enum Resource_Type {
	RESOURCE_TYPE_BUFFER,
	RESOURCE_TYPE_TEXTURE
};
struct Resource_Desc {
	Resource_Desc(Buffer_Desc *_buffer_desc);
	Resource_Desc(Buffer_Desc *_buffer_desc, Resource_Usage usage);
	Resource_Desc(Texture_Desc *_texture_desc);
	~Resource_Desc();

	Resource_Type type;
	Resource_State resource_state;

	union {
		Buffer_Desc buffer_desc;
		Texture_Desc texture_desc;
	};

	String &resource_name();
	Resource_Usage resource_usage();
	D3D12_RESOURCE_DESC d3d12_resource_desc();
	D3D12_RESOURCE_STATES d312_resource_state();
};

struct D3D12_Resource {
	D3D12_Resource(ComPtr<ID3D12Device> &device, Resource_Desc *resource_desc);
	D3D12_Resource(ComPtr<ID3D12Device> &device, ComPtr<ID3D12Resource> &existing_resource);
	D3D12_Resource(ComPtr<ID3D12Device> &device, ComPtr<ID3D12Heap> &heap, u64 heap_offset, Resource_Desc &resource_desc);
	virtual ~D3D12_Resource();

	void *mapped_memory = NULL;
	u32 count = 0;
	u32 stride = 0;
	u64 total_size = 0;
	ComPtr<ID3D12Resource> d3d12_resource;

	void *map();
	void unmap();

	void resource_footprint(Resource_Footprint *resource_footprint);
	
	u32 subresource_count();
	u64 size();
	u64 gpu_address();
	ID3D12Resource *get();
	D3D12_RESOURCE_DESC d3d12_resource_desc();
};

struct D3D12_Base_Buffer : D3D12_Resource {
	D3D12_Base_Buffer(D3D12_Render_Device *render_device, Resource_Desc *resource_desc);
	//D3D12_Base_Buffer(ComPtr<ID3D12Device> &device, ComPtr<ID3D12Heap> &heap, u64 heap_offset, Resource_Desc &resource_desc);
	~D3D12_Base_Buffer();

	D3D12_Render_Device *render_device = NULL;

	D3D12_GPU_Descriptor constant_buffer_descriptor;
	D3D12_GPU_Descriptor shader_resource_descriptor;
	D3D12_GPU_Descriptor unordered_access_descriptor;
};

struct D3D12_Buffer : Buffer {
	D3D12_Buffer(D3D12_Render_Device *render_device, Buffer_Desc *_buffer_desc);
	~D3D12_Buffer();

	D3D12_Render_Device *render_device = NULL;

	Buffer_Desc buffer_desc;
	D3D12_Base_Buffer *default_buffer;
	Queue<Pair<u64, D3D12_Base_Buffer *>> upload_buffers;
	Queue<D3D12_Base_Buffer *> completed_upload_buffer;
	
	void begin_frame();
	void finish_frame(u64 frame_number);
	
	void request_write();
	void write(void *data, u64 data_size, u64 alignment = 0);

	u64 size();
	u64 gpu_virtual_address();
	Buffer_Desc get_buffer_desc();
	D3D12_Base_Buffer *current_buffer();
	D3D12_Base_Buffer *current_upload_buffer();

	CBV_Descriptor *constant_buffer_descriptor();
	SRV_Descriptor *shader_resource_descriptor(u32 mipmap_level = 0);
	UAV_Descriptor *unordered_access_descriptor(u32 mipmap_level = 0);
};

struct D3D12_Texture : Texture {
	D3D12_Texture(D3D12_Render_Device *_render_device, Texture_Desc *_texture_desc);
	D3D12_Texture(D3D12_Render_Device *_render_device, ComPtr<ID3D12Resource> &existing_resource);
	~D3D12_Texture();

	D3D12_Render_Device *render_device = NULL;
	D3D12_Resource *resource = NULL;

	Texture_Desc texture_desc;
	D3D12_CPU_Descriptor _depth_stencil_descriptor;
	D3D12_CPU_Descriptor _render_target_descriptor;
	Array<D3D12_GPU_Descriptor> shader_resource_descriptors;
	Array<D3D12_GPU_Descriptor> unordered_access_descriptors;

	ID3D12Resource *get();

	u32 subresource_count();
	Subresource_Footprint subresource_footprint(u32 subresource_index);

	Texture_Desc get_texture_desc();

	SRV_Descriptor *shader_resource_descriptor(u32 mipmap_level = 0);
	UAV_Descriptor *unordered_access_descriptor(u32 mipmap_level = 0);
	DSV_Descriptor *depth_stencil_descriptor();
	RTV_Descriptor *render_target_descriptor();
};

struct D3D12_Sampler : Sampler {
	D3D12_Sampler(D3D12_Render_Device *render_device, Sampler_Filter filter, Address_Mode uvw);
	~D3D12_Sampler();

	D3D12_Render_Device *render_device = NULL;
	Sampler_Filter filter;
	Address_Mode u;
	Address_Mode v;
	Address_Mode w;
	D3D12_GPU_Descriptor _sampler_descriptor;

	Sampler_Descriptor *sampler_descriptor();
	D3D12_SAMPLER_DESC d3d12_sampler_desc();
};
#endif
