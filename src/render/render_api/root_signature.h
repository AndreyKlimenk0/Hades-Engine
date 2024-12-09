#ifndef RENDER_API_ROOT_SIGNATURE_H
#define RENDER_API_ROOT_SIGNATURE_H

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

struct Root_Signature : D3D12_Object<ID3D12RootSignature> {
	Root_Signature();
	~Root_Signature();

	u32 register_space;
	Shader_Visibility visibility;
	Descriptor_Ranges *descriptor_ranges = NULL;

	Array<D3D12_ROOT_PARAMETER1> parameters;
	Array<Descriptor_Ranges *> parameters_descriptor_ranges;

	void create(Gpu_Device &device, u32 access_flags);
	//void cb_descriptor_paramater();
	//void sr_descriptor_paramater();
	//void ua_descriptor_paramater();
	void begin_descriptor_table_parameter(u32 shader_register_space, Shader_Visibility shader_visibility);
	void end_parameter();

	void add_descriptor_range(u32 shader_register, CB_Descriptor &descriptor);
	void add_descriptor_range(u32 shader_register, SR_Descriptor &descriptor);
	void add_descriptor_range(u32 shader_register, UA_Descriptor &descriptor);
	void add_descriptor_range(u32 shader_register, Sampler_Descriptor &descriptor);
};

#endif
