#ifndef SYS_LOCAL_H
#define SYS_LOCAL_H

#include "../libs/str.h"
#include "../win32/win_local.h"
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

template <typename... Args>
void print(Args... args)
{
	append_text_to_text_buffer(format(args...));
}
#endif