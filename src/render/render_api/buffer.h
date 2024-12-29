#ifndef RENDER_API_BUFFER_H
#define RENDER_API_BUFFER_H

#include "resource.h"
#include "descriptor_heap.h"

#include "../../libs/number_types.h"

struct Buffer : GPU_Resource {
	Buffer();
	~Buffer();

	u8 *map();
	void unmap();

	CB_Descriptor cb_descriptor;
	SR_Descriptor sr_descriptor;
	UA_Descriptor ua_descriptor;

	void create(Gpu_Device &device, GPU_Heap &heap, u32 number_items, u32 item_size);
	void create(Gpu_Device &device, GPU_Heap_Type heap_type, u32 number_items, u32 item_size);
};

#endif
