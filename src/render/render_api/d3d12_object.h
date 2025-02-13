#ifndef RENDER_API_D3D12_OBJECT_H
#define RENDER_API_D3D12_OBJECT_H

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

template <typename T>
struct D3D12_Object {
	D3D12_Object();
	virtual ~D3D12_Object();

	ComPtr<T> d3d12_object;

	void set_name(const wchar_t *name);

	u32 release();
	T *get();
	T **get_address();
	T **release_and_get_address();
};

template<typename T>
inline D3D12_Object<T>::D3D12_Object()
{
}

template<typename T>
inline D3D12_Object<T>::~D3D12_Object()
{
}

template<typename T>
inline void D3D12_Object<T>::set_name(const wchar_t *name)
{
	d3d12_object->SetName(name);
}

template<typename T>
inline u32 D3D12_Object<T>::release()
{
	return d3d12_object.Reset();
}

template<typename T>
inline T *D3D12_Object<T>::get()
{
	return d3d12_object.Get();
}

template<typename T>
inline T **D3D12_Object<T>::get_address()
{
	return d3d12_object.GetAddressOf();
}

template<typename T>
inline T **D3D12_Object<T>::release_and_get_address()
{
	return d3d12_object.ReleaseAndGetAddressOf();
}

#endif
