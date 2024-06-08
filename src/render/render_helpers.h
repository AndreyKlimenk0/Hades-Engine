#ifndef RENDER_HELPERS_H
#define RENDER_HELPERS_H

#include "../sys/engine.h"
#include "render_api.h"
#include "../libs/str.h"
#include "../libs/math/vector.h"
#include "../win32/win_types.h"

void fill_texture(void *value, Texture2D *texture2d);
bool is_valid_texture(Texture2D *texture2d, String *error_message = NULL);
u32 *r8_to_rgba32(u8 *data, u32 width, u32 height);

struct R24U8 {
	R24U8(u32 r24u8_value);
	R24U8(u32 numerator, u8 typeless_bits);

	u32 numerator;
	u8 typeless_bits;

	u32 get_packed_value();
	float get_unorm_value();
};

struct Gpu_Struct_Buffer {
	Gpu_Buffer gpu_buffer;

	template <typename T>
	void allocate(u32 elements_count);
	template <typename T>
	void update(Array<T> *array);
	void free();
};


template<typename T>
inline void Gpu_Struct_Buffer::allocate(u32 elements_count)
{
	Gpu_Buffer_Desc desc;
	desc.usage = RESOURCE_USAGE_DYNAMIC;
	desc.data = NULL;
	desc.data_size = sizeof(T);
	desc.struct_size = sizeof(T);
	desc.data_count = elements_count;
	desc.bind_flags = BIND_SHADER_RESOURCE;
	desc.cpu_access = CPU_ACCESS_WRITE;
	desc.misc_flags = RESOURCE_MISC_BUFFER_STRUCTURED;

	Gpu_Device *gpu_device = &Engine::get_render_system()->gpu_device;
	gpu_device->create_gpu_buffer(&desc, &gpu_buffer);
	gpu_device->create_shader_resource_view(&gpu_buffer);
}

template<typename T>
inline void Gpu_Struct_Buffer::update(Array<T> *array)
{
	if (array->count == 0) {
		return;
	}

	Render_Pipeline *render_pipeline = &Engine::get_render_system()->render_pipeline;

	if (array->count > gpu_buffer.data_count) {
		free();
		allocate<T>(array->count);
	}

	T *buffer = (T *)render_pipeline->map(gpu_buffer);
	memcpy((void *)buffer, (void *)array->items, sizeof(T) * array->count);
	render_pipeline->unmap(gpu_buffer);
}

inline void Gpu_Struct_Buffer::free()
{
	if (!gpu_buffer.is_empty()) {
		gpu_buffer.free();
	}
}

#endif
