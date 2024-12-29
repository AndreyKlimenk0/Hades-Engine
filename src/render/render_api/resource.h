#ifndef RENDER_API_GPU_RESOURCE_H
#define RENDER_API_GPU_RESOURCE_H

#include <d3d12.h>
#include <string.h>

#include "base.h"
#include "d3d12_object.h"
#include "../../libs/number_types.h"

enum GPU_Heap_Type {
	GPU_HEAP_TYPE_DEFAULT,
	GPU_HEAP_TYPE_UPLOAD,
	GPU_HEAP_TYPE_READBACK
};

enum GPU_Heap_Content {
	GPU_HEAP_CONTAIN_BUFFERS,
	GPU_HEAP_CONTAIN_BUFFERS_AND_TEXTURES,
	GPU_HEAP_CONTAIN_RT_DS_TEXTURES
};

struct GPU_Heap : D3D12_Object<ID3D12Heap> {
	GPU_Heap();
	~GPU_Heap();
	
	u32 offset = 0;

	void create(Gpu_Device &device, u32 heap_size, GPU_Heap_Type heap_type, GPU_Heap_Content content);
	D3D12_HEAP_DESC d3d12_heap_desc();
};

struct GPU_Resource : D3D12_Object<ID3D12Resource> {
	GPU_Resource();
	virtual ~GPU_Resource();

	u32 count = 0;
	u32 stride = 0;

	void set_size(u32 number_items, u32 item_size);

	u32 get_size();
	u64 get_gpu_address();
	D3D12_RESOURCE_DESC d3d12_resource_desc();
};
#endif
