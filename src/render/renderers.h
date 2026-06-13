#ifndef RENDERERS_H
#define RENDERERS_H

#include "../libs/color.h"
#include "../libs/number_types.h"
#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"
#include "../libs/structures/array.h"
#include "../libs/structures/sparse_set.h"
#include "../collision/collision.h"
#include "render_api/render.h"

typedef u32 Primitive_ID;

struct Primitive_Draw_Command {
	Primitive_Type primitive_type = PRIMITIVE_TYPE_UNKNOWN;
	u32 vertex_count = 0;
	u32 vertex_offset = 0;
	Color color;
	Matrix4 world_matrix;
};

struct Primitive_Renderer {
	// Set to true so that the vertex buffer is created on the first frame.
	bool upload_data = true;

	Array<Vector3> vertices;
	Sparse_Set<Primitive_Draw_Command> draw_commands;

	Buffer *vertex_buffer = NULL;
	Render_Device *render_device = NULL;

	void init(Render_Device *_render_device);
	void prepare_for_rendering();

	void remove_primitive(Primitive_ID id);
	void set_transformation_matrix(Primitive_ID id, const Matrix4 &transformation_matrx);

	Primitive_ID add_aabb(AABB *aabb);
	Primitive_ID add_frustum();

};
#endif