#pragma once

#include <Driver/DriverCommon.h>
#include "Driver.h"

namespace drv_d3d11
{
	class RenderState
	{
	public:
		RenderState(const RenderStateDesc& desc_);
		~RenderState();
		const ResId& getId() const { return id; }
		ID3D11RasterizerState* getRasterizerState() const { return rasterizerState.Get(); };
		ID3D11DepthStencilState* getDepthStencilState() const { return depthStencilState.Get(); };

	private:
		RenderStateDesc desc;
		ResId id = BAD_RESID;
		ComPtr<ID3D11RasterizerState> rasterizerState;
		ComPtr<ID3D11DepthStencilState> depthStencilState;
	};
}