#ifndef RENDER_API_ROOT_SIGNATURE_H
#define RENDER_API_ROOT_SIGNATURE_H

#include <limits.h>
#include <d3d12.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include "base.h"
#include "d3d12_object.h"
#include "descriptor_heap.h"
#include "../../libs/number_types.h"
#include "../../libs/structures/array.h"

enum Shader_Visibility {
	VISIBLE_TO_ALL,
	VISIBLE_TO_VERTEX_SHADER,
	VISIBLE_TO_HULL_SHADER,
	VISIBLE_TO_DOMAIN_SHADER,
	VISIBLE_TO_GEOMETRY_SHADER,
	VISIBLE_TO_PIXEL_SHADER,
	VISIBLE_TO_AMPLIFICATION_SHADER,
	VISIBLE_TO_MESH_SHADER
};

const u32 ALLOW_INPUT_LAYOUT_ACCESS = 0x1;
const u32 ALLOW_VERTEX_SHADER_ACCESS = 0x2;
const u32 ALLOW_PIXEL_SHADER_ACCESS = 0x4;
const u32 ALLOW_HULL_SHADER_ACCESS = 0x8;
const u32 ALLOW_DOMAIN_SHADER_ACCESS = 0x10;
const u32 ALLOW_GEOMETRY_SHADER_ACCESS = 0x20;
const u32 ALLOW_AMPLIFICATION_SHADER_ACCESS = 0x40;
const u32 ALLOW_MESH_SHADER_ACCESS = 0x80;

typedef Array<D3D12_DESCRIPTOR_RANGE1> Descriptor_Ranges;

enum Binding_Type {
	BIND_RESOURCE_RESOURCE,
	BIND_CONSTATNT_BUFFER,
	BIND_UNORDERED_ACESS_RESOURCE,
};

// There is a string representation of the next enum in the .cpp file
// Don't forget to update it.
enum Root_Parameter_Type {
	ROOT_PARAMETER_CONSTANT_BUFFER = 0,
	ROOT_PARAMETER_SHADER_RESOURCE = 1,
	ROOT_PARAMETER_UNORDERED_ACESS_RESOURCE = 2,
	ROOT_PARAMETER_SAMPLER = 3
};

struct Root_Paramter_Index {
	~Root_Paramter_Index();
	Root_Paramter_Index();

	u32 cb_parameter_index;
	u32 sr_parameter_index;
	u32 ua_parameter_index;
	u32 sampler_parameter_index;

	void set_parameter_index(u32 parameter_index, Root_Parameter_Type parameter_type);
	u32 get_parameter_index(Root_Parameter_Type parameter_type);
};

const u32 HLSL_REGISTRE_COUNT = 20;
const u32 HLSL_SPACE_COUNT = 20;

struct Root_Signature : D3D12_Object<ID3D12RootSignature> {
	Root_Signature();
	~Root_Signature();

	u32 register_space;
	Shader_Visibility visibility;
	Descriptor_Ranges *descriptor_ranges = NULL;

	Array<D3D12_ROOT_PARAMETER1> parameters;
	Array<D3D12_DESCRIPTOR_RANGE1 *> ranges;
	Array<D3D12_STATIC_SAMPLER_DESC> samplers;

	Array<Descriptor_Ranges *> parameters_descriptor_ranges;
	
	Root_Paramter_Index parameter_index_table[HLSL_REGISTRE_COUNT][HLSL_SPACE_COUNT];

	void create(Gpu_Device &device, u32 access_flags = 0);

	void store_parameter_index(u32 parameter_index, u32 shader_register, u32 shader_space, Root_Parameter_Type parameter_type);
	u32 get_parameter_index(u32 shader_register, u32 shader_space, Root_Parameter_Type parameter_type);

	u32 add_32bit_constants_parameter(u32 shader_register, u32 shader_space, u32 struct_size, Shader_Visibility shader_visibility = VISIBLE_TO_ALL);

	u32 add_cb_descriptor_table_parameter(u32 shader_register, u32 shader_space, Shader_Visibility shader_visibility = VISIBLE_TO_ALL);
	
	u32 add_sr_descriptor_table_parameter(u32 shader_register, u32 shader_space, Shader_Visibility shader_visibility = VISIBLE_TO_ALL);
	u32 add_sr_descriptor_table_parameter_xxx(u32 shader_register, u32 shader_space, Shader_Visibility shader_visibility = VISIBLE_TO_ALL);
	
	u32 add_ua_descriptor_table_parameter(u32 shader_register, u32 shader_space, Shader_Visibility shader_visibility = VISIBLE_TO_ALL);
	u32 add_ua_descriptor_table_parameter(u32 shader_register, u32 shader_space, u32 number_descriptors, Shader_Visibility shader_visibility = VISIBLE_TO_ALL);
	u32 add_sampler_descriptor_table_parameter(u32 shader_register, u32 shader_space, Shader_Visibility shader_visibility = VISIBLE_TO_ALL);

	void begin_descriptor_table_parameter(u32 shader_register_space, Shader_Visibility shader_visibility);
	void end_parameter();

	void add_descriptor_range(u32 shader_register, CB_Descriptor &descriptor);
	void add_descriptor_range(u32 shader_register, SR_Descriptor &descriptor);
	void add_descriptor_range(u32 shader_register, UA_Descriptor &descriptor);
	void add_descriptor_range(u32 shader_register, Sampler_Descriptor &descriptor);
};

#endif
