#include "renderers.h"
#include "render_api/base_structs.h"

#include "../sys/utils.h"

void Primitive_Renderer::init(Render_Device *_render_device)
{
    render_device = _render_device;
}

Primitive_ID Primitive_Renderer::add_aabb(AABB *aabb)
{
    upload_data = true;

	Primitive_Draw_Command draw_command;
    draw_command.primitive_type = PRIMITIVE_TYPE_LINE;
	draw_command.vertex_count = 24;
	draw_command.vertex_offset = vertices.count;
	draw_command.color = Color::Red;
	draw_command.world_matrix = make_identity_matrix();

    Vector3 min = aabb->min;
    Vector3 max = aabb->max;
    
    vertices.push(Vector3(min.x, min.y, min.z));
    vertices.push(Vector3(max.x, min.y, min.z));
    vertices.push(Vector3(max.x, min.y, min.z));
    vertices.push(Vector3(max.x, max.y, min.z));
    vertices.push(Vector3(max.x, max.y, min.z));
    vertices.push(Vector3(min.x, max.y, min.z));
    vertices.push(Vector3(min.x, max.y, min.z));
    vertices.push(Vector3(min.x, min.y, min.z));
    vertices.push(Vector3(min.x, min.y, max.z));
    vertices.push(Vector3(max.x, min.y, max.z));
    vertices.push(Vector3(max.x, min.y, max.z));
    vertices.push(Vector3(max.x, max.y, max.z));
    vertices.push(Vector3(max.x, max.y, max.z));
    vertices.push(Vector3(min.x, max.y, max.z));
    vertices.push(Vector3(min.x, max.y, max.z));
    vertices.push(Vector3(min.x, min.y, max.z));
    vertices.push(Vector3(min.x, min.y, min.z));
    vertices.push(Vector3(min.x, min.y, max.z));
    vertices.push(Vector3(max.x, min.y, min.z));
    vertices.push(Vector3(max.x, min.y, max.z));
    vertices.push(Vector3(max.x, max.y, min.z));
    vertices.push(Vector3(max.x, max.y, max.z));
    vertices.push(Vector3(min.x, max.y, min.z));
    vertices.push(Vector3(min.x, max.y, max.z));

    return draw_commands.push(draw_command);
}

void Primitive_Renderer::prepare_for_rendering()
{
    if (upload_data) {
        if (!vertex_buffer || (vertex_buffer->count() < (u64)vertices.count)) {
            DELETE_PTR(vertex_buffer);
            Buffer_Desc buffer_desc;
            buffer_desc.size = vertices.get_size();
            buffer_desc.stride = vertices.stride;
            buffer_desc.data = vertices.to_void_ptr();

            vertex_buffer = render_device->create_buffer(&buffer_desc);
        } else {
            vertex_buffer->request_write();
            vertex_buffer->write(vertices.to_void_ptr(), vertices.get_size());
        }
    }
    upload_data = false;
}

void Primitive_Renderer::remove_primitive(Primitive_ID id)
{
    draw_commands.remove(id);
}

void Primitive_Renderer::set_transformation_matrix(Primitive_ID id, const Matrix4 &transformation_matrx)
{
    draw_commands.get_sparse(id).world_matrix = transformation_matrx;
}
