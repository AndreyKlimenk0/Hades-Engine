#include <d3d12.h>
#include <wrl/client.h>

#include "render_device.h"
#include "../../sys/utils.h"

using Microsoft::WRL::ComPtr;

namespace gpu {
	D3D12_RESOURCE_STATES to_d3d12_resource_state(const Resource_State &resource_state)
	{
		switch (resource_state) {
			case RESOURCE_STATE_COMMON:
				return D3D12_RESOURCE_STATE_COMMON;
			case RESOURCE_STATE_GENERIC_READ:
				return D3D12_RESOURCE_STATE_GENERIC_READ;
			case RESOURCE_STATE_COPY_DEST:
				return D3D12_RESOURCE_STATE_COPY_DEST;
			case RESOURCE_STATE_COPY_SOURCE:
				return D3D12_RESOURCE_STATE_COPY_SOURCE;
			case RESOURCE_STATE_RENDER_TARGET:
				return D3D12_RESOURCE_STATE_RENDER_TARGET;
			case RESOURCE_STATE_PRESENT:
				return D3D12_RESOURCE_STATE_PRESENT;
			case RESOURCE_STATE_DEPTH_WRITE:
				return D3D12_RESOURCE_STATE_DEPTH_WRITE;
			case RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
				return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			case RESOURCE_STATE_ALL_SHADER_RESOURCE:
				return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		}
		assert(false);
		return (D3D12_RESOURCE_STATES)0;
	}

	struct D3D12_Buffer : Buffer {
		D3D12_Buffer(ComPtr<ID3D12Device> &device, ComPtr<ID3D12Resource> &d3d12_buffer);
		~D3D12_Buffer();

		Buffer_Desc buffer_desc();
	};

	struct D3D12_Texture : Texture {
		D3D12_Texture();
		~D3D12_Texture();

		Texture_Desc texture_desc();
	};

	struct D3D12_Render_Device : Render_Device {
		D3D12_Render_Device();
		 ~D3D12_Render_Device();

		 ComPtr<ID3D12Device> device;

		bool create();

		Buffer *create_buffer(Buffer_Desc *buffer_desc);
		Texture *create_texture(Texture_Desc *texture_desc);
	};

	D3D12_Render_Device::D3D12_Render_Device()
	{
	}
	
	D3D12_Render_Device::~D3D12_Render_Device()
	{
	}

	Buffer *D3D12_Render_Device::create_buffer(Buffer_Desc *buffer_desc)
	{
		D3D12_RESOURCE_DESC resource_desc;
		ZeroMemory(&resource_desc, sizeof(D3D12_RESOURCE_DESC));
		resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resource_desc.Alignment = 0;
		resource_desc.Width = buffer_desc->size();
		resource_desc.Height = 1;
		resource_desc.DepthOrArraySize = 1;
		resource_desc.MipLevels = 1;
		resource_desc.Format = DXGI_FORMAT_UNKNOWN;
		resource_desc.SampleDesc.Count = 1;
		resource_desc.SampleDesc.Quality = 0;
		resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12_CLEAR_VALUE *d3d12_clear_value = NULL;
		D3D12_CLEAR_VALUE temp = to_d3d12_clear_value(clear_value, resource_desc.Format);
		if ((resource_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) || (resource_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) {
			d3d12_clear_value = &temp;
		}

		u64 address = resource_allocator->allocate(buffer_desc->size());
		if (address == 0) {
		}
		
		ComPtr<ID3D12Resource> d3d12_buffer;
		HR(device->CreatePlacedResource(heap.get(), offset, &resource_desc, to_d3d12_resource_state(resource_state), d3d12_clear_value, IID_PPV_ARGS(release_and_get_address())));
		//HR(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_COMMON, d3d12_clear_value, IID_PPV_ARGS(d3d12_buffer.ReleaseAndGetAddressOf())));

		D3D12_Buffer *buffer = new D3D12_Buffer(device, d3d12_buffer);
		
		return buffer;
	}
}