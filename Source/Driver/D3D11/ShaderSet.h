#pragma once

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
		bool recompile();
		ID3DBlob* getVsBlob() const { return vsBlob; }
		bool isCompiledSuccessfully() { return compiledSuccessfully; }
		bool isCompute() { return isCompute_; }
		void set();

	private:
		void releaseAll();
		bool compileAll();

		ShaderSetDesc desc;
		ComputeShaderDesc computeDesc;
		bool isCompute_ = false;
		ResId id = BAD_RESID;
		ID3DBlob* vsBlob = nullptr;
		ID3D11VertexShader* vs = nullptr;
		ID3D11PixelShader* ps = nullptr;
		ID3D11GeometryShader* gs = nullptr;
		ID3D11HullShader* hs = nullptr;
		ID3D11DomainShader* ds = nullptr;
		ID3D11ComputeShader* cs = nullptr;
		bool compiledSuccessfully = false;
	};
}