#include "ShaderSet.h"

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

#include <Common.h>

#include <fstream>
#include <system_error>
#include <assert.h>
#include <functional>
#include <numeric>

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

unsigned int ShaderSet::getVariantIndexForKeywords(const char** keywords, unsigned int num_keywords)
{
	unsigned int result = 0;
	std::vector<const char*> relevantKeywords;
	for (unsigned int i = 0; i < num_keywords; i++)
	{
		if (std::find(supportedKeywords.begin(), supportedKeywords.end(), keywords[i]) != supportedKeywords.end())
			relevantKeywords.push_back(keywords[i]);
	}
	std::vector<unsigned int> candidates(variants.size());
	std::iota(candidates.begin(), candidates.end(), 0);
	for (unsigned int i = 0; i < relevantKeywords.size(); i++)
	{
		candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
			[&](unsigned int c) { return std::find(variants[c].keywords.begin(), variants[c].keywords.end(), relevantKeywords[i]) == variants[c].keywords.end(); }),
			candidates.end());
		if (candidates.empty())
			break;
		else
			result = candidates[0];
	}
	return result;
}

void ShaderSet::setToContext(unsigned int variant_index) const
{
	const ShaderSetVariant& variant = variants[variant_index];
	if (!variant.compiledSuccessfully)
	{
		Driver::get().getErrorShaderSet()->setToContext(0);
		return;
	}

	CONTEXT_LOCK_GUARD
	ID3D11DeviceContext& ctx = Driver::get().getContext();
	if (isCompute_)
	{
		ctx.CSSetShader(variant.cs, nullptr, 0);
	}
	else
	{
		ctx.VSSetShader(variant.vs, nullptr, 0);
		ctx.PSSetShader(variant.ps, nullptr, 0);
		ctx.GSSetShader(variant.gs, nullptr, 0);
		ctx.HSSetShader(variant.hs, nullptr, 0);
		ctx.DSSetShader(variant.ds, nullptr, 0);
	}
}

void ShaderSet::releaseAll()
{
	for (ShaderSetVariant v : variants)
	{
		SAFE_RELEASE(v.vsBlob);
		SAFE_RELEASE(v.vs);
		SAFE_RELEASE(v.ps);
		SAFE_RELEASE(v.gs);
		SAFE_RELEASE(v.hs);
		SAFE_RELEASE(v.ds);
		SAFE_RELEASE(v.cs);
	}
	variants.clear();
	supportedKeywords.clear();
}

bool ShaderSet::compileAll()
{
	std::vector<std::vector<std::string>> keywordCombinations;
	parseKeywordCombinations(keywordCombinations);
	assert(!keywordCombinations.empty());

	bool allVariantsCompiledSuccessfully = true;
	for (const std::vector<std::string>& keywords : keywordCombinations)
	{
		bool variantCompiledSuccessfully = compileAndAddVariant(keywords);
		allVariantsCompiledSuccessfully &= variantCompiledSuccessfully;
	}

	return allVariantsCompiledSuccessfully;
}

void ShaderSet::parseKeywordCombinations(std::vector<std::vector<std::string>>& keyword_combinations)
{
	// NOTE: This function can definitely be done way faster if needed

	supportedKeywords.clear();

	std::vector<std::string> emptyCombination;
	keyword_combinations.push_back(emptyCombination);

	std::ifstream shaderFileStream(isCompute_ ? computeDesc.sourceFilePath : desc.sourceFilePath);

	const char* pragmaStr = "#pragma multi_compile ";
	const size_t pragmaStrLen = strlen(pragmaStr);

	std::string line;
	while (std::getline(shaderFileStream, line))
	{
		const size_t pragmaPos = line.find(pragmaStr);
		const size_t commentPos = line.find("//");
		if (pragmaPos != std::string::npos && commentPos > pragmaPos)
		{
			std::vector<std::string> possibleKeywords;

			size_t keywordPos = pragmaPos + pragmaStrLen;
			std::string keywordsSeparatedBySpaces;
			if (commentPos != std::string::npos)
				keywordsSeparatedBySpaces = line.substr(keywordPos, keywordPos + commentPos);
			else
				keywordsSeparatedBySpaces = line.substr(keywordPos);

			size_t pos = 0;
			std::string keyword;
			while ((pos = keywordsSeparatedBySpaces.find(' ')) != std::string::npos)
			{
				keyword = keywordsSeparatedBySpaces.substr(0, pos);
				possibleKeywords.push_back(keyword);
				keywordsSeparatedBySpaces.erase(0, pos + 1);
			}
			possibleKeywords.push_back(keywordsSeparatedBySpaces);

			std::vector<std::vector<std::string>> updatedCombinations;
			for (const std::string& possibleKeyword : possibleKeywords)
			{
				supportedKeywords.push_back(possibleKeyword);

				for (const std::vector<std::string>& existingCombination : keyword_combinations)
				{
					std::vector<std::string> newCombination(existingCombination);
					newCombination.push_back(possibleKeyword);
					updatedCombinations.push_back(newCombination);
				}
			}
			keyword_combinations.assign(updatedCombinations.begin(), updatedCombinations.end());
		}
	}
}

template<typename T>
static T* compile_and_create_shader(
	const char* file_name, const char* entry_point, const char* target_string, const std::vector<std::string>& keywords, ID3DBlob** out_blob,
	std::function<HRESULT(const void* byte_code, SIZE_T byte_code_length, T** out_shader)> create_func)
{
	ID3DBlob* blob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	wchar_t fileName[MAX_PATH];
	utf8_to_wcs(file_name, fileName, MAX_PATH);

	std::vector<D3D_SHADER_MACRO> defines;
	for (const std::string& kw : keywords)
		defines.push_back({ kw.c_str(), "1" });
	defines.push_back({ NULL, NULL });

	unsigned int flags1 = 0, flags2 = 0;
#if defined(TOY_DEBUG)
	flags1 |= D3DCOMPILE_DEBUG;
#endif

	HRESULT hr = D3DCompileFromFile(
		fileName, defines.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entry_point, target_string, flags1, flags2, &blob, &errorBlob);

	if (FAILED(hr) && errorBlob == nullptr)
	{
		std::string hrStr = std::system_category().message(hr);
		PLOG_ERROR << "Failed to compile shader '" << entry_point << "' at path: " << file_name << std::endl
			<< "HRESULT: 0x" << std::hex << hr << " " << hrStr;
		return nullptr;
	}
	else if (FAILED(hr) && errorBlob != nullptr)
	{
		std::string concatenatedKeywords;
		for (const std::string& kw : keywords)
			concatenatedKeywords.append(kw + " ");
		PLOG_ERROR << "Failed to compile shader function '" << entry_point << "' at path: " << file_name << std::endl
			<< "Keywords: " << concatenatedKeywords << std::endl
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

bool ShaderSet::compileAndAddVariant(const std::vector<std::string>& keywords)
{
	ShaderSetVariant variant;
	variant.keywords = keywords;
	variant.compiledSuccessfully = true;

	if (isCompute_)
	{
		variant.cs = compile_and_create_shader<ID3D11ComputeShader>(
			computeDesc.sourceFilePath.c_str(), computeDesc.shaderFuncName.c_str(), "cs_5_0", keywords, nullptr,
			[&](const void* byte_code, SIZE_T byte_code_length, ID3D11ComputeShader** out_shader)
			{
				return Driver::get().getDevice().CreateComputeShader(
					byte_code, byte_code_length, nullptr, out_shader);
			});
		if (variant.cs == nullptr)
			variant.compiledSuccessfully = false;
	}
	else
	{
		if (!desc.shaderFuncNames[(int)ShaderStage::VS].empty())
		{
			variant.vs = compile_and_create_shader<ID3D11VertexShader>(
				desc.sourceFilePath.c_str(), desc.shaderFuncNames[(int)ShaderStage::VS].c_str(), "vs_5_0", keywords, &variant.vsBlob,
				[&](const void* byte_code, SIZE_T byte_code_length, ID3D11VertexShader** out_shader)
				{
					return Driver::get().getDevice().CreateVertexShader(
						byte_code, byte_code_length, nullptr, out_shader);
				});
			if (variant.vs == nullptr)
				variant.compiledSuccessfully = false;
		}
		if (!desc.shaderFuncNames[(int)ShaderStage::PS].empty())
		{
			variant.ps = compile_and_create_shader<ID3D11PixelShader>(
				desc.sourceFilePath.c_str(), desc.shaderFuncNames[(int)ShaderStage::PS].c_str(), "ps_5_0", keywords, nullptr,
				[&](const void* byte_code, SIZE_T byte_code_length, ID3D11PixelShader** out_shader)
				{
					return Driver::get().getDevice().CreatePixelShader(
						byte_code, byte_code_length, nullptr, out_shader);
				});
			if (variant.ps == nullptr)
				variant.compiledSuccessfully = false;
		}
		if (!desc.shaderFuncNames[(int)ShaderStage::GS].empty())
		{
			variant.gs = compile_and_create_shader<ID3D11GeometryShader>(
				desc.sourceFilePath.c_str(), desc.shaderFuncNames[(int)ShaderStage::GS].c_str(), "gs_5_0", keywords, nullptr,
				[&](const void* byte_code, SIZE_T byte_code_length, ID3D11GeometryShader** out_shader)
				{
					return Driver::get().getDevice().CreateGeometryShader(
						byte_code, byte_code_length, nullptr, out_shader);
				});
			if (variant.gs == nullptr)
				variant.compiledSuccessfully = false;
		}
		if (!desc.shaderFuncNames[(int)ShaderStage::HS].empty())
		{
			variant.hs = compile_and_create_shader<ID3D11HullShader>(
				desc.sourceFilePath.c_str(), desc.shaderFuncNames[(int)ShaderStage::HS].c_str(), "hs_5_0", keywords, nullptr,
				[&](const void* byte_code, SIZE_T byte_code_length, ID3D11HullShader** out_shader)
				{
					return Driver::get().getDevice().CreateHullShader(
						byte_code, byte_code_length, nullptr, out_shader);
				});
			if (variant.hs == nullptr)
				variant.compiledSuccessfully = false;
		}
		if (!desc.shaderFuncNames[(int)ShaderStage::DS].empty())
		{
			variant.ds = compile_and_create_shader<ID3D11DomainShader>(
				desc.sourceFilePath.c_str(), desc.shaderFuncNames[(int)ShaderStage::DS].c_str(), "ds_5_0", keywords, nullptr,
				[&](const void* byte_code, SIZE_T byte_code_length, ID3D11DomainShader** out_shader)
				{
					return Driver::get().getDevice().CreateDomainShader(
						byte_code, byte_code_length, nullptr, out_shader);
				});
			if (variant.ds == nullptr)
				variant.compiledSuccessfully = false;
		}
	}
	variants.push_back(variant);
	return variant.compiledSuccessfully;
}