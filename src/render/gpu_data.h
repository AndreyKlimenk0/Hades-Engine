#ifndef HADES_ENGINE_HLSL_H
#define HADES_ENGINE_HLSL_H


#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"
#include "../libs/number_types.h"

typedef float   Pad1;
typedef Vector2 Pad2;
typedef Vector3 Pad3;
typedef Vector4 Pad4;

struct GPU_Frame_Info {
	Matrix4 view_matrix;
	Matrix4 perspective_matrix;
	Matrix4 orthographic_matrix;
	Vector3 view_position;
	float near_plane;
	Vector3 view_direction;
	float far_plane;
	u32 light_count;
	Pad3 pad;
};

struct GPU_Shadow_Atlas_Info {
	u32 shadow_atlas_size;
	u32 shadow_cascade_size;
	u32 jittering_sampling_tile_size;
	u32 jittering_sampling_filter_size;
	u32 jittering_sampling_scaling;
	Pad3 pad;
};

struct GPU_Light {
	Vector3 position;
	float radius;
	Vector3 direction;
	float range;
	Vector3 color;
	u32 light_type;
};

struct GPU_Cascaded_Shadows_Info {
	Vector3 light_direction;
	u32 shadow_map_start_index;
	u32 shadow_map_end_index;
};

#endif
