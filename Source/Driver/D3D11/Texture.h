#pragma once

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
		ID3D11RenderTargetView* getRtv() const { return rtv; }
		ID3D11DepthStencilView* getDsv() const { return dsv; }

	private:
		void createViews();
		void releaseAll();

		TextureDesc desc;
		ResId id = BAD_RESID;
		ID3D11Texture2D* resource;
		ID3D11ShaderResourceView* srv;
		ID3D11UnorderedAccessView* uav;
		ID3D11RenderTargetView* rtv;
		ID3D11DepthStencilView* dsv;

		bool isStub_ = false;
	};
}