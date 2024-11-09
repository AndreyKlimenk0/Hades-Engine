#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <windows.h>

#include "../libs/number_types.h"

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

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x) DEFER_2(x, __COUNTER__)
#define defer(code) auto DEFER_3(_defer_) = defer_func([&](){code;})

#define RELEASE_COM(x) { if(x){ x->Release(); x = 0; } }
#define DELETE_PTR(x) {if (x) delete x, x = NULL;}
#define DELETE_ARRAY(x) {if (x) delete[] x, x = NULL;}

#define DELETE_COPING(class_name) \
	class_name(const class_name &other) = delete; \
	void operator=(const class_name &other) = delete; \

#define CALL_METHOD(object, method_ptr, ...) ((object.*method_ptr)(__VA_ARGS__))

struct Callback {
	~Callback() {}
	virtual void call(void *args) = 0;
};

template< typename T>
struct Member_Callback : Callback {
	Member_Callback(T *object, void (T:: *member)(void *args)) : object(object), member(member) {}
	~Member_Callback() {}

	T *object = NULL;
	void (T:: *member)(void *args) = NULL;

	void call(void *args)
	{
		(object->*member)(args);
	}
};

template <typename T>
inline Member_Callback<T> *make_member_callback(T *object, void (T:: *callback)(void *args))
{
	return new Member_Callback<T>(object, callback);
}

#endif