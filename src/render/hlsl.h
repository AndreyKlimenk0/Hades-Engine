#ifndef HADES_ENGINE_HLSL_H
#define HADES_ENGINE_HLSL_H

#include "../win32/win_types.h"
#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"

typedef float   Pad1;
typedef Vector2 Pad2;
typedef Vector3 Pad3;
typedef Vector4 Pad4;

const u32 CB_PASS_DATA_REGISTER = 0;
const u32 CB_RENDER_2D_INFO_REGISTER = 4;
const u32 CB_FRAME_INFO_REGISTER = 5;
const u32 CB_SHADOW_ATLAS_INFO_REGISTER = 6;

const u32 JITTERING_SAMPLES_TEXTURE_REGISTER = 2;

const u32 POINT_SAMPLING_REGISTER = 0;
const u32 LINEAR_SAMPLING_REGISTER = 1;

struct CB_Render_2d_Info {
	Matrix4 position_orthographic_matrix;
	Vector4 color;
};

struct CB_Frame_Info {
	Matrix4 view_matrix;
	Matrix4 perspective_matrix;
	Matrix4 orthographic_matrix;
	Vector4 camera_position;
	Vector4 camera_direction;
	float near_plane;
	float far_plane;
	u32 light_count;
	Pad1 pad;
};

struct CB_Shadow_Atlas_Info {
	u32 shadow_atlas_size;
	u32 shadow_cascade_size;
	u32 jittering_sampling_tile_size;
	u32 jittering_sampling_filter_size;
	u32 jittering_sampling_scaling;
	Pad3 pad;
};

struct Hlsl_Light {
	Vector4 position;
	Vector4 direction;
	Vector4 color;
	float radius;
	float range;
	u32 light_type;
	u32 shadow_map_idx;
};

struct Cascaded_Shadows_Info {
	Vector3 light_direction;
	u32 shadow_map_start_index;
	u32 shadow_map_end_index;
};

#endif