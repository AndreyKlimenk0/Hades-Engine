#ifndef RENDER_API_GPU_RESOURCE_H
#define RENDER_API_GPU_RESOURCE_H

#include <d3d12.h>
#include <string.h>

#include "base.h"
#include "d3d12_object.h"
#include "../../libs/number_types.h"

struct GPU_Resource : D3D12_Object<ID3D12Resource> {
	GPU_Resource();
	virtual ~GPU_Resource();

	u32 count = 0;
	u32 stride = 0;

	u8 *map();
	void unmap();

	u32 get_size();
	u64 get_gpu_address();
	D3D12_RESOURCE_DESC d3d12_resource_desc();
};

struct Constant_Buffer : GPU_Resource {
	Constant_Buffer() {};
	~Constant_Buffer() {};

	void create(Gpu_Device &device, u32 size);
	
	template <typename T>
	void update(const T &data);
};

template<typename T>
inline void Constant_Buffer::update(const T &data)
{
	u8 *memory = map();
	memcpy((void *)memory, (void *)&data, sizeof(T));
	unmap();
}

struct Buffer : GPU_Resource {
	Buffer() {};
	~Buffer() {};

	void create(Gpu_Device &device, D3D12_HEAP_TYPE heap_type, u32 number_items, u32 item_size);

	template <typename T>
	void update(const T *data);
};

template<typename T>
inline void Buffer::update(const T *data)
{
	u8 *memory = map();
	memcpy((void *)memory, (void *)data, get_size());
	unmap();
}

#endif
