#ifndef GENERAL_H
#define GENERAL_H

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#include "math/vector.h"
#include "../win32/win_types.h"


char *get_str_error_message_from_hresult_description(HRESULT hr);
void report_hresult_error(const char *file, u32 line, HRESULT hr, const char *expr);


#if defined(DEBUG) | defined(_DEBUG)
#ifndef HR
#define HR(x)                                                  \
	{                                                          \
		HRESULT hr = (x);                                      \
		if(FAILED(hr))                                         \
		{                                                      \
			report_hresult_error(__FILE__, (u32)__LINE__, hr, #x);         \
		}                                                      \
	}
#endif

#else
#ifndef HR
#define HR(x) (x)
#endif
#endif

#define RELEASE_COM(x) { if(x){ x->Release(); x = 0; } }
#define DELETE_PTR(x) if (x) delete x, x = NULL;
#define DELETE_ARRAY(x) if (x) delete[] x;


extern Vector4 White;
extern Vector4 Black;
extern Vector4 Red;
extern Vector4 Green;
extern Vector4 Blue;
extern Vector4 Yellow;
extern Vector4 Cyan;
extern Vector4 Magenta;
extern Vector4 Silver;
extern Vector4 LightSteelBlue;
#endif