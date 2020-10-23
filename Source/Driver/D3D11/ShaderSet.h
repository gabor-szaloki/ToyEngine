#pragma once

#include <Driver/DriverCommon.h>
#include "Driver.h"

namespace drv_d3d11
{
	class ShaderSet
	{
	public:
		ShaderSet(const ShaderSetDesc& desc_);
		~ShaderSet();
		const ResId& getId() const { return id; }
		ID3DBlob* getVsBlob() const { return vsBlob; }
		void set();

	private:
		ShaderSetDesc desc;
		ResId id = BAD_RESID;
		ID3DBlob* vsBlob;
		ID3D11VertexShader* vs;
		ID3D11PixelShader* ps;
		ID3D11GeometryShader* gs;
		ID3D11HullShader* hs;
		ID3D11DomainShader* ds;
	};
}