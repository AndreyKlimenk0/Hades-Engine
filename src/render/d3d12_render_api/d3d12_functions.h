#ifndef D3D12_FUNCTIONS_H
#define D3D12_FUNCTIONS_H

#include <d3d12.h>
#include <dxgiformat.h>
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
#endif
