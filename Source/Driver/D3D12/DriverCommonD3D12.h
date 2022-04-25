#pragma once

#include <Driver/DriverCommon.h>

#define NOMINMAX
#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")
#include <3rdParty/DirectX-Headers/d3dx12.h>

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

inline void set_debug_name(ID3D12DeviceChild* child, const std::string& name)
{
	if (child != nullptr)
		child->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.size(), name.c_str());
}