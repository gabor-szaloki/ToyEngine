#include "Texture.h"

#include <assert.h>

using namespace drv_d3d11;

Texture::Texture(const TextureDesc& desc_, ID3D11Texture2D* tex) : desc(desc_), resource(tex)
{
	if (resource == nullptr)
	{
		D3D11_TEXTURE2D_DESC td{};
		td.Width = desc.width;
		td.Height = desc.height;
		td.MipLevels = desc.mips;
		td.ArraySize = 1;
		td.Format = (DXGI_FORMAT)desc.format;
		td.SampleDesc.Count = 1;
		td.SampleDesc.Quality = 0;
		td.Usage = (D3D11_USAGE)desc.usageFlags;
		td.BindFlags = desc.bindFlags;
		td.CPUAccessFlags = desc.cpuAccessFlags;
		td.MiscFlags = desc.miscFlags;
		HRESULT hr = Driver::get().getDevice().CreateTexture2D(&td, nullptr, &resource);
		assert(SUCCEEDED(hr));
	}

	createViews();
	createSampler();

	id = Driver::get().registerTexture(this);
}

Texture::~Texture()
{
	Driver::get().unregisterTexture(id);
}

void Texture::updateData(unsigned int dst_subresource, const IntBox* dst_box, const void* src_data)
{
	unsigned int rowPitch = desc.width * get_byte_size_for_texfmt(desc.format);

	if (dst_box != nullptr)
	{
		D3D11_BOX dstBox;
		dstBox.left   = dst_box->left;   assert(dst_box->left >= 0);
		dstBox.top    = dst_box->top;    assert(dst_box->top >= 0);
		dstBox.front  = dst_box->front;  assert(dst_box->front >= 0);
		dstBox.right  = dst_box->right;  assert(dst_box->right >= 0);
		dstBox.bottom = dst_box->bottom; assert(dst_box->bottom >= 0);
		dstBox.back   = dst_box->back;   assert(dst_box->back >= 0);
		Driver::get().getContext().UpdateSubresource(resource.Get(), dst_subresource, &dstBox, src_data, rowPitch, 0);
	}
	else
	{
		Driver::get().getContext().UpdateSubresource(resource.Get(), dst_subresource, nullptr, src_data, rowPitch, 0);
	}
}

void Texture::generateMips()
{
	assert(desc.miscFlags & RESOURCE_MISC_GENERATE_MIPS);
	assert(desc.bindFlags & BIND_SHADER_RESOURCE);
	assert(srv != nullptr);
	Driver::get().getContext().GenerateMips(srv.Get());
}

void Texture::createViews()
{
	HRESULT hr;

	if (desc.bindFlags & BIND_SHADER_RESOURCE)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = (DXGI_FORMAT)(desc.srvFormatOverride != TexFmt::INVALID ? desc.srvFormatOverride : desc.format);
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.mips == 0 ? -1 : desc.mips;
		srvDesc.Texture2D.MostDetailedMip = 0;

		hr = Driver::get().getDevice().CreateShaderResourceView(resource.Get(), &srvDesc, &srv);
		assert(SUCCEEDED(hr));
	}
	if (desc.bindFlags & BIND_UNORDERED_ACCESS)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.Format = (DXGI_FORMAT)(desc.uavFormatOverride != TexFmt::INVALID ? desc.uavFormatOverride : desc.format);
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		hr = Driver::get().getDevice().CreateUnorderedAccessView(resource.Get(), &uavDesc, &uav);
		assert(SUCCEEDED(hr));
	}
	if (desc.bindFlags & BIND_RENDER_TARGET)
	{
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = (DXGI_FORMAT)(desc.rtvFormatOverride != TexFmt::INVALID ? desc.rtvFormatOverride : desc.format);
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		hr = Driver::get().getDevice().CreateRenderTargetView(resource.Get(), &rtvDesc, &rtv);
		assert(SUCCEEDED(hr));
	}
	if (desc.bindFlags & BIND_DEPTH_STENCIL)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = (DXGI_FORMAT)(desc.dsvFormatOverride != TexFmt::INVALID ? desc.dsvFormatOverride : desc.format);
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
		hr = Driver::get().getDevice().CreateDepthStencilView(resource.Get(), &dsvDesc, &dsv);
		assert(SUCCEEDED(hr));
	}
}

void Texture::createSampler()
{
	if (!desc.hasSampler)
		return;

	const unsigned int driverAnisotropy = Driver::get().getSettings().textureFilteringAnisotropy;
	D3D11_FILTER defaultFilter = driverAnisotropy > 0 ? D3D11_FILTER_ANISOTROPIC : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	D3D11_SAMPLER_DESC sd{};
	sd.Filter = desc.samplerDesc.filter == FILTER_DEFAULT ? defaultFilter : (D3D11_FILTER)desc.samplerDesc.filter;
	sd.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)desc.samplerDesc.addressU;
	sd.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)desc.samplerDesc.addressV;
	sd.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)desc.samplerDesc.addressW;
	sd.MipLODBias = desc.samplerDesc.mipBias;
	sd.MaxAnisotropy = driverAnisotropy;
	sd.ComparisonFunc = (D3D11_COMPARISON_FUNC)desc.samplerDesc.comparisonFunc;
	sd.MinLOD = desc.samplerDesc.minLOD;
	sd.MaxLOD = desc.samplerDesc.maxLOD;
	for (int i = 0; i < 4; i++)
		sd.BorderColor[i] = desc.samplerDesc.borderColor[i];

	HRESULT hr = Driver::get().getDevice().CreateSamplerState(&sd, &sampler);
	assert(SUCCEEDED(hr));
}
