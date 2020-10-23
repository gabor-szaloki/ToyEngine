#pragma once

#include "Driver.h"

namespace drv_d3d11
{
	class RenderState
	{
	public:
		RenderState(const RenderStateDesc& desc_);
		~RenderState();
		const ResId& getId() const { return id; }
		ID3D11RasterizerState* getRasterizerState() const { return rasterizerState; };
		ID3D11DepthStencilState* getDepthStencilState() const { return depthStencilState; };

	private:
		RenderStateDesc desc;
		ResId id = BAD_RESID;
		ID3D11RasterizerState* rasterizerState;
		ID3D11DepthStencilState* depthStencilState;
	};
}