#include <assert.h>

#include <d3d12.h>
#include <wrl/client.h>

#include "render_device.h"

#include "../../sys/sys.h"
#include "../../sys/utils.h"

#include "../../libs/number_types.h"
#include "../../libs/structures/queue.h"

using Microsoft::WRL::ComPtr;

template <typename... Args>
void set_name(ID3D12Object *objec, Args... args)
{
	char *formatted_string = format(args);
	wchar_t *wstring = to_wstring(name);

	object->SetName(wstring);

	free_string(formatted_string);
	free_string(wstring);
}


static D3D12_HEAP_TYPE to_d3d12_heap_type(Resource_Usage type)
{
	switch (type) {
		case RESOURCE_USAGE_DEFAULT:
			return D3D12_HEAP_TYPE_DEFAULT;
		case RESOURCE_USAGE_UPLOAD:
			return D3D12_HEAP_TYPE_UPLOAD;
		case RESOURCE_USAGE_READBACK:
			return D3D12_HEAP_TYPE_READBACK;
	}
	assert(false);
	return (D3D12_HEAP_TYPE)0;
}

inline D3D12_RESOURCE_DIMENSION to_d3d12_resource_dimension(Texture_Dimension texture_dimension)
{
	switch (texture_dimension) {
		case TEXTURE_DIMENSION_UNKNOWN:
			return D3D12_RESOURCE_DIMENSION_UNKNOWN;
		case TEXTURE_DIMENSION_1D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		case TEXTURE_DIMENSION_2D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		case TEXTURE_DIMENSION_3D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	}
	assert(false);
	return static_cast<D3D12_RESOURCE_DIMENSION>(0);
}


D3D12_RESOURCE_STATES to_d3d12_resource_state(const Resource_State &resource_state)
{
	switch (resource_state) {
		case RESOURCE_STATE_COMMON:
			return D3D12_RESOURCE_STATE_COMMON;
		case RESOURCE_STATE_GENERIC_READ:
			return D3D12_RESOURCE_STATE_GENERIC_READ;
		case RESOURCE_STATE_COPY_DEST:
			return D3D12_RESOURCE_STATE_COPY_DEST;
		case RESOURCE_STATE_COPY_SOURCE:
			return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case RESOURCE_STATE_RENDER_TARGET:
			return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case RESOURCE_STATE_PRESENT:
			return D3D12_RESOURCE_STATE_PRESENT;
		case RESOURCE_STATE_DEPTH_WRITE:
			return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
			return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		case RESOURCE_STATE_ALL_SHADER_RESOURCE:
			return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}
	assert(false);
	return (D3D12_RESOURCE_STATES)0;
}

struct D3D12_Render_Device;

enum Resource_Type {
	RESOURCE_TYPE_BUFFER,
	RESOURCE_TYPE_TEXTURE
};

struct Resource_Desc {
	Resource_Desc(Buffer_Desc *buffer_desc);
	Resource_Desc(Texture_Desc *texture_desc);
	~Resource_Desc();

	Resource_Type type;

	union {
		Buffer_Desc buffer_desc;
		Texture_Desc texture_desc;
	};

	String &resource_name();
	D3D12_RESOURCE_DESC to_d3d12_resource_desc();
	D3D12_RESOURCE_STATES to_d312_resource_state();
};

Resource_Desc::Resource_Desc(Buffer_Desc *buffer_desc)
{
}

Resource_Desc::Resource_Desc(Texture_Desc *texture_desc)
{
}

Resource_Desc::~Resource_Desc()
{
}

String &Resource_Desc::resource_name()
{
	// TODO: insert return statement here
}

D3D12_RESOURCE_DESC Resource_Desc::to_d3d12_resource_desc()
{
	return D3D12_RESOURCE_DESC();
}

D3D12_RESOURCE_STATES Resource_Desc::to_d312_resource_state()
{
	return D3D12_RESOURCE_STATES();
}

struct Working_Buffer {
	u64 frame_number = 0;
	D3D12_Resource buffer;
};

struct D3D12_Base_Buffer {
	D3D12_Base_Buffer();
	~D3D12_Base_Buffer();
	
	u64 frame_number = 0;
	D3D12_Resource resource;

	void set_name();
};

struct D3D12_Buffer : Buffer {
	D3D12_Buffer(D3D12_Render_Device *render_device);
	D3D12_Buffer(D3D12_Render_Device *render_device, D3D12_Resource &default_buffer);
	~D3D12_Buffer();

	D3D12_Render_Device *render_device = NULL;

	Buffer_Desc buffer_desc;
	D3D12_Resource default_buffer;
	Queue<Working_Buffer> working_buffers;
	Queue<D3D12_Resource> completed_buffers;

	void finish_frame(u64 frame_number);
	void write(void *data, u64 data_size, u64 alignment = 0);

	Buffer_Desc get_buffer_desc();
	D3D12_Resource current_upload_buffer();
};

D3D12_Buffer::D3D12_Buffer(D3D12_Render_Device *render_device, D3D12_Resource &default_buffer) : render_device(render_device), default_buffer(default_buffer)
{
}

D3D12_Buffer::~D3D12_Buffer()
{
}

void D3D12_Buffer::finish_frame(u64 frame_number)
{
	while (!working_buffers.empty() && (working_buffers.front().frame_number <= frame_number)) {
		D3D12_Resource upload_buffer = working_buffers.front().buffer;
		set_name(upload_buffer.Get(), "[Upload Buffer] name: {}, frame_number: {} completed", buffer_desc.name, render_device->frame_number);
		completed_buffers.push(upload_buffer);
		working_buffers.pop();
	}
}

void D3D12_Buffer::write(void *data, u64 data_size, u64 alignment)
{
	assert(data);
	assert(data_size > 0);

	Resource_Usage usage;
	if (usage == RESOURCE_USAGE_DEFAULT) {
		Resource_Desc resource_desc = Resource_Desc(&buffer_desc);
		
		D3D12_Resource upload_buffer;
		render_device->create_resource(&resource_desc, upload_buffer);

		void *ptr = NULL;
		HR(upload_buffer->Map(0, NULL, &ptr));
		memcpy(ptr, data, data_size);
		upload_buffer->Unmap(0, NULL);

		D3D12_Copy_Command_List *copy_command_list = render_device->get_upload_list();
		copy_command_list->copy(default_buffer, upload_buffer);

		render_device->safe_release(upload_buffer);
	} else if (usage == RESOURCE_USAGE_UPLOAD) {
		D3D12_Resource upload_buffer;
		if (!completed_buffers.empty()) {
			upload_buffer = completed_buffers.front();
			set_name(upload_buffer.Get(), "[Upload Buffer] name: {}, frame_number: {}", buffer_desc.name, render_device->frame_number);
			completed_buffers.pop();

			working_buffers.push({ render_device->frame_number, upload_buffer });
		} else {
			Resource_Desc resource_desc = Resource_Desc(&buffer_desc);

			render_device->create_resource(&resource_desc, upload_buffer);
			set_name(upload_buffer.Get(), "[Upload Buffer] name: {}, frame_number: {}", buffer_desc.name, render_device->frame_number);
			
			working_buffers.push({ render_device->frame_number, upload_buffer });
		}

		void *ptr = NULL;
		HR(upload_buffer->Map(0, NULL, &ptr));
		memcpy(ptr, data, data_size);
		upload_buffer->Unmap(0, NULL);

		D3D12_Copy_Command_List *copy_command_list = render_device->get_upload_list();
		copy_command_list->copy(default_buffer, upload_buffer);
	}
}

struct D3D12_Texture : Texture {
	D3D12_Texture(D3D12_Render_Device *render_device, D3D12_Resource &texture);
	~D3D12_Texture();

	D3D12_Render_Device *render_device = NULL;
	D3D12_Resource texture;

	Texture_Desc texture_desc();
};

struct D3D12_Copy_Command_List : Command_List {
	D3D12_Copy_Command_List();
	~D3D12_Copy_Command_List();

	//void resource_barrier(const Resource_Barrier &resource_barrier);

	void copy(Buffer *dest, Buffer *source);

	void copy(D3D12_Resource &dest, D3D12_Resource &source);
	void copy_buffer_to_texture(D3D12_Resource &dest, D3D12_Resource &source);
	//void copy_buffer_to_texture(GPU_Resource &dest, GPU_Buffer &source, Subresource_Footprint &subresource_footprint);
};

struct Frame_Command_Allocator {
	u64 frame_number;
	ComPtr<ID3D12CommandAllocator> command_allocator;
};

struct D3D12_Render_Device : Render_Device {
	D3D12_Render_Device();
	~D3D12_Render_Device();

	u64 frame_number = 0;

	Resource_Allocator *resource_allocator = NULL;
	ComPtr<ID3D12Device> device;

	Queue<Working_Buffer> resource_release_queue;

	bool create();

	Buffer *create_buffer(Buffer_Desc *buffer_desc);
	Texture *create_texture(Texture_Desc *texture_desc);

	Graphics_Command_List *create_graphics_command_list();

	void finish_frame(u32 completed_frame);

	//internal
	void safe_release(ComPtr<ID3D12Resource> &resource);
	void create_resource(Resource_Desc *resource_desc, ComPtr<ID3D12Resource> &resource);

	Frame_Command_Allocator allocate_command_allocator();
};

void D3D12_Render_Device::finish_frame(u32 completed_frame)
{
	while (!resource_release_queue.empty() && (resource_release_queue.front().frame_number <= completed_frame)) {
		ComPtr<ID3D12Resource> buffer = resource_release_queue.front().buffer;
		//set_name(buffer.Get(), "Upload buffer [name {}] [frame {} completed]", debug_name, _frame_number)
		set_name(buffer.Get(), "Upload buffer", frame_number);
		resource_release_queue.pop();
	}

	frame_number++;
}

Frame_Command_Allocator D3D12_Render_Device::allocate_command_allocator()
{
	return Frame_Command_Allocator();
}


D3D12_Render_Device::D3D12_Render_Device()
{
}

D3D12_Render_Device::~D3D12_Render_Device()
{
}

Buffer *D3D12_Render_Device::create_buffer(Buffer_Desc *buffer_desc)
{
	assert(buffer_desc->count > 0);
	assert(buffer_desc->stride > 0);

	D3D12_RESOURCE_DESC resource_desc;
	ZeroMemory(&resource_desc, sizeof(D3D12_RESOURCE_DESC));
	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resource_desc.Alignment = 0;
	resource_desc.Width = buffer_desc->size();
	resource_desc.Height = 1;
	resource_desc.DepthOrArraySize = 1;
	resource_desc.MipLevels = 1;
	resource_desc.Format = DXGI_FORMAT_UNKNOWN;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.SampleDesc.Quality = 0;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heap_properties;
	ZeroMemory(&heap_properties, sizeof(D3D12_HEAP_PROPERTIES));
	heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_Resource default_buffer;
	HR(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(default_buffer.ReleaseAndGetAddressOf())));
	set_name(default_buffer.Get(), "[Default Buffer] name: {}", buffer_desc->name);

	if ((buffer_desc->usage == RESOURCE_USAGE_DEFAULT) && buffer_desc->data) {
		heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_Resource upload_buffer;
		HR(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(upload_buffer.ReleaseAndGetAddressOf())));
		set_name(upload_buffer.Get(), "[Upload Buffer] name: {}, frame_number: {}", buffer_desc->name, frame_number);
		
		void *ptr = NULL;
		HR(upload_buffer->Map(0, NULL, &ptr));
		memcpy(ptr, buffer_desc->data, buffer_desc->size());
		upload_buffer->Unmap(0, NULL);

		D3D12_Copy_Command_List *copy_command_list = get_upload_list();
		copy_command_list->copy(default_buffer, upload_buffer);

		safe_release(upload_buffer);
	}
	return (buffer_desc->usage == RESOURCE_USAGE_DEFAULT) ? new D3D12_Buffer(this, default_buffer) : new D3D12_Buffer(this);
}

Texture *D3D12_Render_Device::create_texture(Texture_Desc *texture_desc)
{
	D3D12_RESOURCE_DESC resource_desc;
	ZeroMemory(&resource_desc, sizeof(D3D12_RESOURCE_DESC));
	resource_desc.Dimension = to_d3d12_resource_dimension(texture_desc->dimension);
	resource_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resource_desc.Width = static_cast<u64>(texture_desc->width);
	resource_desc.Height = texture_desc->height;
	resource_desc.DepthOrArraySize = texture_desc->depth;
	resource_desc.MipLevels = texture_desc->miplevels;
	resource_desc.Format = texture_desc->format;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.SampleDesc.Quality = 0;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resource_desc.Flags = static_cast<D3D12_RESOURCE_FLAGS>(texture_desc->flags);

	D3D12_HEAP_PROPERTIES heap_properties;
	ZeroMemory(&heap_properties, sizeof(D3D12_HEAP_PROPERTIES));
	heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_Resource texture;
	HR(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(texture.ReleaseAndGetAddressOf())));
	set_name(texture.Get(), "[Texture] name: {}", texture_desc->name);

	if (texture_desc->data) {
		heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_Resource upload_buffer;
		HR(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(upload_buffer.ReleaseAndGetAddressOf())));
		set_name(upload_buffer.Get(), "[Upload Buffer] name: {}, frame_number: {}", texture_desc->name, frame_number);

		void *ptr = NULL;
		HR(upload_buffer->Map(0, NULL, &ptr));
		memcpy(ptr, texture_desc->data, texture_desc->size());
		upload_buffer->Unmap(0, NULL);

		D3D12_Copy_Command_List *copy_command_list = get_upload_list();
		copy_command_list->copy_buffer_to_texture(texture, upload_buffer);

		safe_release(upload_buffer);
	}
	return new D3D12_Texture(this, texture);
}

struct D3D12_Command_List : Graphics_Command_List {
	D3D12_Command_List();
	~D3D12_Command_List();

	void reset();
	void close();

	// Copy command list methods
	void copy(Buffer *dest, Buffer *source);
	void copy_buffer_to_texture(Texture *texture, Buffer *buffer, Subresource_Footprint &subresource_footprint);

	// Compute command list methods
	void set_pipeline_state(Pipeline_State *pipeline_state);
	void set_compute_root_signature(Root_Signature *root_signature);
	void set_compute_constants(u32 parameter_index, void *data);
	void set_compute_root_descriptor_table(u32 parameter_index, const GPU_Descriptor &base_descriptor);
	void dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z = 1);

	// Graphics command list methods
	void set_graphics_root_signature(Root_Signature *root_signature);
	void set_primitive_type(Primitive_Type primitive_type);
	void set_viewport(Viewport viewport);
	void set_clip_rect(Rect_u32 clip_rect);
	
	void clear_render_target_view(RT_Descriptor *descriptor, const Color &color);
	void clear_depth_stencil_view(DS_Descriptor *descriptor, float depth = 1.0f, u8 stencil = 0);
	
	void set_vertex_buffer(Buffer *buffer);
	void set_index_buffer(Buffer *buffer);
	
	void set_graphics_constants(u32 parameter_index, void *data);
	void set_graphics_root_descriptor_table(u32 parameter_index, GPU_Descriptor *base_descriptor);
	void set_render_target(RT_Descriptor *render_target_descriptor, DS_Descriptor *depth_stencil_descriptor);
	
	void draw(u32 vertex_count);
	void draw_indexed(u32 index_count);
};

struct D3D12_Command_Queue : Command_Queue {
	D3D12_Command_Queue();
	~D3D12_Command_Queue();
	
	void close_and_execute_command_list(Command_List *command_list);
};

Graphics_Command_List *D3D12_Render_Device::create_graphics_command_list()
{

	return nullptr;
}

void D3D12_Render_Device::safe_release(ComPtr<ID3D12Resource> &resource)
{
	resource_release_queue.push({ frame_number, resource });
}

static const char *str_shader_register_types[] = {
	"Constant Buffer",
	"Shader Resource",
	"Unordered Access",
	"Sampler"
};

const u32 HLSL_REGISTRE_COUNT = 20;
const u32 HLSL_SPACE_COUNT = 20;

const u32 SHADER_REGISTER_TYPES_NUMBER = 4;

struct Root_Parameter {
	~Root_Parameter();
	Root_Parameter();

	u32 indices[SHADER_REGISTER_TYPES_NUMBER];

	void set_parameter_index(u32 parameter_index, Shader_Register register_type);
	u32 get_parameter_index(Shader_Register register_type);
};

Root_Parameter::Root_Parameter()
{
	memset((void *)indices, 0xff, sizeof(u32) * SHADER_REGISTER_TYPES_NUMBER);
}

Root_Parameter::~Root_Parameter()
{
}

void Root_Parameter::set_parameter_index(u32 parameter_index, Shader_Register register_type)
{
	assert(static_cast<u32>(register_type) < SHADER_REGISTER_TYPES_NUMBER);
	indices[static_cast<u32>(register_type)] = parameter_index;
}

u32 Root_Parameter::get_parameter_index(Shader_Register register_type)
{
	assert(static_cast<u32>(register_type) < SHADER_REGISTER_TYPES_NUMBER);
	return indices[static_cast<u32>(register_type)];
}

struct Temp_Root_Parameter {
	Temp_Root_Parameter(D3D12_ROOT_PARAMETER_TYPE type);
	~Temp_Root_Parameter();

	Temp_Root_Parameter(const Temp_Root_Parameter &t);
	Temp_Root_Parameter &operator=(const Temp_Root_Parameter &t);

	Array<D3D12_DESCRIPTOR_RANGE1> ranges;
	D3D12_ROOT_PARAMETER1 parameter;

	void add_constants(u32 shader_register, u32 register_space, u32 structure_size);

	void add_srv_descriptor_range(u32 shader_register, u32 register_space, u32 descriptors_number);
	void add_descriptor_range(D3D12_DESCRIPTOR_RANGE_TYPE range_type, u32 shader_register, u32 register_space, u32 descriptors_number);
};


struct D3D12_Root_Signature : Root_Signature {
	D3D12_Root_Signature(D3D12_Render_Device *render_device);
	~D3D12_Root_Signature();

	D3D12_Render_Device *render_device = NULL;

	Array<Temp_Root_Parameter> parameters;
	Root_Parameter parameters_table[HLSL_REGISTRE_COUNT][HLSL_SPACE_COUNT];
	ComPtr<ID3D12RootSignature> d3d12_root_signature;

	void store_parameter_index(u32 parameter_index, u32 shader_register, u32 shader_space, Shader_Register register_type);
	u32 get_parameter_index(u32 shader_register, u32 shader_space, Shader_Register register_type);

	void compile(u32 access_flags = 0);
	void add_shader_resource_parameter(u32 shader_register, u32 register_space, u32 descriptors_number);
};

D3D12_Root_Signature::D3D12_Root_Signature(D3D12_Render_Device *render_device) : render_device(render_device)
{
}

D3D12_Root_Signature::~D3D12_Root_Signature()
{
}

void D3D12_Root_Signature::store_parameter_index(u32 parameter_index, u32 shader_register, u32 shader_space, Shader_Register register_type)
{
	assert(shader_register <= HLSL_REGISTRE_COUNT);
	assert(shader_space <= HLSL_SPACE_COUNT);

	u32 temp = parameters_table[shader_register][shader_space].get_parameter_index(register_type);
	if (temp != UINT_MAX) {
		error("A parameter index has already been set for the shader register(register = {}, space = {}, type = {}).", shader_register, shader_space, str_shader_register_types[static_cast<u32>(register_type)]);
	} else {
		parameters_table[shader_register][shader_space].set_parameter_index(parameter_index, register_type);
	}
}

u32 D3D12_Root_Signature::get_parameter_index(u32 shader_register, u32 shader_space, Shader_Register register_type)
{
	assert(shader_register <= HLSL_REGISTRE_COUNT);
	assert(shader_space <= HLSL_SPACE_COUNT);

	u32 parameter_index = parameters_table[shader_register][shader_space].get_parameter_index(register_type);
	if (parameter_index == UINT_MAX) {
		error("A parameter index has not been set yet for the shader register(register = {}, space = {}, type = {}).", shader_register, shader_space, str_shader_register_types[static_cast<u32>(register_type)]);
	}
	return parameter_index;
}

//static D3D12_DESCRIPTOR_RANGE1 *make_descriptor_range(D3D12_DESCRIPTOR_RANGE_TYPE range_type, u32 shader_register, u32 register_space, u32 descriptors_number)
//{
//	D3D12_DESCRIPTOR_RANGE1 *descriptor_range = new D3D12_DESCRIPTOR_RANGE1();
//	ZeroMemory(descriptor_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
//	descriptor_range->RangeType = range_type;
//	descriptor_range->NumDescriptors = descriptors_number;
//	descriptor_range->BaseShaderRegister = shader_register;
//	descriptor_range->RegisterSpace = register_space;
//	descriptor_range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
//	descriptor_range->Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
//	return descriptor_range;
//}
//
//static D3D12_ROOT_PARAMETER1 make_descriptor_table_root_parameter(u32 descriptor_ranges_number, D3D12_DESCRIPTOR_RANGE1 *descriptor_range)
//{
//	D3D12_ROOT_PARAMETER1 parameter;
//	ZeroMemory(&parameter, sizeof(D3D12_ROOT_PARAMETER1));
//	parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
//	parameter.DescriptorTable.NumDescriptorRanges = descriptor_ranges_number;
//	parameter.DescriptorTable.pDescriptorRanges = descriptor_range;
//	parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
//	return parameter;
//}

const u32 ALLOW_INPUT_LAYOUT_ACCESS = 0x1;
const u32 ALLOW_VERTEX_SHADER_ACCESS = 0x2;
const u32 ALLOW_PIXEL_SHADER_ACCESS = 0x4;
const u32 ALLOW_HULL_SHADER_ACCESS = 0x8;
const u32 ALLOW_DOMAIN_SHADER_ACCESS = 0x10;
const u32 ALLOW_GEOMETRY_SHADER_ACCESS = 0x20;
const u32 ALLOW_AMPLIFICATION_SHADER_ACCESS = 0x40;
const u32 ALLOW_MESH_SHADER_ACCESS = 0x80;

void D3D12_Root_Signature::compile(u32 access_flags)
{
	Array<D3D12_ROOT_PARAMETER1> d3d12_root_paramaters;
	for (u32 i = 0; i < parameters.count; i++) {
		d3d12_root_paramaters.push(parameters[i].parameter);
	}

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
	ZeroMemory(&root_signature_desc, sizeof(D3D12_VERSIONED_ROOT_SIGNATURE_DESC));
	root_signature_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	root_signature_desc.Desc_1_1.NumParameters = d3d12_root_paramaters.count;
	root_signature_desc.Desc_1_1.pParameters = d3d12_root_paramaters.items;

	if (access_flags & ALLOW_INPUT_LAYOUT_ACCESS) {
		root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}
	if (!(access_flags & ALLOW_VERTEX_SHADER_ACCESS)) {
		root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
	}
	if (!(access_flags & ALLOW_PIXEL_SHADER_ACCESS)) {
		root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
	}
	if (!(access_flags & ALLOW_HULL_SHADER_ACCESS)) {
		root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
	}
	if (!(access_flags & ALLOW_DOMAIN_SHADER_ACCESS)) {
		root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	}
	if (!(access_flags & ALLOW_GEOMETRY_SHADER_ACCESS)) {
		root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	}
	if (!(access_flags & ALLOW_AMPLIFICATION_SHADER_ACCESS)) {
		root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
	}
	if (!(access_flags & ALLOW_MESH_SHADER_ACCESS)) {
		root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
	}

	ComPtr<ID3DBlob> signature_blob;
	ComPtr<ID3DBlob> errors;
	D3D12SerializeVersionedRootSignature(&root_signature_desc, &signature_blob, &errors);
	HR(render_device->device->CreateRootSignature(0, signature_blob->GetBufferPointer(), signature_blob->GetBufferSize(), IID_PPV_ARGS(d3d12_root_signature.ReleaseAndGetAddressOf())));
}

void D3D12_Root_Signature::add_shader_resource_parameter(u32 shader_register, u32 register_space, u32 descriptors_number)
{
	//D3D12_DESCRIPTOR_RANGE1 *descriptor_range = make_descriptor_range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, shader_register, register_space, descriptors_number);
	//ranges.push(descriptor_range);

	//D3D12_ROOT_PARAMETER1 root_parameter = make_descriptor_table_root_parameter(1, descriptor_range);
	Temp_Root_Parameter root_parameter = Temp_Root_Parameter(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);
	root_parameter.add_srv_descriptor_range(shader_register, register_space, descriptors_number);

	u32 parameter_index = parameters.push(root_parameter);
	store_parameter_index(parameter_index, shader_register, register_space, SHADER_RESOURCE_REGISTER);
}

Temp_Root_Parameter::Temp_Root_Parameter(D3D12_ROOT_PARAMETER_TYPE type)
{
	ZeroMemory(&parameter, sizeof(D3D12_ROOT_PARAMETER1));
	parameter.ParameterType = type;
	parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
}

Temp_Root_Parameter::~Temp_Root_Parameter()
{
}

Temp_Root_Parameter::Temp_Root_Parameter(const Temp_Root_Parameter &other)
{
	*this = other;
}

Temp_Root_Parameter &Temp_Root_Parameter::operator=(const Temp_Root_Parameter &other)
{
	if (this != &other) {
		ranges = other.ranges;
		parameter = other.parameter;
		if (other.parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
			parameter.DescriptorTable.NumDescriptorRanges = ranges.count;
			parameter.DescriptorTable.pDescriptorRanges = ranges.items;
		}
	}
	return *this;
}

void Temp_Root_Parameter::add_constants(u32 shader_register, u32 register_space, u32 structure_size)
{
	assert((structure_size % 4) == 0);
	assert(parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS);

	parameter.Constants.ShaderRegister = shader_register;
	parameter.Constants.RegisterSpace = register_space;
	parameter.Constants.Num32BitValues = structure_size / 4;
}

void Temp_Root_Parameter::add_srv_descriptor_range(u32 shader_register, u32 register_space, u32 descriptors_number)
{
	add_descriptor_range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, shader_register, register_space, descriptors_number);
}

void Temp_Root_Parameter::add_descriptor_range(D3D12_DESCRIPTOR_RANGE_TYPE range_type, u32 shader_register, u32 register_space, u32 descriptors_number)
{
	assert(parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);

	D3D12_DESCRIPTOR_RANGE1 descriptor_range;
	ZeroMemory(&descriptor_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
	descriptor_range.RangeType = range_type;
	descriptor_range.NumDescriptors = descriptors_number;
	descriptor_range.BaseShaderRegister = shader_register;
	descriptor_range.RegisterSpace = register_space;
	descriptor_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	descriptor_range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

	ranges.push(descriptor_range);

	parameter.DescriptorTable.NumDescriptorRanges = ranges.count;
	parameter.DescriptorTable.pDescriptorRanges = ranges.items;
}
