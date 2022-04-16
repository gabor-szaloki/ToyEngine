#pragma once

#include <Driver/DriverCommon.h>

#define NOMINMAX
#include <d3d12.h>

#include <wrl.h>
template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

inline void safe_release(IUnknown*& resource)
{
	if (resource != nullptr)
	{
		resource->Release();
		resource = nullptr;
	}
}

inline void verify(HRESULT hr)
{
	if (FAILED(hr))
	{
		PLOG_ERROR << "Failed HRESULT: 0x" << std::hex << hr << " " << std::system_category().message(hr);
		assert(false && "Failed verified HRESULT. See error log for more info");
	}
}