#pragma once

#include "DriverCommonD3D12.h"

#include <3rdParty/DirectX-Headers/d3dx12.h>

namespace drv_d3d12
{
	class RenderState
	{
	public:
		RenderState(const RenderStateDesc& desc_);
		~RenderState();
		const ResId& getId() const { return id; }
		const CD3DX12_RASTERIZER_DESC& getRasterizerState() const { return rasterizerState; };
		const CD3DX12_DEPTH_STENCIL_DESC1& getDepthStencilState() const { return depthStencilState; };

	private:
		RenderStateDesc desc;
		ResId id = BAD_RESID;
		CD3DX12_RASTERIZER_DESC rasterizerState;
		CD3DX12_DEPTH_STENCIL_DESC1 depthStencilState;
	};
}