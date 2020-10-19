#pragma once

#include <Driver/DriverCommon.h>
#include "D3D11Driver.h"

class D3D11RenderState
{
public:
	D3D11RenderState(const RenderStateDesc& desc_);
	~D3D11RenderState();
	const ResId& getId() const { return id; }
	ID3D11RasterizerState* getRasterizerState() const { return rasterizerState.Get(); };
	ID3D11DepthStencilState* getDepthStencilState() const { return depthStencilState.Get(); };

private:
	RenderStateDesc desc;
	ResId id = BAD_RESID;
	ComPtr<ID3D11RasterizerState> rasterizerState;
	ComPtr<ID3D11DepthStencilState> depthStencilState;
};