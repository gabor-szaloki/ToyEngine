#pragma once

#include <Driver/ITexture.h>
#include "DriverD3D12.h"

#include <vector>

namespace drv_d3d12
{
	class Texture : public ITexture
	{
	public:
		Texture(const TextureDesc* desc_ = nullptr, ID3D12Resource* tex = nullptr);
		~Texture() override;

		const TextureDesc& getDesc() const override { return desc; }
		const ResId& getId() const override { return id; };
		void* getViewHandle() const override { return nullptr; }; // TODO
		void updateData(unsigned int dst_subresource, const IntBox* dst_box, const void* src_data) override;
		virtual void copyResource(ResId dst_tex_id) const override;
		void generateMips() override;
		bool isStub() override { return isStub_; }
		void setStub(bool is_stub) override { isStub_ = is_stub; }
		void recreate(const TextureDesc& desc_) override;

		void transition(D3D12_RESOURCE_STATES dest_state, ID3D12GraphicsCommandList2* cmd_list = DriverD3D12::get().getFrameCmdList());
		ID3D12Resource* getResource() const { return resource; }
		CD3DX12_CPU_DESCRIPTOR_HANDLE getRtv(unsigned int slice = 0, unsigned int mip = 0) const;
		CD3DX12_CPU_DESCRIPTOR_HANDLE getDsv(unsigned int slice = 0, unsigned int mip = 0) const;

	private:
		void createResource();
		void createViews();
		void releaseAll();

		TextureDesc desc;
		unsigned int numMips = 0;
		ResId id = BAD_RESID;

		ID3D12Resource* resource = nullptr;
		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;

		CD3DX12_CPU_DESCRIPTOR_HANDLE srv{};
		CD3DX12_CPU_DESCRIPTOR_HANDLE uav{};
		std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> rtvs;
		std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> dsvs;

		ID3D12Resource* uploadBuffer = nullptr;

		bool isStub_ = false;
	};
}