#pragma once

#include <Driver/DriverCommon.h>

#define NOMINMAX
#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <wrl.h>
template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

template<typename T>
inline void safe_release(T*& resource)
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