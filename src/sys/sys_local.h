#ifndef SYS_LOCAL_H
#define SYS_LOCAL_H

#include "../libs/str.h"
#include "../win32/win_local.h"
#include "../win32/win_types.h"


void report_error(const char *error_message);
void report_hresult_error(const char *file, u32 line, HRESULT hr, const char *expr);
char *get_str_error_message_from_hresult_description(HRESULT hr);


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


template <typename F>
struct Deffer {
	F f;
	Deffer(F f) : f(f) {};
	~Deffer() { f(); }
};

template <typename F>
Deffer<F> defer_func(F f)
{
	return Deffer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x) DEFER_2(x, __COUNTER__)
#define defer(code) auto DEFER_3(_defer_) = defer_func([&](){code;})

#define RELEASE_COM(x) { if(x){ x->Release(); x = 0; } }
#define DELETE_PTR(x) if (x) delete x, x = NULL;
#define DELETE_ARRAY(x) if (x) delete[] x, x = NULL;

template <typename... Args>
void print(Args... args)
{
	char *formatted_string = format(args...);
	append_text_to_text_buffer(formatted_string);
	DELETE_PTR(formatted_string);
}

template <typename... Args>
void error(Args... args)
{
	char *formatted_string = format(args...);
	report_error(formatted_string);
}
#endif