#ifndef D3D12_FUNCTIONS_H
#define D3D12_FUNCTIONS_H

#include <d3d12.h>
#include <dxgiformat.h>
#include "d3d12_device.h"
#include "../../libs/str.h"
#include "../../libs/number_types.h"

u32 dxgi_format_size(DXGI_FORMAT format);
bool check_tearing_support();

template <typename... Args>
inline void set_name(ID3D12Object *object, Args... args)
{
	char *formatted_string = format(args...);
	wchar_t *wstring = to_wstring(formatted_string);

	object->SetName(wstring);

	free_string(formatted_string);
	free_string(wstring);
}

template <typename T>
inline T check_feature_support(D3D12_Render_Device *render_device, D3D12_FEATURE feature)
{
	T data;
	ZeroMemory(&data, sizeof(T));

	HR(render_device->device->CheckFeatureSupport(feature, (void *)&data, sizeof(T)));
	return data;
}

template <>
inline D3D12_FEATURE_DATA_SHADER_MODEL check_feature_support<D3D12_FEATURE_DATA_SHADER_MODEL>(D3D12_Render_Device *render_device, D3D12_FEATURE feature)
{
	D3D12_FEATURE_DATA_SHADER_MODEL data;
	data.HighestShaderModel = D3D_HIGHEST_SHADER_MODEL;

	HR(render_device->device->CheckFeatureSupport(feature, (void *)&data, sizeof(D3D12_FEATURE_DATA_SHADER_MODEL)));
	return data;
}
#endif
