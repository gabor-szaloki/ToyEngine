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
		ID3DBlob* getVsBlob() const { return vsBlob.Get(); }
		void set();

	private:
		ShaderSetDesc desc;
		ResId id = BAD_RESID;
		ComPtr<ID3DBlob> vsBlob;
		ComPtr<ID3D11VertexShader> vs;
		ComPtr<ID3D11PixelShader> ps;
		ComPtr<ID3D11GeometryShader> gs;
		ComPtr<ID3D11HullShader> hs;
		ComPtr<ID3D11DomainShader> ds;
	};
}