#include <string.h>

#include "root_signature.h"
#include "../../libs/str.h"
#include "../../sys/sys.h"
#include "../../sys/utils.h"

static const char *str_shader_register_types[] = {
	"Constant Buffer",
	"Shader Resource",
	"Unordered Access",
	"Sampler"
};

static D3D12_SHADER_VISIBILITY shader_visibility_to_d3d12(Shader_Visibility shader_visibility)
{
	switch (shader_visibility) {
		case VISIBLE_TO_ALL:
			return D3D12_SHADER_VISIBILITY_ALL;
		case VISIBLE_TO_VERTEX_SHADER:
			return D3D12_SHADER_VISIBILITY_VERTEX;
		case VISIBLE_TO_HULL_SHADER:
			return D3D12_SHADER_VISIBILITY_HULL;
		case VISIBLE_TO_DOMAIN_SHADER:
			return D3D12_SHADER_VISIBILITY_DOMAIN;
		case VISIBLE_TO_GEOMETRY_SHADER:
			return D3D12_SHADER_VISIBILITY_GEOMETRY;
		case VISIBLE_TO_PIXEL_SHADER:
			return D3D12_SHADER_VISIBILITY_PIXEL;
		case VISIBLE_TO_AMPLIFICATION_SHADER:
			return D3D12_SHADER_VISIBILITY_AMPLIFICATION;
		case VISIBLE_TO_MESH_SHADER:
			return D3D12_SHADER_VISIBILITY_MESH;
	}
	assert(false);
	return (D3D12_SHADER_VISIBILITY)0;
}

Root_Signature::Root_Signature()
{
}

Root_Signature::~Root_Signature()
{
	free_memory(&parameters_descriptor_ranges);
}

void Root_Signature::create(Gpu_Device &device, u32 access_flags)
{
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
	ZeroMemory(&root_signature_desc, sizeof(D3D12_VERSIONED_ROOT_SIGNATURE_DESC));
	root_signature_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	root_signature_desc.Desc_1_1.NumParameters = parameters.count;
	root_signature_desc.Desc_1_1.pParameters = parameters.items;
	
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
	HR(device->CreateRootSignature(0, signature_blob->GetBufferPointer(), signature_blob->GetBufferSize(), IID_PPV_ARGS(release_and_get_address())));

	free_memory(&parameters_descriptor_ranges);
	free_memory(&ranges);
}

void Root_Signature::store_parameter_index(u32 parameter_index, u32 shader_register, u32 shader_space, Root_Parameter_Type parameter_type)
{
	assert(shader_register <= HLSL_REGISTRE_COUNT);
	assert(shader_space <= HLSL_SPACE_COUNT);

	u32 temp = parameter_index_table[shader_register][shader_space].get_parameter_index(parameter_type);
	if (temp != UINT_MAX) {
		error("A parameter index has already been set for the shader register(register = {}, space = {}, type = {}).", shader_register, shader_space, str_shader_register_types[static_cast<u32>(parameter_type)]);
	} else {
		parameter_index_table[shader_register][shader_space].set_parameter_index(parameter_index, parameter_type);
	}
}

u32 Root_Signature::get_parameter_index(u32 shader_register, u32 shader_space, Root_Parameter_Type parameter_type)
{
	assert(shader_register <= HLSL_REGISTRE_COUNT);
	assert(shader_space <= HLSL_SPACE_COUNT);

	u32 parameter_index = parameter_index_table[shader_register][shader_space].get_parameter_index(parameter_type);
	if (parameter_index == UINT_MAX) {
		error("A parameter index has not been set yet for the shader register(register = {}, space = {}, type = {}).", shader_register, shader_space, str_shader_register_types[static_cast<u32>(parameter_type)]);
	}
	return parameter_index;
}

u32 Root_Signature::add_32bit_constants_parameter(u32 shader_register, u32 shader_space, u32 struct_size, Shader_Visibility shader_visibility)
{
	assert(struct_size % 4 == 0);

	D3D12_ROOT_PARAMETER1 parameter;
	ZeroMemory(&parameter, sizeof(D3D12_ROOT_PARAMETER1));
	parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	parameter.Constants.ShaderRegister = shader_register;
	parameter.Constants.RegisterSpace = shader_space;
	parameter.Constants.Num32BitValues = struct_size / 4;
	parameter.ShaderVisibility = shader_visibility_to_d3d12(shader_visibility);

	u32 parameter_index = parameters.push(parameter);
	store_parameter_index(parameter_index, shader_register, shader_space, ROOT_PARAMETER_CONSTANT_BUFFER);
	return parameter_index;
}

u32 Root_Signature::add_cb_descriptor_table_parameter(u32 shader_register, u32 shader_space, Shader_Visibility shader_visibility)
{
	D3D12_DESCRIPTOR_RANGE1 *descriptor_range = new D3D12_DESCRIPTOR_RANGE1();
	ZeroMemory(descriptor_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
	descriptor_range->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descriptor_range->NumDescriptors = 1;
	descriptor_range->BaseShaderRegister = shader_register;
	descriptor_range->RegisterSpace = shader_space;
	descriptor_range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	ranges.push(descriptor_range);

	D3D12_ROOT_PARAMETER1 parameter;
	ZeroMemory(&parameter, sizeof(D3D12_ROOT_PARAMETER1));
	parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	parameter.DescriptorTable.NumDescriptorRanges = 1;
	parameter.DescriptorTable.pDescriptorRanges = ranges.last();
	parameter.ShaderVisibility = shader_visibility_to_d3d12(shader_visibility);

	u32 parameter_index = parameters.push(parameter);
	store_parameter_index(parameter_index, shader_register, shader_space, ROOT_PARAMETER_CONSTANT_BUFFER);
	return parameter_index;
}

u32 Root_Signature::add_sr_descriptor_table_parameter(u32 shader_register, u32 shader_space, Shader_Visibility shader_visibility)
{
	D3D12_DESCRIPTOR_RANGE1 *descriptor_range = new D3D12_DESCRIPTOR_RANGE1();
	ZeroMemory(descriptor_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
	descriptor_range->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptor_range->NumDescriptors = 1;
	descriptor_range->BaseShaderRegister = shader_register;
	descriptor_range->RegisterSpace = shader_space;
	descriptor_range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	ranges.push(descriptor_range);

	D3D12_ROOT_PARAMETER1 parameter;
	ZeroMemory(&parameter, sizeof(D3D12_ROOT_PARAMETER1));
	parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	parameter.DescriptorTable.NumDescriptorRanges = 1;
	parameter.DescriptorTable.pDescriptorRanges = ranges.last();
	parameter.ShaderVisibility = shader_visibility_to_d3d12(shader_visibility);

	u32 parameter_index = parameters.push(parameter);
	store_parameter_index(parameter_index, shader_register, shader_space, ROOT_PARAMETER_SHADER_RESOURCE);
	return parameter_index;
}

u32 Root_Signature::add_sr_descriptor_table_parameter_xxx(u32 shader_register, u32 shader_space, Shader_Visibility shader_visibility)
{
	D3D12_DESCRIPTOR_RANGE1 *descriptor_range = new D3D12_DESCRIPTOR_RANGE1();
	ZeroMemory(descriptor_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
	descriptor_range->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptor_range->NumDescriptors = UINT_MAX;
	descriptor_range->BaseShaderRegister = shader_register;
	descriptor_range->RegisterSpace = shader_space;
	descriptor_range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	descriptor_range->Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

	ranges.push(descriptor_range);

	D3D12_ROOT_PARAMETER1 parameter;
	ZeroMemory(&parameter, sizeof(D3D12_ROOT_PARAMETER1));
	parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	parameter.DescriptorTable.NumDescriptorRanges = 1;
	parameter.DescriptorTable.pDescriptorRanges = ranges.last();
	parameter.ShaderVisibility = shader_visibility_to_d3d12(shader_visibility);

	u32 parameter_index = parameters.push(parameter);
	store_parameter_index(parameter_index, shader_register, shader_space, ROOT_PARAMETER_SHADER_RESOURCE);
	return parameter_index;
}

u32 Root_Signature::add_ua_descriptor_table_parameter(u32 shader_register, u32 shader_space, Shader_Visibility shader_visibility)
{
	D3D12_DESCRIPTOR_RANGE1 *descriptor_range = new D3D12_DESCRIPTOR_RANGE1();
	ZeroMemory(descriptor_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
	descriptor_range->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	descriptor_range->NumDescriptors = 1;
	descriptor_range->BaseShaderRegister = shader_register;
	descriptor_range->RegisterSpace = shader_space;
	descriptor_range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	ranges.push(descriptor_range);

	D3D12_ROOT_PARAMETER1 parameter;
	ZeroMemory(&parameter, sizeof(D3D12_ROOT_PARAMETER1));
	parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	parameter.DescriptorTable.NumDescriptorRanges = 1;
	parameter.DescriptorTable.pDescriptorRanges = ranges.last();
	parameter.ShaderVisibility = shader_visibility_to_d3d12(shader_visibility);

	u32 parameter_index = parameters.push(parameter);
	store_parameter_index(parameter_index, shader_register, shader_space, ROOT_PARAMETER_UNORDERED_ACESS_RESOURCE);
	return parameter_index;
}

u32 Root_Signature::add_ua_descriptor_table_parameter(u32 shader_register, u32 shader_space, u32 number_descriptors, Shader_Visibility shader_visibility)
{
	D3D12_DESCRIPTOR_RANGE1 *descriptor_range = new D3D12_DESCRIPTOR_RANGE1();
	ZeroMemory(descriptor_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
	descriptor_range->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	descriptor_range->NumDescriptors = number_descriptors;
	descriptor_range->BaseShaderRegister = shader_register;
	descriptor_range->RegisterSpace = shader_space;
	descriptor_range->Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	descriptor_range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	ranges.push(descriptor_range);

	D3D12_ROOT_PARAMETER1 parameter;
	ZeroMemory(&parameter, sizeof(D3D12_ROOT_PARAMETER1));
	parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	parameter.DescriptorTable.NumDescriptorRanges = 1;
	parameter.DescriptorTable.pDescriptorRanges = ranges.last();
	parameter.ShaderVisibility = shader_visibility_to_d3d12(shader_visibility);

	u32 parameter_index = parameters.push(parameter);
	store_parameter_index(parameter_index, shader_register, shader_space, ROOT_PARAMETER_UNORDERED_ACESS_RESOURCE);
	return parameter_index;
}

u32 Root_Signature::add_sampler_descriptor_table_parameter(u32 shader_register, u32 shader_space, Shader_Visibility shader_visibility)
{
	D3D12_DESCRIPTOR_RANGE1 *descriptor_range = new D3D12_DESCRIPTOR_RANGE1();
	ZeroMemory(descriptor_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
	descriptor_range->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	descriptor_range->NumDescriptors = 1;
	descriptor_range->BaseShaderRegister = shader_register;
	descriptor_range->RegisterSpace = shader_space;
	descriptor_range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	ranges.push(descriptor_range);

	D3D12_ROOT_PARAMETER1 parameter;
	ZeroMemory(&parameter, sizeof(D3D12_ROOT_PARAMETER1));
	parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	parameter.DescriptorTable.NumDescriptorRanges = 1;
	parameter.DescriptorTable.pDescriptorRanges = ranges.last();
	parameter.ShaderVisibility = shader_visibility_to_d3d12(shader_visibility);

	u32 parameter_index = parameters.push(parameter);
	store_parameter_index(parameter_index, shader_register, shader_space, ROOT_PARAMETER_SAMPLER);
	return parameter_index;
}

void Root_Signature::begin_descriptor_table_parameter(u32 shader_register_space, Shader_Visibility shader_visibility)
{
	register_space = shader_register_space;
	visibility = shader_visibility;

	descriptor_ranges = new Descriptor_Ranges();
	parameters_descriptor_ranges.push(descriptor_ranges);
}

void Root_Signature::end_parameter()
{
	D3D12_ROOT_PARAMETER1 parameter;
	ZeroMemory(&parameter, sizeof(D3D12_ROOT_PARAMETER1));
	parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	parameter.DescriptorTable.NumDescriptorRanges = descriptor_ranges->count;
	parameter.DescriptorTable.pDescriptorRanges = descriptor_ranges->items;
	parameter.ShaderVisibility = shader_visibility_to_d3d12(visibility);

	parameters.push(parameter);
	descriptor_ranges = NULL;
}

void Root_Signature::add_descriptor_range(u32 shader_register, CB_Descriptor &descriptor)
{
	D3D12_DESCRIPTOR_RANGE1 descriptor_range;
	ZeroMemory(&descriptor_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
	descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descriptor_range.NumDescriptors = 1;
	descriptor_range.BaseShaderRegister = shader_register;
	descriptor_range.RegisterSpace = register_space;
	descriptor_range.OffsetInDescriptorsFromTableStart = descriptor.index;

	descriptor_ranges->push(descriptor_range);
}

void Root_Signature::add_descriptor_range(u32 shader_register, SR_Descriptor &descriptor)
{
	D3D12_DESCRIPTOR_RANGE1 descriptor_range;
	ZeroMemory(&descriptor_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
	descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptor_range.NumDescriptors = 1;
	descriptor_range.BaseShaderRegister = shader_register;
	descriptor_range.RegisterSpace = register_space;
	descriptor_range.OffsetInDescriptorsFromTableStart = descriptor.index;

	descriptor_ranges->push(descriptor_range);
}

void Root_Signature::add_descriptor_range(u32 shader_register, UA_Descriptor &descriptor)
{
	D3D12_DESCRIPTOR_RANGE1 descriptor_range;
	ZeroMemory(&descriptor_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
	descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	descriptor_range.NumDescriptors = 1;
	descriptor_range.BaseShaderRegister = shader_register;
	descriptor_range.RegisterSpace = register_space;
	descriptor_range.OffsetInDescriptorsFromTableStart = descriptor.index;

	descriptor_ranges->push(descriptor_range);
}

void Root_Signature::add_descriptor_range(u32 shader_register, Sampler_Descriptor &descriptor)
{
	D3D12_DESCRIPTOR_RANGE1 descriptor_range;
	ZeroMemory(&descriptor_range, sizeof(D3D12_DESCRIPTOR_RANGE1));
	descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	descriptor_range.NumDescriptors = 1;
	descriptor_range.BaseShaderRegister = shader_register;
	descriptor_range.RegisterSpace = register_space;
	descriptor_range.OffsetInDescriptorsFromTableStart = descriptor.index;

	descriptor_ranges->push(descriptor_range);
}

Root_Paramter_Index::~Root_Paramter_Index()
{
}

Root_Paramter_Index::Root_Paramter_Index()
{
	memset((void *)this, 0xff, sizeof(Root_Paramter_Index));
}

void Root_Paramter_Index::set_parameter_index(u32 parameter_index, Root_Parameter_Type parameter_type)
{
	switch (parameter_type) {
		case ROOT_PARAMETER_CONSTANT_BUFFER: {
			cb_parameter_index = parameter_index;
			break;
		}
		case ROOT_PARAMETER_SHADER_RESOURCE: {
			sr_parameter_index = parameter_index;
			break;
		}
		case ROOT_PARAMETER_UNORDERED_ACESS_RESOURCE: {
			ua_parameter_index = parameter_index;
			break;
		}
		case ROOT_PARAMETER_SAMPLER: {
			sampler_parameter_index = parameter_index;
			break;
		}
		default: {
			assert(false);
		}
	}
}

u32 Root_Paramter_Index::get_parameter_index(Root_Parameter_Type parameter_type)
{
	switch (parameter_type) {
		case ROOT_PARAMETER_CONSTANT_BUFFER:
			return cb_parameter_index;
		case ROOT_PARAMETER_SHADER_RESOURCE:
			return sr_parameter_index;
		case ROOT_PARAMETER_UNORDERED_ACESS_RESOURCE:
			return ua_parameter_index;
		case ROOT_PARAMETER_SAMPLER:
			return sampler_parameter_index;
	}
	assert(false);
	return 0;
}
