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
typedef ComPtr<ID3D11UnorderedAccessView> Unordered_Access_View;

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

struct Gpu_Struct_Buffer;
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

const u32 CLEAR_DEPTH_BUFFER = 0x1L;
const u32 CLEAR_STENCIL_BUFFER = 0x2L;

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
	Gpu_Resource();
	~Gpu_Resource();
	ComPtr<T> resource;

	void release();
	T *get();
};

template<typename T>
inline Gpu_Resource<T>::Gpu_Resource() 
{
}

template<typename T>
inline Gpu_Resource<T>::~Gpu_Resource()
{
	release();
}

template<typename T>
inline void Gpu_Resource<T>::release()
{
	resource.Reset();
}

template<typename T>
inline T *Gpu_Resource<T>::get()
{
	return resource.Get();
}

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

	Shader_Resource_View srv;
	Unordered_Access_View uav;

	void free();
	bool is_empty() { return data_count == 0; }
	u32 get_data_width();
};

struct Viewport {
	u32 x = 0;
	u32 y = 0;
	u32 width = 0;
	u32 height = 0;
	float min_depth = 0.0f;
	float max_depth = 1.0f;
};

struct Rasterizer_Desc {
	Rasterizer_Desc();
	
	D3D11_RASTERIZER_DESC desc;

	//@Note: these methods can be removed 
	// when Rasterizer_Desc will fully wrap D3D11_RASTERIZER_DESC
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
	bool enable_depth_test = true;
	bool enalbe_stencil_test = false;
	u32 stencil_read_mask = 0xff;
	u32 stencil_write_mack = 0xff;
	Stencil_Operation stencil_failed = STENCIL_OP_KEEP;
	Stencil_Operation depth_failed = STENCIL_OP_KEEP;
	Stencil_Operation pass = STENCIL_OP_KEEP;
	Comparison_Func compare_func = COMPARISON_ALWAYS;
};

struct Shader {
	Shader();
	~Shader();

	Vertex_Shader vertex_shader;
	Geometry_Shader geometry_shader;
	Compute_Shader compute_shader;
	Hull_Shader hull_shader;
	Domain_Shader domain_shader;
	Pixel_Shader pixel_shader;
	
	void free();
};

struct Gpu_Resource_Views {
	Gpu_Resource_Views();
	~Gpu_Resource_Views();

	Shader_Resource_View srv;
	Depth_Stencil_View dsv;
	Render_Target_View rtv;
	Unordered_Access_View uav;

	void release();
};

struct Multisample_Info {
	u32 count = 1;
	u32 quality = 0;
};

struct Texture2D_Desc {
	u32 width = 0;
	u32 height = 0;
	u32 array_count = 1;
	u32 mip_levels = 0;
	u32 cpu_access = 0;
	u32 bind = BIND_SHADER_RESOURCE;
	void *data = NULL;
	Resource_Usage usage = RESOURCE_USAGE_DEFAULT;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	Multisample_Info multisampling;
};
struct Texture3D_Desc {
	u32 width = 0;
	u32 height = 0;
	u32 depth = 0;
	u32 mip_levels = 0;
	u32 cpu_access = 0;
	u32 bind = BIND_SHADER_RESOURCE;
	void *data = NULL;
	Resource_Usage usage = RESOURCE_USAGE_DEFAULT;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
};

bool is_multisampled_texture(Texture2D_Desc *texture_desc);
u32 get_texture_size(Texture2D_Desc *texture_desc);
u32 get_texture_pitch(Texture2D_Desc *texture_desc);

struct Texture2D : Gpu_Resource<ID3D11Texture2D>, Gpu_Resource_Views {
	Texture2D();
	~Texture2D();

	void release();
	void get_desc(Texture2D_Desc *texture_desc);
	Texture2D &operator=(const Texture2D &other);
};

struct Texture3D : Gpu_Resource<ID3D11Texture3D>, Gpu_Resource_Views {
	Texture3D();
	~Texture3D();

	void release();
};

struct Gpu_Device {
	Dx11_Device dx11_device;
	Dx11_Debug debug;

	void create_shader(u8 *byte_code, u32 byte_code_size, Vertex_Shader &shader);
	void create_shader(u8 *byte_code, u32 byte_code_size, Geometry_Shader &shader);
	void create_shader(u8 *byte_code, u32 byte_code_size, Compute_Shader &shader);
	void create_shader(u8 *byte_code, u32 byte_code_size, Hull_Shader &shader);
	void create_shader(u8 *byte_code, u32 byte_code_size, Domain_Shader &shader);
	void create_shader(u8 *byte_code, u32 byte_code_size, Pixel_Shader &shader);
	
	void create_gpu_buffer(Gpu_Buffer_Desc *desc, Gpu_Buffer *buffer);
	void create_constant_buffer(u32 buffer_size, Gpu_Buffer *buffer);

	void create_texture_2d(Texture2D_Desc *texture_desc, Texture2D *texture);
	void create_texture_3d(Texture3D_Desc *texture_desc, Texture3D *texture);

	void create_rasterizer_state(Rasterizer_Desc *rasterizer_desc, Rasterizer_State *rasterizer_state);
	void create_blend_state(Blend_State_Desc *blending_desc, Blend_State *blend_state);
	void create_depth_stencil_state(Depth_Stencil_State_Desc *depth_stencil_desc, Depth_Stencil_State *depth_stencil_state);

	void create_shader_resource_view(Gpu_Buffer *gpu_buffer);
	void create_shader_resource_view(Texture2D_Desc *texture_desc, Texture2D *texture);
	void create_shader_resource_view(Texture3D_Desc *texture_desc, Texture3D *texture);
	void create_depth_stencil_view(Texture2D_Desc *texture_desc, Texture2D *texture);
	void create_unordered_access_view(Texture2D_Desc *texture_desc, Texture2D *texture);
	void create_render_target_view(Texture2D *texture);
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
	Shader *shader = NULL;
	Blend_State blend_state;
	Depth_Stencil_State depth_stencil_state;
	Rasterizer_State rasterizer_state;
	Sampler_State sampler_state;
	Viewport view_port;
	Depth_Stencil_View depth_stencil_view;
	Render_Target_View render_target_view;

	void setup_default_state(Render_System *render_sys);
};

struct Swap_Chain {
	Multisample_Info multisampling;
	DXGI_Swap_Chain dxgi_swap_chain;

	void init(Gpu_Device *gpu_device, Win32_Info *win32_info);
	void resize(u32 window_width, u32 window_height);
	void get_back_buffer_as_texture(Texture2D *texture);
};

struct Render_Pipeline {
	Dx11_Device_Context dx11_context;

	template <typename T>
	void copy_resource(const Gpu_Resource<T> &dst, const Gpu_Resource<T> &src);
	
	template <typename T>
	void copy_subresource(const Gpu_Resource<T> &dst, u32 dst_x, u32 dst_y, const Gpu_Resource<T> &src);

	void apply(Render_Pipeline_State *render_pipeline_state);
	
	template <typename T>
	void *map(Gpu_Resource<T> &resource, Map_Type map_type = MAP_TYPE_WRITE_DISCARD);
	template <typename T>
	void unmap(Gpu_Resource<T> &resource);

	void update_constant_buffer(Gpu_Buffer *gpu_buffer, void *data);
	void update_subresource(Texture2D *resource, void *source_data, u32 row_pitch, Rect_u32 *rect = NULL);
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
	void set_compute_shader(Shader *shader);
	void set_hull_shader(Shader *shader);
	void set_domain_shader(Shader *shader);
	void set_pixel_shader(Shader *shader);

	void set_vertex_shader_resource(u32 gpu_register, const Gpu_Buffer &constant_buffer);
	void set_vertex_shader_resource(u32 gpu_register, const Shader_Resource_View &shader_resource);
	void set_vertex_shader_resource(u32 shader_resource_register, const Gpu_Struct_Buffer &struct_buffer);
	
	void set_pixel_shader_sampler(u32 sampler_register, const Sampler_State &sampler_state);
	void set_pixel_shader_resource(u32 gpu_register, const Gpu_Buffer &constant_buffer);
	void set_pixel_shader_resource(u32 shader_resource_register, const Shader_Resource_View &shader_resource_view);
	void set_pixel_shader_resource(u32 shader_resource_register, const Gpu_Struct_Buffer &struct_buffer);
	void reset_pixel_shader_resource(u32 shader_resource_register);

	void set_rasterizer_state(const Rasterizer_State &rasterizer_state);
	void set_scissor(Rect_s32 *rect);
	void set_viewport(Viewport *view_port);
	void reset_rasterizer();

	void set_blend_state(const Blend_State &blend_state);
	void reset_blending_state();

	void set_depth_stencil_state(const Depth_Stencil_State &depth_stencil_state, u32 stencil_ref = 0);
	void reset_depth_stencil_state();

	void set_render_target(const Render_Target_View &render_target_view, const Depth_Stencil_View &depth_stencil_view);

	void draw(u32 vertex_count);
	void draw_indexed(u32 index_count, u32 index_offset, u32 vertex_offset);
};

template <typename T>
inline void *Render_Pipeline::map(Gpu_Resource<T> &resource, Map_Type map_type)
{
	D3D11_MAP dx11_map_type;
	switch (map_type) {
		case MAP_TYPE_READ:
			dx11_map_type = D3D11_MAP_READ;
			break;
		case MAP_TYPE_WRITE:
			dx11_map_type = D3D11_MAP_WRITE;
			break;
		case MAP_TYPE_READ_WRITE:
			dx11_map_type = D3D11_MAP_READ_WRITE;
			break;
		case MAP_TYPE_WRITE_DISCARD:
			dx11_map_type = D3D11_MAP_WRITE_DISCARD;
			break;
		case MAP_TYPE_MAP_WRITE_NO_OVERWRITE:
			dx11_map_type = D3D11_MAP_WRITE_NO_OVERWRITE;
			break;
		default:
			assert(false);
	}
	D3D11_MAPPED_SUBRESOURCE subresource;
	HR(dx11_context->Map(resource.get(), 0, dx11_map_type, 0, &subresource));
	return subresource.pData;
}

template <typename T>
inline void Render_Pipeline::unmap(Gpu_Resource<T> &resource)
{
	dx11_context->Unmap(resource.get(), 0);
}

template<typename T>
inline void Render_Pipeline::copy_resource(const Gpu_Resource<T> &dst, const Gpu_Resource<T> &src)
{
	dx11_context->CopyResource(dst.resource.Get(), src.resource.Get());
}

template<typename T>
inline void Render_Pipeline::copy_subresource(const Gpu_Resource<T> &dst, u32 dst_x, u32 dst_y, const Gpu_Resource<T> &src)
{
	dx11_context->CopySubresourceRegion(dst.resource.Get(), 0, dst_x, dst_y, 0, src.resource.Get(), 0, NULL);
}

u32 get_dxgi_format_size(DXGI_FORMAT format);
void init_render_api(Gpu_Device *gpu_device, Render_Pipeline *render_pipeline);
void setup_multisampling(Gpu_Device *gpu_device, Multisample_Info *multisample_info);
Multisample_Info get_default_multisample();

#endif

