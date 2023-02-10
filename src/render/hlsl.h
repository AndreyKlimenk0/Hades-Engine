#ifndef HADES_ENGINE_HLSL_H
#define HADES_ENGINE_HLSL_H

#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"

struct Frame_Info {
	Matrix4 view_matrix;
	Matrix4 perspective_matrix;
	Vector3 camera_position;
	int pad1;
	Vector3 camera_direction;
	int pad2;
	u32 light_count;
	Vector3 pad3;
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