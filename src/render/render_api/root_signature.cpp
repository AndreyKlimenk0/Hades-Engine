#include "root_signature.h"
#include "../../sys/utils.h"

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
		default: {
			assert(false);
		}
	}
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
	assert(access_flags != 0);

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
