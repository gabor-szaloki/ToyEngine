#include "RenderStateD3D12.h"

#include "DriverD3D12.h"

namespace drv_d3d12
{

RenderState::RenderState(const RenderStateDesc& desc_) : desc(desc_)
{
	rasterizerState = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());
	rasterizerState.FillMode = desc.rasterizerDesc.wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
	rasterizerState.CullMode = (D3D12_CULL_MODE)desc.rasterizerDesc.cullMode;
	rasterizerState.DepthBias = desc.rasterizerDesc.depthBias;
	rasterizerState.SlopeScaledDepthBias = desc.rasterizerDesc.slopeScaledDepthBias;
	rasterizerState.DepthClipEnable = desc.rasterizerDesc.depthClipEnable;

	depthStencilState = CD3DX12_DEPTH_STENCIL_DESC1(CD3DX12_DEFAULT());
	depthStencilState.DepthEnable = desc.depthStencilDesc.depthTest || desc.depthStencilDesc.depthWrite;
	depthStencilState.DepthWriteMask = desc.depthStencilDesc.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilState.DepthFunc = (D3D12_COMPARISON_FUNC)desc.depthStencilDesc.DepthFunc;

	id = DriverD3D12::get().registerRenderState(this);
}

RenderState::~RenderState()
{
	DriverD3D12::get().unregisterRenderState(id);
}

}