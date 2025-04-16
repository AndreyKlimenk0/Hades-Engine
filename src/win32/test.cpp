#include "test.h"
#include "../libs/str.h"
#include "../sys/sys.h"
#include "../libs/memory/base.h"
#include "../libs/number_types.h"
#include "../libs/structures/tree.h"
#include "../libs/structures/hash_table.h"
#include "../libs/structures/queue.h"

inline u32 pack_RGB(const Vector3 &rgb_value)
{
	u32 r = u32(255.0f * rgb_value.x);
	u32 g = u32(255.0f * rgb_value.y);
	u32 b = u32(255.0f * rgb_value.z);

	u32 result = 0;
	result |= r << 24;
	result |= g << 16;
	result |= b << 8;
	result |= 0xff;
	return result;
}

inline u32 encode_color(const Vector3 &rgb_value)
{
	u32 r = u32(255.0f * rgb_value.x);
	u32 g = u32(255.0f * rgb_value.y);
	u32 b = u32(255.0f * rgb_value.z);

	u32 result = 0;
	result |= r << 16;
	result |= g << 8;
	result |= b;
	return result;
}

struct alignas(256) World_Matrix {
	Matrix4 world_matrix;
};

struct Ring_Buffer {
	Ring_Buffer();
	~Ring_Buffer();

	struct Allocation_Info {
		u64 frame_number;
		u64 size;
	};

	u64 head = 0;
	u64 tail = 0;
	u64 size = 0;
	u64 memory_size = 0;
	u64 memory_used = 0;
	u64 frame_number = 0;

	void init(u64 size);
	void begin_frame(u64 frame_number);
	void free_frame_resources(u64 frame_number);

	u64 allocate(u64 size, u64 alignment = 0);

	Array<Allocation_Info> frames_allocations;
};

static const u64 INVALID_ALLOCATION = UINT64_MAX;

u64 Ring_Buffer::allocate(u64 allocation_size, u64 alignment)
{
	if (allocation_size > size) {
		return INVALID_ALLOCATION;
	}

	if ((tail + allocation_size) <= size) {
		u64 offset = tail;
		tail += allocation_size;
		frames_allocations.push({ frame_number, allocation_size });

		return offset;

	} else {

	}

	return u64();
}

inline bool ring_buffer_allocation_valid(u64 size)
{
	return size == INVALID_ALLOCATION;
}

void test()
{
}

void update_test()
{
}
