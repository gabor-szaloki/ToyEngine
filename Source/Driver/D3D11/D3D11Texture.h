#pragma once

#include <Driver/ITexture.h>
#include "D3D11Driver.h"

class D3D11Texture : public ITexture
{
public:
	D3D11Texture(const TextureDesc& desc_, ID3D11Texture2D* tex = nullptr);
	~D3D11Texture();

	const TextureDesc& getDesc() const override { return desc; }
	const ResId& getId() const override { return id; };
	void updateData(unsigned int dst_subresource, const IntBox* dst_box, const void* src_data) override;
	void generateMips() override;

	ID3D11Texture2D* getResource() const { return resource.Get(); }
	ID3D11SamplerState* getSampler() const { return sampler.Get(); }
	ID3D11ShaderResourceView* getSrv() const { return srv.Get(); }
	ID3D11UnorderedAccessView* getUav() const { return uav.Get(); }
	ID3D11RenderTargetView* getRtv() const { return rtv.Get(); }
	ID3D11DepthStencilView* getDsv() const { return dsv.Get(); }

private:
	void createViews();
	void createSampler();

	TextureDesc desc;
	ResId id = BAD_RESID;
	ComPtr<ID3D11Texture2D> resource;
	ComPtr<ID3D11SamplerState> sampler;
	ComPtr<ID3D11ShaderResourceView> srv;
	ComPtr<ID3D11UnorderedAccessView> uav;
	ComPtr<ID3D11RenderTargetView> rtv;
	ComPtr<ID3D11DepthStencilView> dsv;
};