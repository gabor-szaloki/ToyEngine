#include "ShaderSet.h"

#include <assert.h>
#include <functional>

using namespace drv_d3d11;

static wchar_t* utf8_to_wcs(const char* utf8_str, wchar_t* wcs_buf, int wcs_buf_len) // TODO: move to some kind of utils
{
	int cnt = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wcs_buf, wcs_buf_len);
	if (!cnt)
		return nullptr;
	wcs_buf[cnt < wcs_buf_len ? cnt : wcs_buf_len - 1] = L'\0';
	return wcs_buf;
}

template<typename T>
static T* compile_and_create_shader(
	const char* file_name, const char* entry_point, const char* target_string, ID3DBlob** out_blob,
	std::function<HRESULT(const void* byte_code, SIZE_T byte_code_length, T** out_shader)> create_func)
{
	ID3DBlob* blob;
	ComPtr<ID3DBlob> errorBlob;

	wchar_t fileName[MAX_PATH];
	utf8_to_wcs(file_name, fileName, MAX_PATH);

	unsigned int flags1 = 0, flags2 = 0;
#if defined(_DEBUG)
	flags1 |= D3DCOMPILE_DEBUG;
#endif

	HRESULT hr = D3DCompileFromFile(
		fileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entry_point, target_string, flags1, flags2, &blob, &errorBlob);

	if (errorBlob)
		OutputDebugString(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
	if (FAILED(hr))
		return nullptr;

	T* shader;
	hr = create_func(blob->GetBufferPointer(), blob->GetBufferSize(), &shader);
	assert(SUCCEEDED(hr));

	if (out_blob != nullptr)
		*out_blob = blob;
	else if (blob != nullptr)
		blob->Release();

	return shader;
}

ShaderSet::ShaderSet(const ShaderSetDesc& desc_) : desc(desc_)
{
	if (desc.shaderFuncNames[(int)ShaderStage::VS] != nullptr)
	{
		vs.Attach(compile_and_create_shader<ID3D11VertexShader>(
			desc.sourceFilePath, desc.shaderFuncNames[(int)ShaderStage::VS], "vs_5_0", &vsBlob,
			[&](const void* byte_code, SIZE_T byte_code_length, ID3D11VertexShader** out_shader)
			{
				return Driver::get().getDevice().CreateVertexShader(
					byte_code, byte_code_length, nullptr, out_shader);
			}));
	}
	if (desc.shaderFuncNames[(int)ShaderStage::PS] != nullptr)
	{
		ps.Attach(compile_and_create_shader<ID3D11PixelShader>(
			desc.sourceFilePath, desc.shaderFuncNames[(int)ShaderStage::PS], "ps_5_0", nullptr,
			[&](const void* byte_code, SIZE_T byte_code_length, ID3D11PixelShader** out_shader)
			{
				return Driver::get().getDevice().CreatePixelShader(
					byte_code, byte_code_length, nullptr, out_shader);
			}));
	}
	if (desc.shaderFuncNames[(int)ShaderStage::GS] != nullptr)
	{
		gs.Attach(compile_and_create_shader<ID3D11GeometryShader>(
			desc.sourceFilePath, desc.shaderFuncNames[(int)ShaderStage::GS], "gs_5_0", nullptr,
			[&](const void* byte_code, SIZE_T byte_code_length, ID3D11GeometryShader** out_shader)
			{
				return Driver::get().getDevice().CreateGeometryShader(
					byte_code, byte_code_length, nullptr, out_shader);
			}));
	}
	if (desc.shaderFuncNames[(int)ShaderStage::HS] != nullptr)
	{
		hs.Attach(compile_and_create_shader<ID3D11HullShader>(
			desc.sourceFilePath, desc.shaderFuncNames[(int)ShaderStage::HS], "hs_5_0", nullptr,
			[&](const void* byte_code, SIZE_T byte_code_length, ID3D11HullShader** out_shader)
			{
				return Driver::get().getDevice().CreateHullShader(
					byte_code, byte_code_length, nullptr, out_shader);
			}));
	}
	if (desc.shaderFuncNames[(int)ShaderStage::DS] != nullptr)
	{
		ds.Attach(compile_and_create_shader<ID3D11DomainShader>(
			desc.sourceFilePath, desc.shaderFuncNames[(int)ShaderStage::DS], "ds_5_0", nullptr,
			[&](const void* byte_code, SIZE_T byte_code_length, ID3D11DomainShader** out_shader)
			{
				return Driver::get().getDevice().CreateDomainShader(
					byte_code, byte_code_length, nullptr, out_shader);
			}));
	}

	id = Driver::get().registerShaderSet(this);
}

ShaderSet::~ShaderSet()
{
	Driver::get().unregisterShaderSet(id);
}

void ShaderSet::set()
{
	Driver::get().getContext().VSSetShader(vs.Get(), nullptr, 0);
	Driver::get().getContext().PSSetShader(ps.Get(), nullptr, 0);
	Driver::get().getContext().GSSetShader(gs.Get(), nullptr, 0);
	Driver::get().getContext().HSSetShader(hs.Get(), nullptr, 0);
	Driver::get().getContext().DSSetShader(ds.Get(), nullptr, 0);
}
