#include "GraphicsShaderSet.h"

#include "DriverD3D12.h"

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

#include <fstream>
#include <system_error>
#include <functional>
#include <numeric>

namespace drv_d3d12
{

GraphicsShaderSet::GraphicsShaderSet(const ShaderSetDesc& desc_) : desc(desc_)
{
	compileAll();
	id = DriverD3D12::get().registerShaderSet(this);
}

GraphicsShaderSet::~GraphicsShaderSet()
{
	releaseAll();
	DriverD3D12::get().unregisterShaderSet(id);
}

bool GraphicsShaderSet::recompile()
{
	releaseAll();
	return compileAll();
}

unsigned int GraphicsShaderSet::getVariantIndexForKeywords(const char** keywords, unsigned int num_keywords)
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

void GraphicsShaderSet::setToPipelineState(GraphicsPipelineStateStream& pipeline_state, const unsigned int variant_index) const
{
	const ShaderSetVariant& variant = variants[variant_index];
	if (!variant.compiledSuccessfully)
	{
		DriverD3D12::get().getErrorShaderSet()->setToPipelineState(pipeline_state, 0);
		return;
	}

	auto getShaderByteCodeFromBlob = [](ID3DBlob* shaderBlob)
	{
		return shaderBlob != nullptr ? CD3DX12_SHADER_BYTECODE(shaderBlob) : CD3DX12_SHADER_BYTECODE(nullptr, 0);
	};

	pipeline_state.vs = getShaderByteCodeFromBlob(variant.shaderBlobs[(int)ShaderStage::VS]);
	pipeline_state.ps = getShaderByteCodeFromBlob(variant.shaderBlobs[(int)ShaderStage::PS]);
	pipeline_state.gs = getShaderByteCodeFromBlob(variant.shaderBlobs[(int)ShaderStage::GS]);
	pipeline_state.hs = getShaderByteCodeFromBlob(variant.shaderBlobs[(int)ShaderStage::HS]);
	pipeline_state.ds = getShaderByteCodeFromBlob(variant.shaderBlobs[(int)ShaderStage::DS]);
}

void GraphicsShaderSet::releaseAll()
{
	for (ShaderSetVariant& v : variants)
		for (ID3DBlob* blob : v.shaderBlobs)
			safe_release(blob);
	variants.clear();
	supportedKeywords.clear();
}

bool GraphicsShaderSet::compileAll()
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

void GraphicsShaderSet::parseKeywordCombinations(std::vector<std::vector<std::string>>& keyword_combinations)
{
	// NOTE: This function can definitely be done way faster if needed

	supportedKeywords.clear();

	std::vector<std::string> emptyCombination;
	keyword_combinations.push_back(emptyCombination);

	std::ifstream shaderFileStream(desc.sourceFilePath);

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

static ID3DBlob* compile_shader(const char* file_name, const char* entry_point, const char* target_string, const std::vector<std::string>& keywords)
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
	safe_release(errorBlob);

	return blob;
}

bool GraphicsShaderSet::compileAndAddVariant(const std::vector<std::string>& keywords)
{
	ShaderSetVariant variant;
	variant.keywords = keywords;
	variant.compiledSuccessfully = true;

	static const char* shader_model_target_strings[] = { "vs_5_1", "ps_5_1", "gs_5_1", "hs_5_1", "ds_5_1" };

	for (int stage = 0; stage < (int)ShaderStage::GRAPHICS_STAGE_COUNT; stage++)
	{
		if (desc.shaderFuncNames[stage].empty())
			continue;

		variant.shaderBlobs[stage] = compile_shader(desc.sourceFilePath.c_str(), desc.shaderFuncNames[stage].c_str(), shader_model_target_strings[stage], keywords);
		if (variant.shaderBlobs[stage] == nullptr)
		{
			variant.compiledSuccessfully = false;
			break;
		}
	}

	variants.push_back(variant);

	return variant.compiledSuccessfully;
}

}