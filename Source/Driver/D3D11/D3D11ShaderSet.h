#pragma once

#include <Driver/DriverCommon.h>
#include "D3D11Driver.h"

class D3D11ShaderSet
{
public:
	D3D11ShaderSet(const ShaderSetDesc& desc_);
	~D3D11ShaderSet();
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