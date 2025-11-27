#ifndef RENDER_DEVICE_H
#define RENDER_DEVICE_H

#include "../../libs/number_types.h"

namespace gpu {

	struct Buffer_Desc {
		const char *name = NULL;
		u32 stride = 0;
		u32 count = 0;
		void *data = NULL;

		u64 size();
	};

	struct Buffer {
		Buffer();
		virtual ~Buffer();

		virtual Buffer_Desc buffer_desc() = 0;
	};

	struct Texture_Desc {

	};

	struct Texture {
		Texture();
		virtual ~Texture();

		virtual Texture_Desc texture_desc() = 0;
	};

	struct Resource_Allocator {
		Resource_Allocator();
		~Resource_Allocator();

		virtual u64 allocate(u64 size, u64 alignment = 0) = 0;
	};

	struct Render_Device {
		Render_Device();
		virtual ~Render_Device();

		Resource_Allocator *resource_allocator = NULL;

		virtual bool create() = 0;

		void set_context();
		
		virtual Buffer *create_buffer(Buffer_Desc *buffer_desc) = 0;
		virtual Texture *create_texture(Texture_Desc *texture_desc) = 0;
	};
}

#endif