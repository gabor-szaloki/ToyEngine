#include "ShaderSet.h"

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

#include <Common.h>

#include <system_error>
#include <assert.h>
#include <functional>

using namespace drv_d3d11;

ShaderSet::ShaderSet(const ShaderSetDesc& desc_) : desc(desc_), isCompute_(false)
{
	compileAll();
	id = Driver::get().registerShaderSet(this);
}

ShaderSet::ShaderSet(const ComputeShaderDesc& desc_) : computeDesc(desc_), isCompute_(true)
{
	compileAll();
	id = Driver::get().registerShaderSet(this);
}

ShaderSet::~ShaderSet()
{
	releaseAll();
	Driver::get().unregisterShaderSet(id);
}

bool ShaderSet::recompile()
{
	releaseAll();
	return compileAll();
}

void ShaderSet::set()
{
	CONTEXT_LOCK_GUARD
	ID3D11DeviceContext& ctx = Driver::get().getContext();
	if (isCompute_)
	{
		ctx.CSSetShader(cs, nullptr, 0);
	}
	else
	{
		ctx.VSSetShader(vs, nullptr, 0);
		ctx.PSSetShader(ps, nullptr, 0);
		ctx.GSSetShader(gs, nullptr, 0);
		ctx.HSSetShader(hs, nullptr, 0);
		ctx.DSSetShader(ds, nullptr, 0);
	}
}

template<typename T>
static T* compile_and_create_shader(
	const char* file_name, const char* entry_point, const char* target_string, ID3DBlob** out_blob,
	std::function<HRESULT(const void* byte_code, SIZE_T byte_code_length, T** out_shader)> create_func)
{
	ID3DBlob* blob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	wchar_t fileName[MAX_PATH];
	utf8_to_wcs(file_name, fileName, MAX_PATH);

	unsigned int flags1 = 0, flags2 = 0;
#if defined(_DEBUG)
	flags1 |= D3DCOMPILE_DEBUG;
#endif

	HRESULT hr = D3DCompileFromFile(
		fileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entry_point, target_string, flags1, flags2, &blob, &errorBlob);

	if (FAILED(hr) && errorBlob == nullptr)
	{
		std::string hrStr = std::system_category().message(hr);
		PLOG_ERROR << "Failed to compile shader '" << entry_point << "' at path: " << file_name << std::endl
			<< "HRESULT: 0x"<< std::hex << hr << " " << hrStr;
		return nullptr;
	}
	else if (FAILED(hr) && errorBlob != nullptr)
	{
		PLOG_ERROR << "Failed to compile shader '" << entry_point << "' at path: " << file_name << std::endl
			<< reinterpret_cast<const char*>(errorBlob->GetBufferPointer());
		return nullptr;
	}
	else if (errorBlob != nullptr)
	{
		PLOG_WARNING << "Shader " << entry_point << " compiled with warnings" << std::endl
			<< reinterpret_cast<const char*>(errorBlob->GetBufferPointer());
	}
	SAFE_RELEASE(errorBlob);

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
	SAFE_RELEASE(cs);
}

bool ShaderSet::compileAll()
{
	compiledSuccessfully = true;
	if (isCompute_)
	{
		cs = compile_and_create_shader<ID3D11ComputeShader>(
			computeDesc.sourceFilePath.c_str(), computeDesc.shaderFuncName.c_str(), "cs_5_0", nullptr,
			[&](const void* byte_code, SIZE_T byte_code_length, ID3D11ComputeShader** out_shader)
			{
				return Driver::get().getDevice().CreateComputeShader(
					byte_code, byte_code_length, nullptr, out_shader);
			});
		if (cs == nullptr)
			compiledSuccessfully = false;
	}
	else
	{
		if (!desc.shaderFuncNames[(int)ShaderStage::VS].empty())
		{
			vs = compile_and_create_shader<ID3D11VertexShader>(
				desc.sourceFilePath.c_str(), desc.shaderFuncNames[(int)ShaderStage::VS].c_str(), "vs_5_0", &vsBlob,
				[&](const void* byte_code, SIZE_T byte_code_length, ID3D11VertexShader** out_shader)
				{
					return Driver::get().getDevice().CreateVertexShader(
						byte_code, byte_code_length, nullptr, out_shader);
				});
			if (vs == nullptr)
				compiledSuccessfully = false;
		}
		if (!desc.shaderFuncNames[(int)ShaderStage::PS].empty())
		{
			ps = compile_and_create_shader<ID3D11PixelShader>(
				desc.sourceFilePath.c_str(), desc.shaderFuncNames[(int)ShaderStage::PS].c_str(), "ps_5_0", nullptr,
				[&](const void* byte_code, SIZE_T byte_code_length, ID3D11PixelShader** out_shader)
				{
					return Driver::get().getDevice().CreatePixelShader(
						byte_code, byte_code_length, nullptr, out_shader);
				});
			if (ps == nullptr)
				compiledSuccessfully = false;
		}
		if (!desc.shaderFuncNames[(int)ShaderStage::GS].empty())
		{
			gs = compile_and_create_shader<ID3D11GeometryShader>(
				desc.sourceFilePath.c_str(), desc.shaderFuncNames[(int)ShaderStage::GS].c_str(), "gs_5_0", nullptr,
				[&](const void* byte_code, SIZE_T byte_code_length, ID3D11GeometryShader** out_shader)
				{
					return Driver::get().getDevice().CreateGeometryShader(
						byte_code, byte_code_length, nullptr, out_shader);
				});
			if (gs == nullptr)
				compiledSuccessfully = false;
		}
		if (!desc.shaderFuncNames[(int)ShaderStage::HS].empty())
		{
			hs = compile_and_create_shader<ID3D11HullShader>(
				desc.sourceFilePath.c_str(), desc.shaderFuncNames[(int)ShaderStage::HS].c_str(), "hs_5_0", nullptr,
				[&](const void* byte_code, SIZE_T byte_code_length, ID3D11HullShader** out_shader)
				{
					return Driver::get().getDevice().CreateHullShader(
						byte_code, byte_code_length, nullptr, out_shader);
				});
			if (hs == nullptr)
				compiledSuccessfully = false;
		}
		if (!desc.shaderFuncNames[(int)ShaderStage::DS].empty())
		{
			ds = compile_and_create_shader<ID3D11DomainShader>(
				desc.sourceFilePath.c_str(), desc.shaderFuncNames[(int)ShaderStage::DS].c_str(), "ds_5_0", nullptr,
				[&](const void* byte_code, SIZE_T byte_code_length, ID3D11DomainShader** out_shader)
				{
					return Driver::get().getDevice().CreateDomainShader(
						byte_code, byte_code_length, nullptr, out_shader);
				});
			if (ds == nullptr)
				compiledSuccessfully = false;
		}
	}
	return compiledSuccessfully;
}
