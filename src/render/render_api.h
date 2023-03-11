#ifndef DX11_RENDER_API_H
#define DX11_RENDER_API_H

#include <stdlib.h>
#include <d3d11.h>

#include "../win32/win_types.h"
#include "../libs/str.h"
#include "../libs/ds/hash_table.h"
#include "../libs/color.h"

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

typedef ComPtr<ID3D11Device> Dx11_Device;
typedef ComPtr<ID3D11DeviceContext> Dx11_Device_Context;
typedef ComPtr<IDXGISwapChain> DXGI_Swap_Chain;
typedef ComPtr<ID3D11Debug> Dx11_Debug;

typedef ComPtr<ID3D11RenderTargetView> Render_Target_View;
typedef ComPtr<ID3D11DepthStencilView> Depth_Stencil_View;
typedef ComPtr<ID3D11ShaderResourceView> Shader_Resource_View;

typedef ComPtr<ID3D11InputLayout> Input_Layout;
typedef ComPtr<ID3D11RasterizerState> Rasterizer_State;
typedef ComPtr<ID3D11DepthStencilState> Depth_Stencil_State;
typedef ComPtr<ID3D11BlendState> Blend_State;
typedef ComPtr<ID3D11SamplerState> Sampler_State;

typedef ComPtr<ID3D11VertexShader> Vertex_Shader;
typedef ComPtr<ID3D11GeometryShader> Geometry_Shader;
typedef ComPtr<ID3D11ComputeShader> Compute_Shader;
typedef ComPtr<ID3D11HullShader> Hull_Shader;
typedef ComPtr<ID3D11DomainShader> Domain_Shader;
typedef ComPtr<ID3D11PixelShader> Pixel_Shader;

typedef ComPtr<ID3D11Resource> Dx11_Resource;
typedef ComPtr<ID3D11Texture2D> Dx11_Texture_2D;
typedef ComPtr<ID3D11Buffer> Dx11_Buffer;

struct Struct_Buffer;
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

const u32 RESOURCE_MISC_GENERATE_MIPS = 0x1L;
const u32 RESOURCE_MISC_SHARED = 0x2L;
const u32 RESOURCE_MISC_TEXTURECUBE = 0x4L;
const u32 RESOURCE_MISC_DRAWINDIRECT_ARGS = 0x10L;
const u32 RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS = 0x20L;
const u32 RESOURCE_MISC_BUFFER_STRUCTURED = 0x40L;
const u32 RESOURCE_MISC_RESOURCE_CLAMP = 0x80L;
const u32 RESOURCE_MISC_SHARED_KEYEDMUTEX = 0x100L;
const u32 RESOURCE_MISC_GDI_COMPATIBLE = 0x200L;
const u32 RESOURCE_MISC_SHARED_NTHANDLE = 0x800L;
const u32 RESOURCE_MISC_RESTRICTED_CONTENT = 0x1000L;
const u32 RESOURCE_MISC_RESTRICT_SHARED_RESOURCE = 0x2000L;
const u32 RESOURCE_MISC_RESTRICT_SHARED_RESOURCE_DRIVER = 0x4000L;
const u32 RESOURCE_MISC_GUARDED = 0x8000L;
const u32 RESOURCE_MISC_TILE_POOL = 0x20000L;
const u32 RESOURCE_MISC_TILED = 0x40000L;
const u32 RESOURCE_MISC_HW_PROTECTED = 0x80000L;

enum Resource_Usage {
	RESOURCE_USAGE_DEFAULT ,
	RESOURCE_USAGE_IMMUTABLE,
	RESOURCE_USAGE_DYNAMIC,
	RESOURCE_USAGE_STAGING
};

template <typename T>
struct Gpu_Resource {
	Gpu_Resource() {}
	ComPtr<T> gpu_resource;
};

struct Gpu_Buffer_Desc {
	void *data = NULL;
	
	u32 data_count = 0;
	u32 data_size = 0;
	
	u32 bind_flags = 0;
	u32 cpu_access = 0;
	u32 misc_flags = 0;
	u32 struct_size = 0;
	
	Resource_Usage usage;
};

struct Gpu_Buffer : Gpu_Resource<ID3D11Buffer> {
	Gpu_Buffer() : Gpu_Resource<ID3D11Buffer>() {};

	u32 data_size = 0;
	u32 data_count = 0;

	void free();
	bool is_empty() { return data_count == 0; }
	u32 get_data_width();
};

Gpu_Buffer_Desc make_vertex_buffer_desc(u32 data_count, u32 data_size, void *data, Resource_Usage usage = RESOURCE_USAGE_DEFAULT, u32 cpu_access = 0);
Gpu_Buffer_Desc make_index_buffer_desc(u32 data_count, void *data, Resource_Usage usage = RESOURCE_USAGE_DEFAULT, u32 cpu_access = 0);


struct Rasterizer_Desc {
	Rasterizer_Desc();
	
	D3D11_RASTERIZER_DESC desc;

	void set_sciccor(bool state);
	void set_counter_clockwise(bool state);
};

enum Blend_Option {
	BLEND_ZERO,
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

struct Blend_State_Desc {
	Blend_State_Desc();

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

struct Depth_Stencil_State_Desc {
	Depth_Stencil_State_Desc() {};
	Depth_Stencil_State_Desc(Stencil_Operation _stencil_failed, Stencil_Operation _depth_failed, Stencil_Operation _pass, Comparison_Func _compare_func, u32 _write_mask = 0xff, u32 _read_mask = 0xff, bool _enable_depth_test = true);

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

	Vertex_Shader vertex_shader;
	Geometry_Shader geometry_shader;
	Compute_Shader compute_shader;
	Hull_Shader hull_shader;
	Domain_Shader domain_shader;
	Pixel_Shader pixel_shader;

	u8 *byte_code = NULL;
	u32 byte_code_size = 0;

	String name;
};

enum Shader_Resource_Type {
	SHADER_RESOURCE_TYPE_UNKNOWN,
	SHADER_RESOURCE_TYPE_BUFFER ,
	SHADER_RESOURCE_TYPE_TEXTURE_1D,
	SHADER_RESOURCE_TYPE_TEXTURE_1D_ARRAY,
	SHADER_RESOURCE_TYPE_TEXTURE_2D,
	SHADER_RESOURCE_TYPE_TEXTURE_2D_ARRAY,
	SHADER_RESOURCE_TYPE_TEXTURE_2D_MS,
	SHADER_RESOURCE_TYPE_TEXTURE_2D_MS_ARRAY,
	SHADER_RESOURCE_TYPE_TEXTURE_3D,
	SHADER_RESOURCE_TYPE_TEXTURE_CUBE,
	SHADER_RESOURCE_TYPE_TEXTURE_CUBE_ARRAY,
	SHADER_RESOURCE_TYPE_BUFFEREX
};

struct Shader_Resource_Desc {
	Shader_Resource_Desc();

	DXGI_FORMAT format;
	Shader_Resource_Type resource_type;
	union {
		struct Buffer {
			u32 first_element;
			u32 element_count;
		} buffer;
		struct Dx11_Texture_2D {
			u32 most_detailed_mip;
			u32 mip_levels;
		} texture_2d;
		struct Texture_2D_Array {
			u32 count;
			u32 most_detailed_mip;
			u32 mip_levels;
		} texture_2d_array;
	} resource;
};

enum Depth_Stencil_View_Type {
	DEPTH_STENCIL_VIEW_TYPE_UNKNOWN,
	DEPTH_STENCIL_VIEW_TYPE_TEXTURE_1D,
	DEPTH_STENCIL_VIEW_TYPE_TEXTURE_1D_ARRAY,
	DEPTH_STENCIL_VIEW_TYPE_TEXTURE_2D,
	DEPTH_STENCIL_VIEW_TYPE_TEXTURE_2D_ARRAY,
	DEPTH_STENCIL_VIEW_TYPE_TEXTURE_2D_MS,
	DEPTH_STENCIL_VIEW_TYPE_TEXTURE_2D_MS_ARRAY
};

struct Depth_Stencil_View_Desc {
	DXGI_FORMAT format;
	Depth_Stencil_View_Type type;
	union {
		struct Dx11_Texture_2D {
			u32 mip_slice;
		} texture_2d;
		struct Texture_2D_Array {
			u32 mip_slice;
			u32 first_array_slice;
			u32 array_count;
		} texture_2d_array;
	} view;
};

struct Texture_Desc {
	u32 width;
	u32 height; 
	u32 array_count = 1;
	u32 mip_levels = 0;
	u32 cpu_access = 0;
	u32 bind = BIND_SHADER_RESOURCE;
	void *data = NULL; 
	Resource_Usage usage = RESOURCE_USAGE_DEFAULT;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
};

struct Texture2D : Gpu_Resource<ID3D11Texture2D> {
	Texture2D() {}

	u32 width = 0;
	u32 height = 0;
	u32 format_size = 0;
	
	String name;
	
	Shader_Resource_View shader_resource;

	u32 get_row_pitch();
};

u32 *create_color_buffer(u32 width, u32 height, const Color &color);

struct Gpu_Device {
	u32 quality_levels = 0;
	Dx11_Device device;
	Dx11_Debug debug;
	
	static Input_Layout vertex_xc;
	static Input_Layout vertex_xnuv;
	static Input_Layout vertex_xuv;

	void create_input_layouts(Hash_Table<String, Shader *> &shader_table);
	void create_shader(u8 *byte_code, u32 byte_code_size, Shader_Type shader_type, Shader *shader);
	
	void create_gpu_buffer(Gpu_Buffer_Desc *desc, Gpu_Buffer *buffer);
	void create_constant_buffer(u32 buffer_size, Gpu_Buffer *buffer);

	void create_sampler(Sampler_State *sampler_state);
	void create_texture_2d(Texture_Desc *texture_desc, Texture2D *texture);
	void create_texture_2d(Texture_Desc *texture_desc, Shader_Resource_Desc *shader_resource_desc, Texture2D *texture);

	void create_depth_stencil_view(Texture2D *texture, Depth_Stencil_View_Desc *depth_stencil_view_desc, Depth_Stencil_View *depth_stencil_view);

	void create_shader_resource_view(Texture2D *texture, Shader_Resource_Desc *shader_resource_desc, Shader_Resource_View *shader_resource);
	void create_shader_resource_view(Gpu_Buffer *gpu_buffer, Shader_Resource_Desc *shader_resource_desc, Shader_Resource_View *shader_resource);
	void create_shader_resource_view(const Dx11_Resource &gpu_resource, Shader_Resource_Desc *shader_resource_desc, Shader_Resource_View *shader_resource);

	void create_rasterizer_state(Rasterizer_Desc *rasterizer_desc, Rasterizer_State *rasterizer_state);
	void create_blend_state(Blend_State_Desc *blending_desc, Blend_State *blend_state);
	void create_depth_stencil_state(Depth_Stencil_State_Desc *depth_stencil_desc, Depth_Stencil_State *depth_stencil_state);
};

struct Render_Pipeline_States {
	static Rasterizer_State default_rasterizer_state;
	static Depth_Stencil_State default_depth_stencil_state;
	static Depth_Stencil_State disabled_depth_test;
	static Blend_State default_blend_state;
	static Sampler_State default_sampler_state;

	static void init(Gpu_Device *gpu_device);
};

enum Render_Primitive_Type {
	RENDER_PRIMITIVE_TRIANGLES,
	RENDER_PRIMITIVE_LINES,
};

enum Map_Type {
	MAP_TYPE_READ,
	MAP_TYPE_WRITE,
	MAP_TYPE_READ_WRITE,
	MAP_TYPE_WRITE_DISCARD,
	MAP_TYPE_MAP_WRITE_NO_OVERWRITE
};

struct Render_Pipeline_State {
	Render_Primitive_Type primitive_type;
	String shader_name;
	Shader *shader = NULL;
	Blend_State blend_state;
	Depth_Stencil_State depth_stencil_state;
	Rasterizer_State rasterizer_state;
	Sampler_State sampler_state;

	bool setup(Render_System *render_sys);
};

struct Render_Pipeline {
	DXGI_Swap_Chain swap_chain;
	Dx11_Device_Context pipeline;

	//@Note can I these fields not use in this struct ?
	Dx11_Texture_2D depth_stencil_texture;
	Render_Target_View render_target_view;
	Depth_Stencil_View depth_stencil_view;

	void resize(Gpu_Device *gpu_device, u32 window_width, u32 window_height);
	void shutdown();

	void copy_resource(Gpu_Buffer *dst_buffer, Gpu_Buffer *src_buffer);

	void apply(Render_Pipeline_State *render_pipeline_state);
	
	void *map(Gpu_Buffer *gpu_buffer, Map_Type map_type = MAP_TYPE_WRITE_DISCARD);
	void unmap(Gpu_Buffer *gpu_buffer);

	void update_constant_buffer(Gpu_Buffer *gpu_buffer, void *data);
	void update_subresource(Texture2D *gpu_resource, void *source_data, u32 row_pitch, Rect_u32 *rect = NULL);
	void generate_mips(const Shader_Resource_View &shader_resource);
	
	//@Note: may be this should be removed from code.
	void set_input_layout(void *pointer);
	void set_input_layout(const Input_Layout &input_layout);
	void set_primitive(Render_Primitive_Type primitive_type);
	void set_vertex_buffer(Gpu_Buffer *gpu_buffer);
	void set_index_buffer(Gpu_Buffer *gpu_buffer);
	void reset_vertex_buffer();
	void reset_index_buffer();

	void set_vertex_shader(Shader *shader);
	void set_geometry_shader(Shader *shader);
	void set_computer_shader(Shader *shader);
	void set_hull_shader(Shader *shader);
	void set_domain_shader(Shader *shader);
	void set_pixel_shader(Shader *shader);

	void set_vertex_shader_resource(u32 gpu_register, const Gpu_Buffer &constant_buffer);
	void set_vertex_shader_resource(u32 gpu_register, const Shader_Resource_View &shader_resource);
	void set_vertex_shader_resource(u32 shader_resource_register, const Struct_Buffer &struct_buffer);
	
	void set_pixel_shader_sampler(const Sampler_State &sampler_state);
	void set_pixel_shader_resource(u32 shader_resource_register, const Struct_Buffer &struct_buffer);
	void set_pixel_shader_resource(const Shader_Resource_View &shader_resource_view);
	void set_pixel_shader_resource(u32 gpu_register, const Gpu_Buffer &constant_buffer);

	void set_rasterizer_state(const Rasterizer_State &rasterizer_state);
	void set_scissor(Rect_s32 *rect);
	void reset_rasterizer();

	void set_blend_state(const Blend_State &blend_state);
	void reset_blending_test();

	void set_depth_stencil_state(const Depth_Stencil_State &depth_stencil_state, u32 stencil_ref = 0);
	void reset_depth_stencil_test();

	void draw(u32 vertex_count);
	void draw_indexed(u32 index_count, u32 index_offset, u32 vertex_offset);
};

void init_render_api(Gpu_Device *gpu_device, Render_Pipeline *render_pipeline, Win32_Info *win32_state);
u32 *r8_to_rgba32(u8 *data, u32 width, u32 height);

#endif