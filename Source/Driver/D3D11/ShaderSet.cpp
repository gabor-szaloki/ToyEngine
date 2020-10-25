#include "ShaderSet.h"

#include <Common.h>

#include <assert.h>
#include <functional>

using namespace drv_d3d11;

ShaderSet::ShaderSet(const ShaderSetDesc& desc_) : desc(desc_)
{
	compileAll();
	id = Driver::get().registerShaderSet(this);
}

ShaderSet::~ShaderSet()
{
	releaseAll();
	Driver::get().unregisterShaderSet(id);
}

void ShaderSet::recompile()
{
	releaseAll();
	compileAll();
}

void ShaderSet::set()
{
	Driver::get().getContext().VSSetShader(vs, nullptr, 0);
	Driver::get().getContext().PSSetShader(ps, nullptr, 0);
	Driver::get().getContext().GSSetShader(gs, nullptr, 0);
	Driver::get().getContext().HSSetShader(hs, nullptr, 0);
	Driver::get().getContext().DSSetShader(ds, nullptr, 0);
}

template<typename T>
static T* compile_and_create_shader(
	const char* file_name, const char* entry_point, const char* target_string, ID3DBlob** out_blob,
	std::function<HRESULT(const void* byte_code, SIZE_T byte_code_length, T** out_shader)> create_func)
{
	ID3DBlob* blob;
	ID3DBlob* errorBlob;

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
	{
		PLOG_ERROR << "Failed to comiple shader." << std::endl
			<< "\tFile: " << file_name << std::endl
			<< "\tFunction: " << entry_point << std::endl
			<< "\tError: " << reinterpret_cast<const char*>(errorBlob->GetBufferPointer());
		SAFE_RELEASE(errorBlob);
	}
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

void ShaderSet::releaseAll()
{
	SAFE_RELEASE(vsBlob);
	SAFE_RELEASE(vs);
	SAFE_RELEASE(ps);
	SAFE_RELEASE(gs);
	SAFE_RELEASE(hs);
	SAFE_RELEASE(ds);
}

void ShaderSet::compileAll()
{
	if (desc.shaderFuncNames[(int)ShaderStage::VS] != nullptr)
	{
		vs = compile_and_create_shader<ID3D11VertexShader>(
			desc.sourceFilePath, desc.shaderFuncNames[(int)ShaderStage::VS], "vs_5_0", &vsBlob,
			[&](const void* byte_code, SIZE_T byte_code_length, ID3D11VertexShader** out_shader)
			{
				return Driver::get().getDevice().CreateVertexShader(
					byte_code, byte_code_length, nullptr, out_shader);
			});
	}
	if (desc.shaderFuncNames[(int)ShaderStage::PS] != nullptr)
	{
		ps = compile_and_create_shader<ID3D11PixelShader>(
			desc.sourceFilePath, desc.shaderFuncNames[(int)ShaderStage::PS], "ps_5_0", nullptr,
			[&](const void* byte_code, SIZE_T byte_code_length, ID3D11PixelShader** out_shader)
			{
				return Driver::get().getDevice().CreatePixelShader(
					byte_code, byte_code_length, nullptr, out_shader);
			});
	}
	if (desc.shaderFuncNames[(int)ShaderStage::GS] != nullptr)
	{
		gs = compile_and_create_shader<ID3D11GeometryShader>(
			desc.sourceFilePath, desc.shaderFuncNames[(int)ShaderStage::GS], "gs_5_0", nullptr,
			[&](const void* byte_code, SIZE_T byte_code_length, ID3D11GeometryShader** out_shader)
			{
				return Driver::get().getDevice().CreateGeometryShader(
					byte_code, byte_code_length, nullptr, out_shader);
			});
	}
	if (desc.shaderFuncNames[(int)ShaderStage::HS] != nullptr)
	{
		hs = compile_and_create_shader<ID3D11HullShader>(
			desc.sourceFilePath, desc.shaderFuncNames[(int)ShaderStage::HS], "hs_5_0", nullptr,
			[&](const void* byte_code, SIZE_T byte_code_length, ID3D11HullShader** out_shader)
			{
				return Driver::get().getDevice().CreateHullShader(
					byte_code, byte_code_length, nullptr, out_shader);
			});
	}
	if (desc.shaderFuncNames[(int)ShaderStage::DS] != nullptr)
	{
		ds = compile_and_create_shader<ID3D11DomainShader>(
			desc.sourceFilePath, desc.shaderFuncNames[(int)ShaderStage::DS], "ds_5_0", nullptr,
			[&](const void* byte_code, SIZE_T byte_code_length, ID3D11DomainShader** out_shader)
			{
				return Driver::get().getDevice().CreateDomainShader(
					byte_code, byte_code_length, nullptr, out_shader);
			});
	}
}
