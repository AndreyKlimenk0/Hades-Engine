#ifndef DX11_RENDER_API_H
#define DX11_RENDER_API_H

#include <stdlib.h>
#include <d3d11.h>

#include "shader.h"
#include "../win32/win_types.h"

struct Gpu_Device;
struct Render_Pipeline;



const u32 BIND_VERTEX_BUFFER = 0x1L;
const u32 BIND_INDEX_BUFFER = 0x2L;
const u32 BIND_CONSTANT_BUFFER = 0x4L;
const u32 BIND_SHADER_RESOURCE = 0x8L;
const u32 BIND_STREAM_OUTPUT = 0x10L;
const u32 BIND_RENDER_TARGET = 0x20L;
const u32 BIND_DEPTH_STENCIL = 0x40L;
const u32 BIND_UNORDERED_ACCESS = 0x80L;
const u32 BIND_DECODER = 0x200L;
const u32 BIND_VIDEO_ENCODER = 0x400L;


const u32 CPU_ACCESS_WRITE = 0x10000L;
const u32 CPU_ACCESS_READ = 0x20000L;


enum RESOURCE_USAGE {
	RESOURCE_USAGE_DEFAULT = 0,
	RESOURCE_USAGE_IMMUTABLE = 1,
	RESOURCE_USAGE_DYNAMIC = 2,
	RESOURCE_USAGE_STAGING = 3
};

D3D11_USAGE to_dx11_resource_usage(RESOURCE_USAGE usage)
{
	switch (usage) {
		case RESOURCE_USAGE_DEFAULT:
			return D3D11_USAGE_DEFAULT;
		case RESOURCE_USAGE_IMMUTABLE:
			return D3D11_USAGE_IMMUTABLE;
		case RESOURCE_USAGE_DYNAMIC:
			return D3D11_USAGE_DYNAMIC;
		case RESOURCE_USAGE_STAGING:
			return D3D11_USAGE_STAGING;
		default:
			assert(false);
			break;
	}
}

struct Gpu_Buffer {
	Gpu_Buffer(Gpu_Device *_device);
	~Gpu_Buffer();
	
	RESOURCE_USAGE usage;

	void *data = NULL;
	Gpu_Device *device = NULL;
	Render_Pipeline *pipeline = NULL;
	ID3D11Buffer *buffer = NULL;

	u32 data_size = 0;
	u32 data_count = 0;
	u32 bind_flags;
	u32 cpu_access;

	void set(u32 _data_size, u32 _data_count, void *_data, RESOURCE_USAGE _usage, u32 _bind_flags, u32 _cpu_access);
	void map();
	void unmap();
	void bind_to_pipeline();
	void create_buffer();
	void update(void *source_data, u32 data_size);

	u32 get_data_width();
	ID3D11Buffer **get_buffer_ptr();
};

u32 Gpu_Buffer::get_data_width()
{
	return data_count * data_size;
}

ID3D11Buffer **Gpu_Buffer::get_buffer_ptr()
{
	return &buffer;
}

struct Gpu_Device {
	ID3D11Device *device = NULL;

	void create_gpu_buffer(Gpu_Buffer *gpu_buffer);
	void create_shader();
	void create_stencil_text();
};


struct Render_Pipeline {
	ID3D11DeviceContext *pipeline = NULL;
	
	void *map(Gpu_Buffer *gpu_buffer);
	void ummap(Gpu_Buffer *gpu_buffer);
	
	void set_input_layout();
	void set_primitive();
	void set_vertex_buffer(Gpu_Buffer *gpu_buffer);
	void set_index_buffer(Gpu_Buffer *gpu_buffer);

	void set_shader(Shader *shader);
	void set_vertex_shader(Shader *shader);
	void set_geometry_shader(Shader *shader);
	void set_computer_shader(Shader *shader);
	void set_hull_shader(Shader *shader);
	void set_domain_shader(Shader *shader);
	void set_pixel_shader(Shader *shader);
};

#endif