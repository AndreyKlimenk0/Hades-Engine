#ifndef RENDER_API_BUFFER_H
#define RENDER_API_BUFFER_H

#include <assert.h>

#include "base.h"
#include "resource.h"
#include "descriptor_heap.h"
#include "../../libs/memory/base.h"

#include "../../libs/number_types.h"


struct Buffer_Desc {
	Buffer_Desc();
	Buffer_Desc(u32 size);
	Buffer_Desc(u32 count, u32 stride);
	~Buffer_Desc();

	u32 count = 0;
	u32 stride = 0;
	Clear_Value clear_value;

	u32 get_size();
};

struct Buffer : GPU_Resource {
	Buffer();
	~Buffer();

	CB_Descriptor cb_descriptor;
	SR_Descriptor sr_descriptor;
	UA_Descriptor ua_descriptor;

	u8 *map();
	void unmap();

	void create(Gpu_Device &device, GPU_Heap &heap, u64 offset, Resource_State resource_state, const Buffer_Desc &buffer_desc);
	void create(Gpu_Device &device, GPU_Heap_Type heap_type, Resource_State resource_state, const Buffer_Desc &buffer_desc);

	template <typename T>
	void write(const T &data, u32 alignment = 0);
};

template<typename T>
inline void Buffer::write(const T &data, u32 alignment)
{
	u32 aligned_size = align_address<u32>(sizeof(T), alignment);
	assert(aligned_size <= GPU_Resource::get_size());

	u8 *ptr = map();
	memcpy((void *)ptr, (void *)&data, aligned_size);
	unmap();
}

#endif
