#pragma once

#include <vector>

#include <Driver/ITexture.h>
#include "Driver.h"

namespace drv_d3d11
{
	class Texture : public ITexture
	{
	public:
		Texture(const TextureDesc* desc_ = nullptr, ID3D11Texture2D* tex = nullptr);
		~Texture() override;

		const TextureDesc& getDesc() const override { return desc; }
		const ResId& getId() const override { return id; };
		void* getViewHandle() const override { return srv; };
		void updateData(unsigned int dst_subresource, const IntBox* dst_box, const void* src_data) override;
		void generateMips() override;
		bool isStub() override { return isStub_; }
		void setStub(bool is_stub) override { isStub_ = is_stub; }
		void recreate(const TextureDesc& desc_) override;

		ID3D11Texture2D* getResource() const { return resource; }
		ID3D11ShaderResourceView* getSrv() const { return srv; }
		ID3D11UnorderedAccessView* getUav() const { return uav; }
		ID3D11RenderTargetView* getRtv(unsigned int slice = 0, unsigned int mip = 0) const;
		ID3D11DepthStencilView* getDsv(unsigned int slice = 0, unsigned int mip = 0) const;

	private:
		void createViews();
		void releaseAll();

		TextureDesc desc;
		unsigned int numMips = 0;
		ResId id = BAD_RESID;
		ID3D11Texture2D* resource;
		ID3D11ShaderResourceView* srv;
		ID3D11UnorderedAccessView* uav;
		std::vector<ID3D11RenderTargetView*> rtvs;
		std::vector<ID3D11DepthStencilView*> dsvs;

		bool isStub_ = false;
	};
}