#pragma once

#include <Driver/DriverCommon.h>

#define NOMINMAX
#include <d3d12.h>

#include <wrl.h>
template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
		throw std::exception();
}