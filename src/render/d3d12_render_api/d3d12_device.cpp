#define USE_PIX
#include <d3d12.h>
#include <Windows.h>
#include <pix3.h>

#include "d3d12_device.h"
#include "d3d12_resources.h"
#include "d3d12_descriptor_heap.h"
#include "d3d12_functions.h"
#include "to_d3d12_types.h"

#include "../../win32/win_helpers.h"

static D3D12_Render_Device *internal_render_device_reference = NULL;

static const char *str_shader_register_types[] = {
	"Constant Buffer",
	"Shader Resource",
	"Unordered Access",
	"Sampler"
};

inline D3D12_SUBRESOURCE_FOOTPRINT to_d3d12_subresource_footprint(Subresource_Footprint *subresource_footprint)
{
	D3D12_SUBRESOURCE_FOOTPRINT d3d12_subresource_footprint;
	ZeroMemory(&d3d12_subresource_footprint, sizeof(D3D12_SUBRESOURCE_FOOTPRINT));
	d3d12_subresource_footprint.Format = subresource_footprint->format;
	d3d12_subresource_footprint.Width = subresource_footprint->width;
	d3d12_subresource_footprint.Height = subresource_footprint->height;
	d3d12_subresource_footprint.Depth = subresource_footprint->depth;
	d3d12_subresource_footprint.RowPitch = subresource_footprint->row_pitch;
	return d3d12_subresource_footprint;
}

Root_Parameter_Index::Root_Parameter_Index()
{
	memset((void *)indices, 0xff, sizeof(u32) * SHADER_REGISTER_TYPES_NUMBER);
}

Root_Parameter_Index::~Root_Parameter_Index()
{
}

void Root_Parameter_Index::set_parameter_index(u32 parameter_index, Shader_Register register_type)
{
	assert(static_cast<u32>(register_type) < SHADER_REGISTER_TYPES_NUMBER);
	indices[static_cast<u32>(register_type)] = parameter_index;
}

u32 Root_Parameter_Index::get_parameter_index(Shader_Register register_type)
{
	assert(static_cast<u32>(register_type) < SHADER_REGISTER_TYPES_NUMBER);
	return indices[static_cast<u32>(register_type)];
}

Root_Parameter::Root_Parameter()
{
	ZeroMemory(&d3d12_parameter, sizeof(D3D12_ROOT_PARAMETER1));
}

Root_Parameter::Root_Parameter(D3D12_ROOT_PARAMETER_TYPE type) : Root_Parameter()
{
	d3d12_parameter.ParameterType = type;
	d3d12_parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
}

Root_Parameter::~Root_Parameter()
{
}

Root_Parameter::Root_Parameter(const Root_Parameter &other)
{
	*this = other;
}

Root_Parameter &Root_Parameter::operator=(const Root_Parameter &other)
{
	if (this != &other) {
		d3d12_parameter = other.d3d12_parameter;
		if (other.d3d12_parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
			d3d12_descriptor_ranges = other.d3d12_descriptor_ranges;
			d3d12_parameter.DescriptorTable.NumDescriptorRanges = d3d12_descriptor_ranges.count;
			d3d12_parameter.DescriptorTable.pDescriptorRanges = d3d12_descriptor_ranges.items;
		}
	}
	return *this;
}

void Root_Parameter::add_32bit_constants(u32 shader_register, u32 register_space, u32 structure_size)
{
	assert((structure_size % 4) == 0);
	assert(d3d12_parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS);

	d3d12_parameter.Constants.ShaderRegister = shader_register;
	d3d12_parameter.Constants.RegisterSpace = register_space;
	d3d12_parameter.Constants.Num32BitValues = structure_size / 4;
}

void Root_Parameter::add_descriptor(u32 shader_register, u32 register_space)
{
	assert(d3d12_parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV);

	d3d12_parameter.Descriptor.ShaderRegister = shader_register;
	d3d12_parameter.Descriptor.RegisterSpace = register_space;
}

void Root_Parameter::add_srv_descriptor_range(u32 shader_register, u32 register_space, u32 descriptors_number)
{
	add_descriptor_range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, shader_register, register_space, descriptors_number);
}

void Root_Parameter::add_sampler_descriptor_range(u32 shader_register, u32 register_space, u32 descriptors_number)
{
	add_descriptor_range(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, shader_register, register_space, descriptors_number);
}

void Root_Parameter::add_descriptor_range(D3D12_DESCRIPTOR_RANGE_TYPE range_type, u32 shader_register, u32 register_space, u32 descriptors_number)
{
	assert(d3d12_parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);

	D3D12_DESCRIPTOR_RANGE1 descriptor_range;
	ZeroMemory(&descriptor_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
	descriptor_range.RangeType = range_type;
	descriptor_range.NumDescriptors = descriptors_number;
	descriptor_range.BaseShaderRegister = shader_register;
	descriptor_range.RegisterSpace = register_space;
	descriptor_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	descriptor_range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	//descriptor_range.Flags = (descriptors_number == UNBOUNDED_DESCRIPTORS_NUMBER) ? D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE : D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	d3d12_descriptor_ranges.push(descriptor_range);

	d3d12_parameter.DescriptorTable.NumDescriptorRanges = d3d12_descriptor_ranges.count;
	d3d12_parameter.DescriptorTable.pDescriptorRanges = d3d12_descriptor_ranges.items;
}

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

void D3D12_Root_Signature::compile(u32 access_flags)
{
	Array<D3D12_ROOT_PARAMETER1> d3d12_root_paramaters;
	for (u32 i = 0; i < parameters.count; i++) {
		d3d12_root_paramaters.push(parameters[i].d3d12_parameter);
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
	// Full bindless
	//root_signature_desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

	ComPtr<ID3DBlob> signature_blob;
	ComPtr<ID3DBlob> errors;
	HR(D3D12SerializeVersionedRootSignature(&root_signature_desc, &signature_blob, &errors));
	HR(render_device->device->CreateRootSignature(0, signature_blob->GetBufferPointer(), signature_blob->GetBufferSize(), IID_PPV_ARGS(d3d12_root_signature.ReleaseAndGetAddressOf())));
}

void D3D12_Root_Signature::add_32bit_constants_parameter(u32 shader_register, u32 register_space, u32 struct_size)
{
	Root_Parameter root_parameter = Root_Parameter(D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS);
	root_parameter.add_32bit_constants(shader_register, register_space, struct_size);

	u32 parameter_index = parameters.push(root_parameter);
	store_parameter_index(parameter_index, shader_register, register_space, CONSTANT_BUFFER_REGISTER);
}

void D3D12_Root_Signature::add_constant_buffer_parameter(u32 shader_register, u32 register_space)
{
	Root_Parameter root_parameter = Root_Parameter(D3D12_ROOT_PARAMETER_TYPE_CBV);
	root_parameter.add_descriptor(shader_register, register_space);
	
	u32 parameter_index = parameters.push(root_parameter);
	store_parameter_index(parameter_index, shader_register, register_space, CONSTANT_BUFFER_REGISTER);
}

void D3D12_Root_Signature::add_shader_resource_parameter(u32 shader_register, u32 register_space, u32 descriptors_number)
{
	Root_Parameter root_parameter = Root_Parameter(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);
	root_parameter.add_srv_descriptor_range(shader_register, register_space, descriptors_number);

	u32 parameter_index = parameters.push(root_parameter);
	store_parameter_index(parameter_index, shader_register, register_space, SHADER_RESOURCE_REGISTER);
}

void D3D12_Root_Signature::add_sampler_parameter(u32 shader_register, u32 register_space, u32 descriptors_number)
{
	Root_Parameter root_parameter = Root_Parameter(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);
	root_parameter.add_sampler_descriptor_range(shader_register, register_space, descriptors_number);

	u32 parameter_index = parameters.push(root_parameter);
	store_parameter_index(parameter_index, shader_register, register_space, SAMPLER_REGISTER);
}

ID3D12RootSignature *D3D12_Root_Signature::get()
{
	return d3d12_root_signature.Get();
}

D3D12_Pipeline_State::D3D12_Pipeline_State(ComPtr<ID3D12Device> &device, Compute_Pipeline_Desc *pipeline_desc)
{
	assert(type == PIPELINE_TYPE_UNKNOWN);

	type = PIPELINE_TYPE_COMPUTE;
	root_signature = pipeline_desc->root_signature;

	D3D12_Root_Signature *internal_root_signature = static_cast<D3D12_Root_Signature *>(pipeline_desc->root_signature);

	D3D12_COMPUTE_PIPELINE_STATE_DESC d3d12_compute_pipeline_state;
	ZeroMemory(&d3d12_compute_pipeline_state, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
	d3d12_compute_pipeline_state.pRootSignature = internal_root_signature->get();
	d3d12_compute_pipeline_state.CS = tO_d3d12_shader_bytecode(pipeline_desc->cs_bytecode);

	HR(device->CreateComputePipelineState(&d3d12_compute_pipeline_state, IID_PPV_ARGS(d3d12_pipeline.ReleaseAndGetAddressOf())));
}

D3D12_Pipeline_State::D3D12_Pipeline_State(ComPtr<ID3D12Device> &device, Graphics_Pipeline_Desc *pipeline_desc)
{
	assert(type == PIPELINE_TYPE_UNKNOWN);

	u32 layout_offset = 0;
	Array<D3D12_INPUT_ELEMENT_DESC> d3d12_input_elements;

	type = PIPELINE_TYPE_GRAPHICS;
	primitive_type = pipeline_desc->primitive_type;
	root_signature = pipeline_desc->root_signature;

	D3D12_Root_Signature *internal_root_signature = static_cast<D3D12_Root_Signature *>(pipeline_desc->root_signature);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12_graphics_pipeline_state;
	ZeroMemory(&d3d12_graphics_pipeline_state, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	d3d12_graphics_pipeline_state.pRootSignature = internal_root_signature->get();
	d3d12_graphics_pipeline_state.VS = tO_d3d12_shader_bytecode(pipeline_desc->vs_bytecode);
	d3d12_graphics_pipeline_state.PS = tO_d3d12_shader_bytecode(pipeline_desc->ps_bytecode);
	
	if (!pipeline_desc->input_layouts.is_empty()) {
		for (u32 i = 0; i < pipeline_desc->input_layouts.count; i++) {
			Input_Layout input_layout = pipeline_desc->input_layouts[i];
			d3d12_input_elements.push({ input_layout.semantic_name, 0, input_layout.format, 0, layout_offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
			layout_offset += dxgi_format_size(input_layout.format);
		}
		d3d12_graphics_pipeline_state.InputLayout.NumElements = pipeline_desc->input_layouts.count;
		d3d12_graphics_pipeline_state.InputLayout.pInputElementDescs = d3d12_input_elements.items;
	}
	d3d12_graphics_pipeline_state.BlendState = to_d3d12_blend_desc(pipeline_desc->render_targets_formats.count, pipeline_desc->blending_desc);
	d3d12_graphics_pipeline_state.RasterizerState = to_d3d12_rasterizer_desc(pipeline_desc->rasterization_desc);
	d3d12_graphics_pipeline_state.DepthStencilState = to_d3d12_depth_stencil_desc(pipeline_desc->depth_stencil_desc);
	d3d12_graphics_pipeline_state.SampleMask = UINT32_MAX;
	d3d12_graphics_pipeline_state.PrimitiveTopologyType = to_d3d12_primitive_topology_type(pipeline_desc->primitive_type);
	d3d12_graphics_pipeline_state.NumRenderTargets = pipeline_desc->render_targets_formats.count;
	for (u32 i = 0; i < pipeline_desc->render_targets_formats.count; i++) {
		d3d12_graphics_pipeline_state.RTVFormats[i] = pipeline_desc->render_targets_formats[i];
	}
	d3d12_graphics_pipeline_state.DSVFormat = pipeline_desc->depth_stencil_format;
	d3d12_graphics_pipeline_state.SampleDesc.Count = 1;

	HR(device->CreateGraphicsPipelineState(&d3d12_graphics_pipeline_state, IID_PPV_ARGS(d3d12_pipeline.ReleaseAndGetAddressOf())));
}

D3D12_Pipeline_State::~D3D12_Pipeline_State()
{
}

ID3D12PipelineState *D3D12_Pipeline_State::get()
{
	return d3d12_pipeline.Get();
}

D3D12_Command_List::D3D12_Command_List(Command_List_Type command_list_type, D3D12_Render_Device *_render_device)
{
	type = command_list_type;
	render_device = _render_device;

	ComPtr<ID3D12Device> device = render_device->device;
	HR(device->CreateCommandAllocator(to_d3d12_command_list_type(type), IID_PPV_ARGS(command_allocator.ReleaseAndGetAddressOf())));
	device->CreateCommandList(0, to_d3d12_command_list_type(type), command_allocator.Get(), NULL, IID_PPV_ARGS(command_list.ReleaseAndGetAddressOf()));

	close();
}

D3D12_Command_List::~D3D12_Command_List()
{
}

ID3D12GraphicsCommandList *D3D12_Command_List::get()
{
	return command_list.Get();
}

void D3D12_Command_List::reset()
{
	command_allocator->Reset();
	command_list->Reset(command_allocator.Get(), NULL);
}

void D3D12_Command_List::close()
{
	command_list->Close();
}

u8 color_index = 0;

void D3D12_Command_List::begin_event(const char *name)
{
	PIXBeginEvent(command_list.Get(), PIX_COLOR_INDEX(++color_index), name);
}

void D3D12_Command_List::end_event()
{
	PIXEndEvent(command_list.Get());
}

void D3D12_Command_List::copy(Buffer *dest, Buffer *source)
{
	D3D12_Buffer *_dest = (D3D12_Buffer *)dest;
	D3D12_Buffer *_source = (D3D12_Buffer *)source;

	command_list->CopyResource(_dest->current_buffer()->get(), _source->current_buffer()->get());
}

void D3D12_Command_List::copy(D3D12_Resource *dest, D3D12_Resource *source)
{
	command_list->CopyResource(dest->get(), source->get());
}

void D3D12_Command_List::copy_buffer_to_texture(Texture *texture, Buffer *buffer, Subresource_Footprint *subresource_footprint)
{
	D3D12_Buffer *internal_buffer = (D3D12_Buffer *)buffer;
	D3D12_Texture *internal_texture = (D3D12_Texture *)texture;

	D3D12_TEXTURE_COPY_LOCATION dest_texture_copy_location;
	ZeroMemory(&dest_texture_copy_location, sizeof(D3D12_TEXTURE_COPY_LOCATION));
	dest_texture_copy_location.pResource = internal_buffer->current_buffer()->get();
	dest_texture_copy_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dest_texture_copy_location.SubresourceIndex = subresource_footprint->subresource_index;

	D3D12_TEXTURE_COPY_LOCATION source_texture_copy_location;
	ZeroMemory(&source_texture_copy_location, sizeof(D3D12_TEXTURE_COPY_LOCATION));
	source_texture_copy_location.pResource = internal_texture->get();
	source_texture_copy_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	source_texture_copy_location.PlacedFootprint.Footprint = to_d3d12_subresource_footprint(subresource_footprint);

	command_list->CopyTextureRegion(&dest_texture_copy_location, 0, 0, 0, &source_texture_copy_location, NULL);
}

void D3D12_Command_List::copy_buffer_to_texture(D3D12_Resource *texture, D3D12_Resource *buffer, Subresource_Footprint *subresource_footprint)
{
	D3D12_TEXTURE_COPY_LOCATION dest_texture_copy_location;
	ZeroMemory(&dest_texture_copy_location, sizeof(D3D12_TEXTURE_COPY_LOCATION));
	dest_texture_copy_location.pResource = texture->get();
	dest_texture_copy_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dest_texture_copy_location.SubresourceIndex = subresource_footprint->subresource_index;

	D3D12_TEXTURE_COPY_LOCATION source_texture_copy_location;
	ZeroMemory(&source_texture_copy_location, sizeof(D3D12_TEXTURE_COPY_LOCATION));
	source_texture_copy_location.pResource = buffer->get();
	source_texture_copy_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	source_texture_copy_location.PlacedFootprint.Footprint = to_d3d12_subresource_footprint(subresource_footprint);

	command_list->CopyTextureRegion(&dest_texture_copy_location, 0, 0, 0, &source_texture_copy_location, NULL);
}

void D3D12_Command_List::transition_resource_barrier(Buffer *buffer, Resource_State state_before, Resource_State state_after)
{
	D3D12_Buffer *internal_buffer = static_cast<D3D12_Buffer *>(buffer);

	D3D12_RESOURCE_BARRIER d3d12_resource_barrier;
	ZeroMemory(&d3d12_resource_barrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3d12_resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3d12_resource_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3d12_resource_barrier.Transition.pResource = internal_buffer->current_buffer()->get();
	d3d12_resource_barrier.Transition.Subresource = 0;
	d3d12_resource_barrier.Transition.StateBefore = to_d3d12_resource_state(state_before);
	d3d12_resource_barrier.Transition.StateAfter = to_d3d12_resource_state(state_after);

	command_list->ResourceBarrier(1, &d3d12_resource_barrier);
}

void D3D12_Command_List::transition_resource_barrier(Texture *texture, Resource_State state_before, Resource_State state_after, u32 subresource_index)
{
	D3D12_Texture *internal_texture = static_cast<D3D12_Texture *>(texture);

	D3D12_RESOURCE_BARRIER d3d12_resource_barrier;
	ZeroMemory(&d3d12_resource_barrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3d12_resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3d12_resource_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3d12_resource_barrier.Transition.pResource = internal_texture->get();
	d3d12_resource_barrier.Transition.Subresource = subresource_index;
	d3d12_resource_barrier.Transition.StateBefore = to_d3d12_resource_state(state_before);
	d3d12_resource_barrier.Transition.StateAfter = to_d3d12_resource_state(state_after);

	command_list->ResourceBarrier(1, &d3d12_resource_barrier);
}

void D3D12_Command_List::apply(Pipeline_State *pipeline_state)
{
	D3D12_Pipeline_State *internal_pipeline_state = (D3D12_Pipeline_State *)pipeline_state;
	D3D12_Root_Signature *internal_root_signature = (D3D12_Root_Signature *)pipeline_state->root_signature;

	command_list->SetPipelineState(internal_pipeline_state->get());
	if (pipeline_state->type == PIPELINE_TYPE_COMPUTE) {
		command_list->SetComputeRootSignature(internal_root_signature->d3d12_root_signature.Get());
	} else if (pipeline_state->type == PIPELINE_TYPE_GRAPHICS) {
		command_list->SetGraphicsRootSignature(internal_root_signature->d3d12_root_signature.Get());
		command_list->IASetPrimitiveTopology(to_d3d12_primitive_topology(pipeline_state->primitive_type));
	}
	Descriptor_Heap_Pool *descriptor_pool = render_device->descriptor_pool;

	ID3D12DescriptorHeap *descriptor_heaps[] = { descriptor_pool->cbsrua_descriptor_heap.get(), descriptor_pool->sampler_descriptor_heap.get() };
	command_list->SetDescriptorHeaps(2, descriptor_heaps);

	last_set_root_signature = static_cast<D3D12_Root_Signature *>(pipeline_state->root_signature);
}

void D3D12_Command_List::set_compute_constants(u32 shader_register, u32 shader_space, u32 data_size, void *data)
{
	assert((data_size % 4) == 0);

	u32 parameter_index = last_set_root_signature->get_parameter_index(shader_register, shader_space, CONSTANT_BUFFER_REGISTER);
	command_list->SetComputeRoot32BitConstants(parameter_index, data_size / 4, data, 0);
}

void D3D12_Command_List::set_compute_descriptor_table(u32 shader_register, u32 shader_space, Shader_Register register_type, GPU_Descriptor *base_descriptor)
{
	D3D12_GPU_Descriptor *internal_base_descriptor = static_cast<D3D12_GPU_Descriptor *>(base_descriptor);

	u32 parameter_index = last_set_root_signature->get_parameter_index(shader_register, shader_space, register_type);
	command_list->SetComputeRootDescriptorTable(parameter_index, internal_base_descriptor->gpu_handle);
}

void  D3D12_Command_List::dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z = 1)
{
	command_list->Dispatch(group_count_x, group_count_y, group_count_z);
}
void  D3D12_Command_List::set_graphics_root_signature(Root_Signature *root_signature)
{
	D3D12_Root_Signature *internal_root_signature = static_cast<D3D12_Root_Signature *>(root_signature);
	last_set_root_signature = internal_root_signature;
	
	command_list->SetGraphicsRootSignature(internal_root_signature->get());
}
void  D3D12_Command_List::set_primitive_type(Primitive_Type primitive_type)
{
	command_list->IASetPrimitiveTopology(to_d3d12_primitive_topology(primitive_type));
}

void  D3D12_Command_List::set_viewport(Viewport viewport, bool setup_clip_rect)
{
	D3D12_VIEWPORT d3d12_viewport;
	ZeroMemory(&d3d12_viewport, sizeof(D3D12_VIEWPORT));
	d3d12_viewport.TopLeftX = viewport.x;
	d3d12_viewport.TopLeftY = viewport.y;
	d3d12_viewport.Width = viewport.width;
	d3d12_viewport.Height = viewport.height;
	d3d12_viewport.MinDepth = viewport.min_depth;
	d3d12_viewport.MaxDepth = viewport.max_depth;
	command_list->RSSetViewports(1, &d3d12_viewport);
	
	if (setup_clip_rect) {
		Rect_u32 clip_rect;
		clip_rect.x = static_cast<u32>(viewport.x);
		clip_rect.y = static_cast<u32>(viewport.y);
		clip_rect.width = static_cast<u32>(viewport.width);
		clip_rect.height = static_cast<u32>(viewport.height);
		set_clip_rect(clip_rect);
	}
}
void  D3D12_Command_List::set_clip_rect(Rect_u32 clip_rect)
{
	assert(clip_rect.width > 0);
	assert(clip_rect.height > 0);

	D3D12_RECT d3d12_clip_rect;
	ZeroMemory(&d3d12_clip_rect, sizeof(D3D12_RECT));
	d3d12_clip_rect.left = clip_rect.x;
	d3d12_clip_rect.top = clip_rect.y;
	d3d12_clip_rect.right = clip_rect.x + clip_rect.width;
	d3d12_clip_rect.bottom = clip_rect.y + clip_rect.height;

	command_list->RSSetScissorRects(1, &d3d12_clip_rect);
}

void  D3D12_Command_List::clear_render_target_view(RTV_Descriptor *descriptor, const Color &color)
{
	D3D12_CPU_Descriptor *internal_descriptor = static_cast<D3D12_CPU_Descriptor *>(descriptor);

	float rgba[4] = { color.value.x, color.value.y, color.value.z, color.value.w };
	command_list->ClearRenderTargetView(internal_descriptor->cpu_handle, rgba, 0, NULL);
}

void  D3D12_Command_List::clear_depth_stencil_view(DSV_Descriptor *descriptor, float depth, u8 stencil)
{
	D3D12_CPU_Descriptor *internal_descriptor = static_cast<D3D12_CPU_Descriptor *>(descriptor);

	command_list->ClearDepthStencilView(internal_descriptor->cpu_handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, NULL);
}

void  D3D12_Command_List::set_render_target(RTV_Descriptor *render_target_descriptor, DSV_Descriptor *depth_stencil_descriptor)
{
	D3D12_CPU_Descriptor *internal_render_target_descriptor = render_target_descriptor ? static_cast<D3D12_CPU_Descriptor *>(render_target_descriptor) : NULL;
	D3D12_CPU_Descriptor *internal_depth_stencil_descriptor = static_cast<D3D12_CPU_Descriptor *>(depth_stencil_descriptor);

	u32 render_targets_number = render_target_descriptor ? 1 : 0;
	command_list->OMSetRenderTargets(render_targets_number, &internal_render_target_descriptor->cpu_handle, FALSE, &internal_depth_stencil_descriptor->cpu_handle);
}

void  D3D12_Command_List::set_vertex_buffer(Buffer *buffer)
{
	D3D12_Buffer *internal_buffer = (D3D12_Buffer *)buffer;
	D3D12_Base_Buffer *internal_base_buffer = internal_buffer->current_buffer();

	assert((internal_base_buffer->total_size % internal_base_buffer->stride) == 0);

	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
	vertex_buffer_view.BufferLocation = internal_base_buffer->gpu_address();
	vertex_buffer_view.SizeInBytes = internal_base_buffer->stride * internal_base_buffer->count;
	//vertex_buffer_view.SizeInBytes = internal_base_buffer->total_size;
	vertex_buffer_view.StrideInBytes = internal_base_buffer->stride;

	command_list->IASetVertexBuffers(0, 1, &vertex_buffer_view);
}

void  D3D12_Command_List::set_index_buffer(Buffer *buffer)
{
	D3D12_Buffer *internal_buffer = (D3D12_Buffer *)buffer;
	D3D12_Base_Buffer *internal_base_buffer = internal_buffer->current_buffer();

	D3D12_INDEX_BUFFER_VIEW index_buffer_view;
	index_buffer_view.BufferLocation = internal_base_buffer->gpu_address();
	//index_buffer_view.SizeInBytes = internal_base_buffer->total_size;
	index_buffer_view.SizeInBytes = internal_base_buffer->stride * internal_base_buffer->count;
	index_buffer_view.Format = DXGI_FORMAT_R32_UINT;
	
	command_list->IASetIndexBuffer(&index_buffer_view);
}

void D3D12_Command_List::set_graphics_constant_buffer(u32 shader_register, u32 shader_space, Buffer *constant_buffer)
{
	u32 parameter_index = last_set_root_signature->get_parameter_index(shader_register, shader_space, CONSTANT_BUFFER_REGISTER);
	command_list->SetGraphicsRootConstantBufferView(parameter_index, constant_buffer->gpu_virtual_address());
}

void D3D12_Command_List::set_graphics_constants(u32 shader_register, u32 shader_space, u32 data_size, void *data)
{
	assert((data_size % 4) == 0);

	u32 parameter_index = last_set_root_signature->get_parameter_index(shader_register, shader_space, CONSTANT_BUFFER_REGISTER);
	command_list->SetGraphicsRoot32BitConstants(parameter_index, data_size / 4, data, 0);
}

void  D3D12_Command_List::set_graphics_descriptor_table(u32 shader_register, u32 shader_space, Shader_Register register_type, GPU_Descriptor *base_descriptor)
{
	D3D12_GPU_Descriptor *internal_base_descriptor = static_cast<D3D12_GPU_Descriptor *>(base_descriptor);

	u32 parameter_index = last_set_root_signature->get_parameter_index(shader_register, shader_space, register_type);
	command_list->SetGraphicsRootDescriptorTable(parameter_index, internal_base_descriptor->gpu_handle);
}

void  D3D12_Command_List::draw(u32 vertex_count)
{
	command_list->DrawInstanced(vertex_count, 1, 0, 0);
}

void  D3D12_Command_List::draw_indexed(u32 index_count)
{
	command_list->DrawIndexedInstanced(index_count, 1, 0, 0, 0);
}

void D3D12_Command_List::draw_indexed(u32 index_count, u32 index_offset, u32 vertex_offset)
{
	command_list->DrawIndexedInstanced(index_count, 1, index_offset, vertex_offset, 0);
}

D3D12_Fence::D3D12_Fence(ComPtr<ID3D12Device> &device, u64 initial_expected_value)
{
	handle = create_event_handle();
	expected_value = initial_expected_value;
	HR(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(d3d12_fence.ReleaseAndGetAddressOf())));
}

D3D12_Fence::~D3D12_Fence()
{
	expected_value = 0;
	close_event_handle(handle);
}

bool D3D12_Fence::wait_for_gpu()
{
	u64 completed_value = get_completed_value();
	if (completed_value < expected_value) {
		d3d12_fence->SetEventOnCompletion(expected_value, handle);
		WaitForSingleObject(handle, INFINITE);
		return true;
	}
	return false;
}

bool D3D12_Fence::wait_for_gpu(u64 other_expected_value)
{
	if (other_expected_value < expected_value) {
		d3d12_fence->SetEventOnCompletion(expected_value, handle);
		WaitForSingleObject(handle, INFINITE);
		return true;
	}
	return false;
}

u64 D3D12_Fence::get_completed_value()
{
	return d3d12_fence->GetCompletedValue();
}

u64 D3D12_Fence::increment_expected_value()
{
	return ++expected_value;
}

ID3D12Fence *D3D12_Fence::get()
{
	return d3d12_fence.Get();
}

D3D12_Command_Queue::D3D12_Command_Queue(ComPtr<ID3D12Device> &device, Command_List_Type command_list_type)
{

	D3D12_COMMAND_QUEUE_DESC command_queue_desc;
	ZeroMemory(&command_queue_desc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	command_queue_desc.Type = to_d3d12_command_list_type(command_list_type);

	HR(device->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(d3d12_command_queue.ReleaseAndGetAddressOf())));
}

D3D12_Command_Queue::~D3D12_Command_Queue()
{
}

void D3D12_Command_Queue::signal(Fence *fence)
{
	D3D12_Fence *internal_fence = static_cast<D3D12_Fence *>(fence);

	d3d12_command_queue->Signal(internal_fence->get(), internal_fence->expected_value);
}

void D3D12_Command_Queue::wait(Fence *fence)
{
	D3D12_Fence *internal_fence = static_cast<D3D12_Fence *>(fence);
	
	d3d12_command_queue->Wait(internal_fence->get(), internal_fence->expected_value);
}

void D3D12_Command_Queue::execute_command_list(Command_List *command_list)
{
	D3D12_Command_List *internal_command_list = static_cast<D3D12_Command_List *>(command_list);

	ID3D12CommandList *command_lists[] = { internal_command_list->get()};
	d3d12_command_queue->ExecuteCommandLists(1, command_lists);
}

ID3D12CommandQueue *D3D12_Command_Queue::get()
{
	return d3d12_command_queue.Get();
}

D3D12_Render_Device::D3D12_Render_Device(u64 initial_expected_value, ComPtr<ID3D12Device> &_device)
{
	frame_number = initial_expected_value;
	device = _device;

	current_upload_command_list = new D3D12_Command_List(COMMAND_LIST_TYPE_COPY, this);
	current_upload_command_list->reset();
	flight_command_lists.push({frame_number, current_upload_command_list });

	copy_fence = static_cast<D3D12_Fence *>(create_fence(initial_expected_value));
	copy_queue = static_cast<D3D12_Command_Queue *>(create_command_queue(COMMAND_LIST_TYPE_COPY, "Render device copy queue"));

	descriptor_pool = new Descriptor_Heap_Pool();
	descriptor_pool->allocate_pool(device, 4000);
}

D3D12_Render_Device::~D3D12_Render_Device()
{
}

void D3D12_Render_Device::finish_frame(u64 completed_frame)
{
	while (!resource_release_queue.empty() && (resource_release_queue.front().first <= completed_frame)) {
		ComPtr<ID3D12Resource> buffer = resource_release_queue.front().second;
		//set_name(buffer.Get(), "Upload buffer [name {}] [frame {} completed]", debug_name, _frame_number)
		//set_name(buffer.Get(), "Upload buffer", frame_number);
		resource_release_queue.pop();
	}

	while (!resource_release_queue2.empty() && (resource_release_queue2.front().first <= completed_frame)) {
		D3D12_Resource *resource = resource_release_queue2.front().second;
		resource_release_queue2.pop();
		DELETE_PTR(resource);
	}

	while (!flight_command_lists.empty() && (flight_command_lists.front().first <= completed_frame)) {
		D3D12_Command_List *command_list = flight_command_lists.front().second;
		flight_command_lists.pop();
		
		completed_command_lists.push(command_list);
	}

	D3D12_Buffer *buffer = NULL;
	For(buffers, buffer) {
		buffer->finish_frame(completed_frame);
	}
	
	frame_number++;
	copy_fence->increment_expected_value();

	if (completed_command_lists.empty()) {
		current_upload_command_list = new D3D12_Command_List(COMMAND_LIST_TYPE_COPY, this);
		current_upload_command_list->reset();
		flight_command_lists.push({ frame_number, current_upload_command_list });
	} else {
		current_upload_command_list = completed_command_lists.front();
		current_upload_command_list->reset();
		completed_command_lists.pop();
	}

	For(buffers, buffer) {
		buffer->begin_frame();
	}
}

Buffer *D3D12_Render_Device::create_buffer(Buffer_Desc *buffer_desc)
{
	D3D12_Buffer *buffer = new D3D12_Buffer(this, buffer_desc);
	buffer->begin_frame();
	buffers.push(buffer);
	return buffer;
}

Texture *D3D12_Render_Device::create_texture(Texture_Desc *texture_desc)
{
	return new D3D12_Texture(this, texture_desc);
}

Fence *D3D12_Render_Device::create_fence(u64 initial_expected_value)
{
	return new D3D12_Fence(device, initial_expected_value);
}

Sampler *D3D12_Render_Device::create_sampler(Sampler_Filter filter, Address_Mode uvw)
{
	return new D3D12_Sampler(this, filter, uvw);
}

Command_List *D3D12_Render_Device::create_command_list(Command_List_Type command_list_type)
{
	D3D12_Command_List *new_command_list = new D3D12_Command_List(command_list_type, this);
	return new_command_list;
}

Copy_Command_List *D3D12_Render_Device::create_copy_command_list()
{
	return (Copy_Command_List *)create_command_list(COMMAND_LIST_TYPE_COPY);
}

Compute_Command_List *D3D12_Render_Device::create_compute_command_list()
{
	return (Compute_Command_List *)create_command_list(COMMAND_LIST_TYPE_COMPUTE);
}

Graphics_Command_List *D3D12_Render_Device::create_graphics_command_list()
{
	return (Graphics_Command_List *)create_command_list(COMMAND_LIST_TYPE_DIRECT);
}

Command_Queue *D3D12_Render_Device::create_command_queue(Command_List_Type command_list_type, const char *name)
{
	D3D12_Command_Queue *command_queue = new D3D12_Command_Queue(device, command_list_type);
	if (name) { set_name(command_queue->get(), name); }
	return command_queue;
}

Root_Signature *D3D12_Render_Device::create_root_signature()
{
	D3D12_Root_Signature *root_signature = new D3D12_Root_Signature(this);
	return root_signature;
}

Pipeline_State *D3D12_Render_Device::create_pipeline_state(Compute_Pipeline_Desc *pipeline_desc)
{
	D3D12_Pipeline_State *pipeline_state = new D3D12_Pipeline_State(device, pipeline_desc);
	return pipeline_state;
}

Pipeline_State *D3D12_Render_Device::create_pipeline_state(Graphics_Pipeline_Desc *pipeline_desc)
{
	D3D12_Pipeline_State *pipeline_state = new D3D12_Pipeline_State(device, pipeline_desc);
	return pipeline_state;
}

Fence *D3D12_Render_Device::execute_uploading()
{
	current_upload_command_list->close();
	copy_queue->execute_command_list(current_upload_command_list);
	copy_queue->signal(copy_fence);
	return copy_fence;
}

void D3D12_Render_Device::safe_release(D3D12_Resource *resource, u64 resource_frame_number)
{
	if (resource_frame_number == 0) {
		resource_frame_number = frame_number;
	}
	resource_release_queue2.push({ resource_frame_number, resource });
}

void D3D12_Render_Device::safe_release(ComPtr<ID3D12Resource> &resource, u64 resource_frame_number)
{
	if (resource_frame_number == 0) {
		resource_frame_number = frame_number;
	}
	resource_release_queue.push({ resource_frame_number, resource });
}

D3D12_Command_List *D3D12_Render_Device::upload_command_list()
{
	return current_upload_command_list;
}

Resource_Allocation_Info D3D12_Render_Device::resource_allocation_info(Resource_Desc *resource_desc)
{
	D3D12_RESOURCE_DESC d3d12_resource_desc = resource_desc->d3d12_resource_desc();
	D3D12_RESOURCE_ALLOCATION_INFO allocation_info = device->GetResourceAllocationInfo(0, 1, &d3d12_resource_desc);
	return { allocation_info.SizeInBytes, allocation_info.Alignment };
}

GPU_Descriptor *D3D12_Render_Device::base_sampler_descriptor()
{
	return &descriptor_pool->base_sampler_descriptor;
}

GPU_Descriptor *D3D12_Render_Device::base_shader_resource_descriptor()
{
	return &descriptor_pool->base_cbsrua_descriptor;
}

D3D12_Swap_Chain::D3D12_Swap_Chain(bool allow_tearing, u32 buffer_count, u32 width, u32 height, HWND handle, Command_Queue *command_queue) : back_buffer_count(buffer_count)
{
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc;
	ZeroMemory(&swap_chain_desc, sizeof(DXGI_SWAP_CHAIN_DESC1));
	swap_chain_desc.Width = width;
	swap_chain_desc.Height = height;
	swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount = back_buffer_count;
	swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swap_chain_desc.Flags = (allow_tearing && check_tearing_support()) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	u32 factory_flags = 0;
#ifdef _DEBUG
	factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	ComPtr<IDXGIFactory4> dxgi_factory;
	HR(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&dxgi_factory)));

	D3D12_Command_Queue *internal_command_queue = static_cast<D3D12_Command_Queue *>(command_queue);

	ComPtr<IDXGISwapChain1> dxgi_swap_chain1;
	HR(dxgi_factory->CreateSwapChainForHwnd(internal_command_queue->get(), handle, &swap_chain_desc, NULL, NULL, &dxgi_swap_chain1));

	HR(dxgi_factory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER));

	HR(dxgi_swap_chain1.As(&dxgi_swap_chain));

	back_buffers.resize(back_buffer_count);
	//memset(back_buffers.to_void_ptr(), 0xff, back_buffers.get_size());
	for (u32 i = 0; i < back_buffer_count; i++) {
		back_buffers[i] = NULL;
	}

	Texture_Desc depth_texture_desc;
	depth_texture_desc.dimension = TEXTURE_DIMENSION_2D;
	depth_texture_desc.width = width;
	depth_texture_desc.height = height;
	depth_texture_desc.format = DXGI_FORMAT_D32_FLOAT;
	depth_texture_desc.flags = DEPTH_STENCIL_RESOURCE;
	depth_texture_desc.clear_value = Clear_Value(1.0f, 0);
	depth_texture_desc.resource_state = RESOURCE_STATE_DEPTH_WRITE;

	depth_buffer = new D3D12_Texture(internal_render_device_reference, &depth_texture_desc);
}

D3D12_Swap_Chain::~D3D12_Swap_Chain()
{
}

void D3D12_Swap_Chain::resize(u32 width, u32 height)
{

}

void D3D12_Swap_Chain::present(u32 sync_interval, u32 flags)
{
	HR(dxgi_swap_chain->Present(sync_interval, flags));
}

Texture *D3D12_Swap_Chain::get_back_buffer()
{
	u32 back_buffer_index = get_current_back_buffer_index();
	if (back_buffers[back_buffer_index] == NULL) {
		ComPtr<ID3D12Resource> resource;
		HR(dxgi_swap_chain->GetBuffer(get_current_back_buffer_index(), IID_PPV_ARGS(resource.ReleaseAndGetAddressOf())));
		
		back_buffers[back_buffer_index] = new D3D12_Texture(internal_render_device_reference, resource);;
	}
	return back_buffers[back_buffer_index];
}

Texture *D3D12_Swap_Chain::get_depth_stencil_buffer()
{
	return depth_buffer;
}

u32 D3D12_Swap_Chain::get_current_back_buffer_count()
{
	return back_buffer_count;
}

u32 D3D12_Swap_Chain::get_current_back_buffer_index()
{
	return dxgi_swap_chain->GetCurrentBackBufferIndex();
}

bool create_d3d12_device(ComPtr<ID3D12Device> &device)
{
	bool result = false;
	u32 factory_flags = 0;
#ifdef _DEBUG
	factory_flags |= DXGI_CREATE_FACTORY_DEBUG;

	ComPtr<ID3D12Debug> debug_interface;
	HR(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)));
	debug_interface->EnableDebugLayer();
#endif
	ComPtr<IDXGIFactory4> dxgi_factory;
	HR(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&dxgi_factory)));

	ComPtr<IDXGIAdapter1> dxgi_adapter1;
	for (u32 i = 0; dxgi_factory->EnumAdapters1(i, dxgi_adapter1.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; i++) {
		DXGI_ADAPTER_DESC1 dxgi_adapter_desc;
		dxgi_adapter1->GetDesc1(&dxgi_adapter_desc);
		if (dxgi_adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}
		if (SUCCEEDED(D3D12CreateDevice(dxgi_adapter1.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(device.ReleaseAndGetAddressOf())))) {
			char *device_desc = to_string(dxgi_adapter_desc.Description);
			print("init_GPU_device: {} created a new D3D12 device.", device_desc);
			DELETE_PTR(device_desc);
			result = true;
			break;
		}
	}
#ifdef _DEBUG
	ComPtr<ID3D12InfoQueue> info_queue;
	if (result && SUCCEEDED(device.As(&info_queue))) {
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_INFO, TRUE);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_MESSAGE, TRUE);

		D3D12_MESSAGE_SEVERITY apathetic_levels[] = {
			D3D12_MESSAGE_SEVERITY_INFO
		};

		D3D12_MESSAGE_ID apathetic_messages[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
		};

		D3D12_INFO_QUEUE_FILTER message_filter;
		ZeroMemory(&message_filter, sizeof(D3D12_INFO_QUEUE_FILTER));
		message_filter.DenyList.NumSeverities = _countof(apathetic_levels);
		message_filter.DenyList.pSeverityList = apathetic_levels;
		message_filter.DenyList.NumIDs = _countof(apathetic_messages);
		message_filter.DenyList.pIDList = apathetic_messages;

		HR(info_queue->PushStorageFilter(&message_filter));
	}
#endif
	if (!result) {
		print("init_GPU_device: Failed to create D3D12 device. There are no adapters support D3D12.");
	}
	return result;
}

D3D12_Render_Device *create_d3d12_render_device(u64 initial_expected_value)
{
	ComPtr<ID3D12Device> device;
	if (create_d3d12_device(device)) {
		internal_render_device_reference = new D3D12_Render_Device(initial_expected_value, device);
		return internal_render_device_reference;
	}
	return NULL;
}
