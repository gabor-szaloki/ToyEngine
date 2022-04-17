#pragma once

#include "DriverCommonD3D12.h"
#include <vector>
#include <string>

namespace drv_d3d12
{
	struct GraphicsPipelineStateStream;

	class GraphicsShaderSet
	{
	public:
		GraphicsShaderSet(const ShaderSetDesc& desc_);
		~GraphicsShaderSet();
		const ResId& getId() const { return id; }
		const std::string& getName() const { return desc.name; }
		bool recompile();
		unsigned int getVariantIndexForKeywords(const char** keywords, unsigned int num_keywords);
		void setToPipelineState(GraphicsPipelineStateStream& pipeline_state, unsigned int variant_index) const;

	private:
		void releaseAll();
		bool compileAll();
		void parseKeywordCombinations(std::vector<std::vector<std::string>>& keyword_combinations);
		bool compileAndAddVariant(const std::vector<std::string>& keywords);

		ShaderSetDesc desc;
		ResId id = BAD_RESID;

		struct ShaderSetVariant
		{
			std::vector<std::string> keywords;
			ID3DBlob* shaderBlobs[(int)ShaderStage::GRAPHICS_STAGE_COUNT]{};
			bool compiledSuccessfully = false;
		};
		std::vector<ShaderSetVariant> variants;
		std::vector<std::string> supportedKeywords;
	};
}