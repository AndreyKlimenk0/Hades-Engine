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

struct Shadow_Atlas {
	u32 atlas_size = 8192;
	u32 cascade_size = 1024;
};

Shadow_Atlas shadow_atlas;

Vector2 cascade_ndc_to_atlas_ndc(Vector2 cascade_ndc_coordinates, u32 shadow_cascade_index)
{
	u32 shadow_cascade_rows = shadow_atlas.atlas_size / shadow_atlas.cascade_size;
	u32 shadow_cascade_cols = shadow_atlas.atlas_size / shadow_atlas.cascade_size;
	u32 shadow_cascade_row_index = shadow_cascade_index % shadow_cascade_rows;
	u32 shadow_cascade_col_index = shadow_cascade_index / shadow_cascade_cols;
	Vector2 shadow_atlas_ndc_coordinates;
	shadow_atlas_ndc_coordinates.x = ((cascade_ndc_coordinates.x * (shadow_atlas.cascade_size - 1)) + ((shadow_atlas.cascade_size - 1) * shadow_cascade_row_index)) / shadow_atlas.atlas_size;
	shadow_atlas_ndc_coordinates.y = ((cascade_ndc_coordinates.y * (shadow_atlas.cascade_size - 1)) + ((shadow_atlas.cascade_size - 1) * shadow_cascade_col_index)) / shadow_atlas.atlas_size;
	return shadow_atlas_ndc_coordinates;
}

void update_test()
{
}

struct Temp_Frame_Info {
	Vector4 planes[6];
};

struct Planes {
	Vector4 planes[6];
};

Planes get_planes()
{
	Planes x;
	x.planes[0] = Vector4(1.0f, 2.0f, 3.0f, 4.0f);
	x.planes[1] = Vector4(1.0f, 2.0f, 3.0f, 4.0f);
	x.planes[2] = Vector4(1.0f, 2.0f, 3.0f, 4.0f);
	x.planes[3] = Vector4(1.0f, 2.0f, 3.0f, 4.0f);
	x.planes[4] = Vector4(1.0f, 2.0f, 3.0f, 4.0f);
	x.planes[5] = Vector4(1.0f, 2.0f, 3.0f, 4.0f);
	return x;
}

void temp_f(float temp[6])
{
	temp[0] = 1.0f;
	temp[1] = 2.0f;
	temp[2] = 3.0f;
	temp[3] = 4.0f;
	temp[4] = 5.0f;
	temp[5] = 6.0f;
}

#include "../libs/structures/sparse_set.h"

template <typename T>
void print_sparse_set(Sparse_Set<T> *sparse_set)
{
	print("------------Print Sparse Array-------------");
	for (u32 i = 0; i < sparse_set->sparse_array.count; i++) {
		print("[{}] dense index {}", i, sparse_set->sparse_array[i]);
	}

	for (u32 i = 0; i < sparse_set->dense_array.count; i++) {
		print("[{}] sparse index {}, data {}", i, sparse_set->dense_array[i].sparse_index, sparse_set->dense_array[i].value);
	}
}

void test()
{
	Sparse_Set<String> temp;
	//temp.push("C++11");
	//temp.push("C++17");
	//temp.push("C++20");
	//temp.push("Python");
	//temp.push("Jave");
	temp.push("Rust");
	temp.push("Jai");
	temp.push("Lua");

	print_sparse_set(&temp);
	temp.remove(0);
	print_sparse_set(&temp);
	temp.remove(1);
	print_sparse_set(&temp);
	temp.remove(2);
	print_sparse_set(&temp);

	//temp.push("Jave");
	u32 i0 = temp.push("Rust");
	u32 i1 = temp.push("Jai");
	u32 i2 = temp.push("Lua");

	String s1 = temp.get_sparse(i0);
	String s2 = temp.get_sparse(i1);
	String s3 = temp.get_sparse(i2);

	String s11 = temp.get_dense(0);
	String s22 = temp.get_dense(1);
	String s33 = temp.get_dense(2);

	print_sparse_set(&temp);

	int x = 0;
}
