#ifndef HADES_ENGINE_HLSL_H
#define HADES_ENGINE_HLSL_H

#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"

struct Frame_Info {
	Matrix4 view_matrix;
	Matrix4 perspective_matrix;
	Matrix4 orthographic_matrix;
	Vector4 camera_position;
	Vector4 camera_direction;
	float near_plane;
	float far_plane;
	u32 light_count;
	u32 pad;
};


struct Shader_Light {
	Vector4 position;
	Vector4 direction;
	Vector4 color;
	u32 light_type;
	float radius;
	float range;
	float pad;
};

#endif