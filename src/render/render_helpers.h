#ifndef RENDER_HELPERS_H
#define RENDER_HELPERS_H

#include <dxgi1_4.h>
#include "../libs/str.h"
#include "../libs/number_types.h"
#include "render_api\command.h"
#include "render_api\descriptor_heap.h"

struct R24U8 {
	R24U8(u32 r24u8_value);
	R24U8(u32 numerator, u8 typeless_bits);

	u32 numerator;
	u8 typeless_bits;

	u32 get_packed_value();
	float get_unorm_value();
};

u32 dxgi_format_size(DXGI_FORMAT format);

struct Compute_Command_List_Helper {
	Compute_Command_List_Helper(Compute_Command_List *command_list, Root_Signature *root_signature);
	~Compute_Command_List_Helper();

	Compute_Command_List *command_list = NULL;	Root_Signature *root_signature = NULL;
	void set_root_descriptor_table(u32 shader_register, u32 shader_space, CB_Descriptor *base_decriptor);
	void set_root_descriptor_table(u32 shader_register, u32 shader_space, SR_Descriptor *base_decriptor);
	void set_root_descriptor_table(u32 shader_register, u32 shader_space, Sampler_Descriptor *base_decriptor);
};

struct Graphics_Command_List_Helper {
	Graphics_Command_List_Helper(Graphics_Command_List *command_list, Root_Signature *root_signature);
	~Graphics_Command_List_Helper();

	Graphics_Command_List *command_list = NULL;	Root_Signature *root_signature = NULL;
	void set_root_descriptor_table(u32 shader_register, u32 shader_space, CB_Descriptor *base_decriptor);
	void set_root_descriptor_table(u32 shader_register, u32 shader_space, SR_Descriptor *base_decriptor);
	void set_root_descriptor_table(u32 shader_register, u32 shader_space, Sampler_Descriptor *base_decriptor);
};
#endif

