#include "RenderState.h"

#include <assert.h>

using namespace drv_d3d11;

RenderState::RenderState(const RenderStateDesc& desc_) : desc(desc_)
{
	HRESULT hr;

	D3D11_RASTERIZER_DESC rsd{};
	rsd.FillMode = desc.rasterizerDesc.wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	rsd.CullMode = (D3D11_CULL_MODE)desc.rasterizerDesc.cullMode;
	rsd.DepthBias = desc.rasterizerDesc.depthBias;
	rsd.SlopeScaledDepthBias = desc.rasterizerDesc.slopeScaledDepthBias;
	rsd.ScissorEnable = desc.rasterizerDesc.scissorEnable;
	hr = Driver::get().getDevice().CreateRasterizerState(&rsd, &rasterizerState);
	assert(SUCCEEDED(hr));

	D3D11_DEPTH_STENCIL_DESC dsd{};
	dsd.DepthEnable = desc.depthStencilDesc.depthTest || desc.depthStencilDesc.depthWrite;
	dsd.DepthWriteMask = desc.depthStencilDesc.depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = (D3D11_COMPARISON_FUNC)desc.depthStencilDesc.DepthFunc;
	hr = Driver::get().getDevice().CreateDepthStencilState(&dsd, &depthStencilState);
	assert(SUCCEEDED(hr));

	id = Driver::get().registerRenderState(this);
}

RenderState::~RenderState()
{
	Driver::get().unregisterRenderState(id);
}