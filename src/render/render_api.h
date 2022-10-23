#ifndef DX11_RENDER_API_H
#define DX11_RENDER_API_H

#include <stdlib.h>
#include <d3d11.h>

#include "../win32/win_types.h"
#include "../libs/str.h"
#include "../libs/ds/hash_table.h"
#include "../libs/color.h"


struct Gpu_Device;
struct Render_Pipeline;

typedef ID3D11SamplerState Texture_Sampler;
typedef ID3D11ShaderResourceView Shader_Resource_View;
typedef ID3D11Resource  Gpu_Resource;
typedef ID3D11InputLayout Input_Layout;
typedef ID3D11RasterizerState Rasterizer;
typedef ID3D11DepthStencilState Depth_Stencil_Test;
typedef ID3D11BlendState Blending_Test;

typedef ID3D11VertexShader   Vertex_Shader;
typedef ID3D11GeometryShader Geometry_Shader;
typedef ID3D11ComputeShader  Compute_Shader;
typedef ID3D11HullShader     Hull_Shader;
typedef ID3D11DomainShader   Domain_Shader;
typedef ID3D11PixelShader    Pixel_Shader;

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


enum Resource_Usage {
	RESOURCE_USAGE_DEFAULT ,
	RESOURCE_USAGE_IMMUTABLE,
	RESOURCE_USAGE_DYNAMIC,
	RESOURCE_USAGE_STAGING
};

struct Gpu_Buffer_Desc {
	void *data = NULL;
	
	u32 data_count = 0;
	u32 data_size = 0;
	
	u32 bind_flags = 0;
	u32 cpu_access = 0;

	Resource_Usage usage;
};

struct Gpu_Buffer {
	Gpu_Buffer() {};
	~Gpu_Buffer();

	ID3D11Buffer *buffer = NULL;

	u32 data_size = 0;
	u32 data_count = 0;

	u32 get_data_width();
	ID3D11Buffer **get_buffer_ptr();
};


struct Vertex_Buffer_Desc : Gpu_Buffer_Desc {
	Vertex_Buffer_Desc(u32 _data_count, u32 _data_size, void *_data, Resource_Usage _usage = RESOURCE_USAGE_DEFAULT, u32 _cpu_access = 0);
};

struct Index_Buffer_Desc : Gpu_Buffer_Desc {
	Index_Buffer_Desc(u32 _data_count, void *_data, Resource_Usage _usage = RESOURCE_USAGE_DEFAULT, u32 _cpu_access = 0);
};

struct Rasterizer_Desc {
	Rasterizer_Desc();
	
	D3D11_RASTERIZER_DESC desc;

	void set_sciccor(bool state);
	void set_counter_clockwise(bool state);
};

enum Blend_Option {
	BLEND_ZERO ,
	BLEND_ONE,
	BLEND_SRC_COLOR,
	BLEND_INV_SRC_COLOR,
	BLEND_SRC_ALPHA,
	BLEND_INV_SRC_ALPHA,
	BLEND_DEST_ALPHA,
	BLEND_INV_DEST_ALPHA,
	BLEND_DEST_COLOR,
	BLEND_INV_DEST_COLOR,
	BLEND_SRC_ALPHA_SAT,
	BLEND_BLEND_FACTOR,
	BLEND_INV_BLEND_FACTOR,
	BLEND_SRC1_COLOR,
	BLEND_INV_SRC1_COLOR,
	BLEND_SRC1_ALPHA,
	BLEND_INV_SRC1_ALPHA
};

enum Blend_Operation {
	BLEND_OP_ADD,
	BLEND_OP_SUBTRACT,
	BLEND_OP_REV_SUBTRACT,
	BLEND_OP_MIN,
	BLEND_OP_MAX
};

struct Blending_Test_Desc {
	Blending_Test_Desc();

	bool enable;
	Blend_Option src;
	Blend_Option dest;
	Blend_Operation blend_op;
	Blend_Option src_alpha;
	Blend_Option dest_alpha;
	Blend_Operation blend_op_alpha;
	u8 write_mask = D3D11_COLOR_WRITE_ENABLE_ALL;
};

enum Stencil_Operation {
	STENCIL_OP_KEEP,
	STENCIL_OP_ZERO,
	STENCIL_OP_REPLACE,
	STENCIL_OP_INCR_SAT,
	STENCIL_OP_DECR_SAT,
	STENCIL_OP_INVERT,
	STENCIL_OP_INCR,
	STENCIL_OP_DECR
};
enum Comparison_Func {
	COMPARISON_NEVER,
	COMPARISON_LESS,
	COMPARISON_EQUAL,
	COMPARISON_LESS_EQUAL,
	COMPARISON_GREATER,
	COMPARISON_NOT_EQUAL,
	COMPARISON_GREATER_EQUAL,
	COMPARISON_ALWAYS 
};

struct Depth_Stencil_Test_Desc {
	Depth_Stencil_Test_Desc() {};
	Depth_Stencil_Test_Desc(Stencil_Operation _stencil_failed, Stencil_Operation _depth_failed, Stencil_Operation _pass, Comparison_Func _compare_func, u32 _write_mask = 0xff, u32 _read_mask = 0xff, bool _enable_depth_test = true);

	bool enable_depth_test = true;
	bool enalbe_stencil_test = false;
	u32 stencil_read_mask = 0xff;
	u32 stencil_write_mack = 0xff;
	Stencil_Operation stencil_failed = STENCIL_OP_KEEP;
	Stencil_Operation depth_failed = STENCIL_OP_KEEP;
	Stencil_Operation pass = STENCIL_OP_KEEP;
	Comparison_Func compare_func = COMPARISON_ALWAYS;
};

enum Shader_Type {
	VERTEX_SHADER,
	GEOMETRY_SHADER,
	COMPUTE_SHADER,
	HULL_SHADER,
	DOMAIN_SHADER,
	PIXEL_SHADER,
};

struct Shader {
	Shader() {};
	~Shader();

	Vertex_Shader *vertex_shader = NULL;
	Geometry_Shader *geometry_shader = NULL;
	Compute_Shader *compute_shader = NULL;
	Hull_Shader *hull_shader = NULL;
	Domain_Shader *domain_shader = NULL;
	Pixel_Shader *pixel_shader = NULL;

	u8 *byte_code = NULL;
	u32 byte_code_size = 0;

	String name;
};

struct Texture {
	Texture() {}
	~Texture();

	u32 width;
	u32 height;
	u32 format_size;

	Gpu_Resource *gpu_resource = NULL;
	Shader_Resource_View *shader_resource = NULL;

	String name;

	u32 get_row_pitch();
};

u32 *create_color_buffer(u32 width, u32 height, const Color &color);

struct Gpu_Device {
	u32 quality_levels = 0;
	ID3D11Device *device = NULL;
	
	static Input_Layout *vertex_xc;
	static Input_Layout *vertex_xnuv;
	static Input_Layout *vertex_xuv;
	
	void shutdown();

	void create_input_layouts(Hash_Table<String, Shader *> &shaders);
	void create_shader(u8 *byte_code, u32 byte_code_size, Shader_Type shader_type, Shader *shader);
	
	Gpu_Buffer *create_gpu_buffer(Gpu_Buffer_Desc *desc);
	Gpu_Buffer *create_constant_buffer(u32 buffer_size);

	Texture_Sampler *create_sampler();
	Texture *create_texture_2d(u32 width, u32 height, void *data = NULL, u32 mip_levels = 0, Resource_Usage usage = RESOURCE_USAGE_DEFAULT, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

	Rasterizer *create_rasterizer(Rasterizer_Desc *rasterizer_desc);
	Blending_Test *create_blending_test(Blending_Test_Desc *blending_desc);
	Depth_Stencil_Test *create_depth_stencil_test(Depth_Stencil_Test_Desc *depth_stencil_desc);
};

enum Render_Primitive_Type {
	RENDER_PRIMITIVE_TRIANGLES,
	RENDER_PRIMITIVE_LINES,
};

struct Render_Pipeline {
	IDXGISwapChain *swap_chain = NULL;
	ID3D11DeviceContext *pipeline = NULL;

	//@Note can I these fields not use in this struct ?
	ID3D11RenderTargetView *render_target_view = NULL;
	ID3D11DepthStencilView *depth_stencil_view = NULL;
	ID3D11Texture2D *depth_stencil_buffer = NULL;

	void resize(Gpu_Device *gpu_device, u32 window_width, u32 window_height);
	void shutdown();
	
	void *map(Gpu_Buffer *gpu_buffer);
	void unmap(Gpu_Buffer *gpu_buffer);

	void update_constant_buffer(Gpu_Buffer *gpu_buffer, void *data);
	void update_subresource(Texture *gpu_resource, void *source_data, u32 row_pitch, Rect_u32 *rect = NULL);
	void generate_mips(Shader_Resource_View *shader_resource);
	
	void set_input_layout(Input_Layout *input_layout);
	void set_primitive(Render_Primitive_Type primitive_type);
	void set_vertex_buffer(Gpu_Buffer *gpu_buffer);
	void set_index_buffer(Gpu_Buffer *gpu_buffer);

	void set_vertex_shader(Shader *shader);
	void set_geometry_shader(Shader *shader);
	void set_computer_shader(Shader *shader);
	void set_hull_shader(Shader *shader);
	void set_domain_shader(Shader *shader);
	void set_pixel_shader(Shader *shader);

	void set_veretex_shader_resource(Gpu_Buffer *constant_buffer);
	void set_pixel_shader_sampler(Texture_Sampler *sampler);
	void set_pixel_shader_resource(Gpu_Buffer *constant_buffer);
	void set_pixel_shader_resource(Shader_Resource_View *shader_resource_view);

	void set_rasterizer(Rasterizer *rasterizer);
	void set_scissor(Rect_s32 *rect);
	void reset_rasterizer();

	void set_blending_text(Blending_Test *blending_test);
	void reset_blending_test();

	void set_depth_stencil_test(Depth_Stencil_Test *depth_stencil_test, u32 stencil_ref = 0);
	void reset_depth_stencil_test();

	void draw_indexed(u32 index_count, u32 index_offset, u32 vertex_offset);
};

void init_render_api(Gpu_Device *gpu_device, Render_Pipeline *render_pipeline, Win32_State *win32_state);
u32 *r8_to_rgba32(u8 *data, u32 width, u32 height);

#endif