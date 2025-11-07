#ifndef RENDER_RESOURCES_H
#define RENDER_RESOURCES_H

#include <assert.h>

#include "../libs/str.h"
#include "../libs/memory/base.h"
#include "../libs/number_types.h"
#include "../libs/structures/queue.h"
#include "render_api/base.h"
#include "render_api/buffer.h"

struct Resource_Allocator;
struct Descriptor_Heap_Pool;
struct Copy_Manager;

enum Buffer_Type {
	BUFFER_TYPE_DEFAULT,
	BUFFER_TYPE_UPLOAD,
	BUFFER_TYPE_READBACK,
};

struct Buffer {
	Buffer();
	~Buffer();

	u64 frame_number = 0;

	Buffer_Type type;
	GPU_Buffer *current_buffer = NULL;
	Resource_Allocator *allocator = NULL;
	Descriptor_Heap_Pool *descriptor_pool = NULL;
	Copy_Manager *copy_manager = NULL;

	String debug_name;
	Buffer_Desc desc;
	GPU_Buffer default_buffer;

	Queue<GPU_Buffer *> free_upload_buffers;
	Queue<Pair<GPU_Buffer *, u64>> working_upload_buffers;

	void create(Buffer_Type buffer_type, Buffer_Desc *buffer_desc, Resource_Allocator *resource_allocator, Descriptor_Heap_Pool *descriptor_heap_pool, Copy_Manager *resource_copy_manager);
	void free();
	void begin_frame(u64 _frame_number);
	void end_frame(u64 _frame_number);
	void copy_upload_buffer();
	void set_debug_name(const char *name);

	template <typename T>
	void write(const T *data, u32 data_size, u32 alignment = 0);

	u32 get_size();
	u64 get_gpu_address();
	GPU_Buffer *current_frame_upload_buffer();
	GPU_Buffer *allocate_new_upload_buffer();

	CB_Descriptor get_constant_buffer_descriptor();
	SR_Descriptor get_shader_resource_descriptor();
	UA_Descriptor get_unordered_access_descriptor();
};

template<typename T>
inline void Buffer::write(const T *data, u32 data_size, u32 alignment)
{
	GPU_Buffer *upload_buffer = current_frame_upload_buffer();
	assert((upload_buffer || (type != BUFFER_TYPE_UPLOAD)));
	
	if (!upload_buffer && (type == BUFFER_TYPE_DEFAULT)) {
		upload_buffer = allocate_new_upload_buffer();
	}

	u32 aligned_size = align_address<u32>(data_size, alignment);
	assert(aligned_size <= upload_buffer->get_size());

	u8 *mapped_memory = upload_buffer->map();
	memcpy((void *)mapped_memory, (void *)data, aligned_size);

	copy_upload_buffer();
}

//const u32 ALLOW_RENDER_TARGET = 0x1,
const u32 DEPTH_STENCIL_RESOURCE = 0x2;
const u32 ALLOW_UNORDERED_ACCESS = 0x4;
//const u32 DENY_SHADER_RESOURCE = 0x8,
//const u32 ALLOW_CROSS_ADAPTER = 0x10,
//const u32 ALLOW_SIMULTANEOUS_ACCESS = 0x20,
//const u32 VIDEO_DECODE_REFERENCE_ONLY = 0x40,
//const u32 VIDEO_ENCODE_REFERENCE_ONLY = 0x80,
//const u32 RAYTRACING_ACCELERATION_STRUCTURE = 0x100;

enum Texture_Dimension {
	TEXTURE_DIMENSION_UNKNOWN,
	TEXTURE_DIMENSION_1D,
	TEXTURE_DIMENSION_2D,
	TEXTURE_DIMENSION_3D
};

struct Texture_Desc {
	Texture_Dimension dimension = TEXTURE_DIMENSION_UNKNOWN;
	u32 width = 0;
	u32 height = 0;
	u32 depth = 1;
	u32 miplevels = 1;
	u32 flags = 0;
	DXGI_FORMAT format;
	Clear_Value clear_value;
	Resource_State resource_state = RESOURCE_STATE_COMMON;
	String name;

	bool valid();
};

struct Texture : GPU_Resource {
	Texture();
	~Texture();

	Resource_Allocator *allocator = NULL;
	Descriptor_Heap_Pool *descriptor_pool = NULL;

	DS_Descriptor depth_stencil_descriptor;
	RT_Descriptor render_target_descriptor;
	Array<SR_Descriptor> shader_resource_descriptors;
	Array<UA_Descriptor> unordered_access_descriptors;

	void create(Texture_Desc *desc, Resource_Allocator *resource_allocator, Descriptor_Heap_Pool *descriptor_heap_pool);
	void free();

	u32 get_size();
	u32 get_subresource_count();
	Texture_Desc get_texture_desc();
	
	SR_Descriptor get_shader_resource_descriptor(u32 mipmap_level = 0);
	UA_Descriptor get_unordered_access_descriptor(u32 mipmap_level = 0);
	DS_Descriptor get_depth_stencil_descriptor();
	RT_Descriptor get_render_target_descriptor();
};

#endif
