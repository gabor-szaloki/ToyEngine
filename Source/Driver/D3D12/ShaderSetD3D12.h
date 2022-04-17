#pragma once

#include <vector>
#include <string>
#include "DriverD3D12.h"

namespace drv_d3d12
{
	class ShaderSet
	{
	public:
		ShaderSet(const ShaderSetDesc& desc_);
		ShaderSet(const ComputeShaderDesc& desc_);
		~ShaderSet();
		const ResId& getId() const { return id; }
		const std::string& getName() const { return isCompute() ? computeDesc.name : desc.name; }
		bool recompile();
		bool isCompute() const { return isCompute_; }
		unsigned int getVariantIndexForKeywords(const char** keywords, unsigned int num_keywords);
		void setToPipelineState(GraphicsPipelineStateStream& pipeline_state, unsigned int variant_index) const;

	private:
		void releaseAll();
		bool compileAll();
		void parseKeywordCombinations(std::vector<std::vector<std::string>>& keyword_combinations);
		bool compileAndAddVariant(const std::vector<std::string>& keywords);

		ShaderSetDesc desc;
		ComputeShaderDesc computeDesc;
		const bool isCompute_ = false;
		ResId id = BAD_RESID;

		struct ShaderSetVariant
		{
			std::vector<std::string> keywords;
			ID3DBlob* vsBlob = nullptr;
			ID3DBlob* psBlob = nullptr;
			ID3DBlob* gsBlob = nullptr;
			ID3DBlob* hsBlob = nullptr;
			ID3DBlob* dsBlob = nullptr;
			ID3DBlob* csBlob = nullptr;
			bool compiledSuccessfully = false;
		};
		std::vector<ShaderSetVariant> variants;
		std::vector<std::string> supportedKeywords;
	};
}