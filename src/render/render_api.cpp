#include "render_api.h"
#include "../sys/sys_local.h"

Gpu_Buffer::Gpu_Buffer(Gpu_Device *_device)
{
	device = _device;
}

Gpu_Buffer::~Gpu_Buffer()
{
	DELETE_PTR(data);
	RELEASE_COM(buffer);
}

void Gpu_Buffer::set(u32 _data_size, u32 _data_count, void *_data, RESOURCE_USAGE _usage, u32 _bind_flags, u32 _cpu_access)
{
	data_size = _data_size;
	data_count = _data_count;
	data = _data;
	usage = _usage;
	bind_flags = _bind_flags;
	cpu_access = _cpu_access;
}

void Gpu_Buffer::map()
{
}

void Gpu_Buffer::unmap()
{
}

void Gpu_Buffer::bind_to_pipeline()
{
}

void Gpu_Buffer::create_buffer()
{
	device->create_gpu_buffer(this);
}

void Gpu_Buffer::update(void *source_data, u32 data_size)
{
	void *dst_data = pipeline->map(this);

	memcpy(dst_data, source_data, data_size);

	pipeline->ummap(this);
}

void Gpu_Device::create_gpu_buffer(Gpu_Buffer *gpu_buffer)
{
	assert(gpu_buffer->data_count > 0);
	assert(gpu_buffer->data_size > 0);

	D3D11_BUFFER_DESC buffer_desc;
	ZeroMemory(&buffer_desc, sizeof(D3D11_BUFFER_DESC));
	buffer_desc.Usage = to_dx11_resource_usage(gpu_buffer->usage);
	buffer_desc.BindFlags = gpu_buffer->bind_flags;
	buffer_desc.ByteWidth = gpu_buffer->get_data_width();
	buffer_desc.CPUAccessFlags = gpu_buffer->cpu_access;

	if (gpu_buffer->data) {
		D3D11_SUBRESOURCE_DATA resource_data_desc;
		ZeroMemory(&resource_data_desc, sizeof(D3D11_SUBRESOURCE_DATA));
		resource_data_desc.pSysMem = (void *)gpu_buffer->data;
		HR(device->CreateBuffer(&buffer_desc, &resource_data_desc, gpu_buffer->get_buffer_ptr()));
	} else {
		HR(device->CreateBuffer(&buffer_desc, NULL, gpu_buffer->get_buffer_ptr()));
	}
}

void Gpu_Device::create_shader()
{
}

void Gpu_Device::create_stencil_text()
{
}

void *Render_Pipeline::map(Gpu_Buffer *gpu_buffer)
{
	D3D11_MAPPED_SUBRESOURCE subresource;
	HR(pipeline->Map(gpu_buffer->buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource));
	return subresource.pData;
}

void Render_Pipeline::ummap(Gpu_Buffer *gpu_buffer)
{
	pipeline->Unmap(gpu_buffer->buffer, 0);
}

void Render_Pipeline::set_vertex_buffer(Gpu_Buffer *gpu_buffer)
{
	u32 strides = gpu_buffer->data_size;
	u32 offsets = 0;
	pipeline->IASetVertexBuffers(0, 0, gpu_buffer->get_buffer_ptr(), &strides, &offsets);
}

void Render_Pipeline::set_index_buffer(Gpu_Buffer *gpu_buffer)
{
	//pipeline->IASetIndexBuffer(gpu_buffer->buffer, 0, 0);
}

void Render_Pipeline::set_shader(Shader *shader)
{
	if (shader->vertex_shader) {
		pipeline->VSSetShader(shader->vertex_shader, 0, 0);
	}

	if (shader->geometry_shader) {
		pipeline->GSSetShader(shader->geometry_shader, 0, 0);
	}

	if (shader->compute_shader) {
		pipeline->CSSetShader(shader->compute_shader, 0, 0);
	}

	if (shader->hull_shader) {
		pipeline->HSSetShader(shader->hull_shader, 0, 0);
	}

	if (shader->domain_shader) {
		pipeline->DSSetShader(shader->domain_shader, 0, 0);
	}

	if (shader->pixel_shader) {
		pipeline->PSSetShader(shader->pixel_shader, 0, 0);
	}
}

void Render_Pipeline::set_vertex_shader(Shader *shader)
{
	if (!shader->vertex_shader) {
		print("Vertex shader with name [] is NULL, the shader can not be set to render pipeline");
		return;
	}
	pipeline->VSSetShader(shader->vertex_shader, 0, 0);
}

void Render_Pipeline::set_geometry_shader(Shader * shader)
{
}

void Render_Pipeline::set_computer_shader(Shader * shader)
{
}

void Render_Pipeline::set_hull_shader(Shader * shader)
{
}

void Render_Pipeline::set_domain_shader(Shader * shader)
{
}

void Render_Pipeline::set_pixel_shader(Shader * shader)
{
}
