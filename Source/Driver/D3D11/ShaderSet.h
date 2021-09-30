#pragma once

#include <vector>
#include <string>
#include "Driver.h"

namespace drv_d3d11
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
		ID3DBlob* getVsBlob() const { return variants.empty() ? nullptr : variants[0].vsBlob; }
		bool isCompute() const { return isCompute_; }
		unsigned int getVariantIndexForKeywords(const char** keywords, unsigned int num_keywords);
		void setToContext(unsigned int variant_index) const;

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
			ID3D11VertexShader* vs = nullptr;
			ID3D11PixelShader* ps = nullptr;
			ID3D11GeometryShader* gs = nullptr;
			ID3D11HullShader* hs = nullptr;
			ID3D11DomainShader* ds = nullptr;
			ID3D11ComputeShader* cs = nullptr;
			bool compiledSuccessfully = false;
		};
		std::vector<ShaderSetVariant> variants;
	};
}