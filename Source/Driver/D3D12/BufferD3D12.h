#pragma once

#include <Driver/IBuffer.h>

#include "DriverD3D12.h"

namespace drv_d3d12
{
	class Buffer : public IBuffer
	{
	public:
		Buffer(const BufferDesc& desc_);
		~Buffer() override;

		const BufferDesc& getDesc() const override { return desc; }
		const ResId& getId() const override { return id; };
		void updateData(const void* src_data) override;

		void transition(D3D12_RESOURCE_STATES dest_state, ID3D12GraphicsCommandList2* cmd_list = DriverD3D12::get().getFrameCmdList());
		ID3D12Resource* getResource() const { return resource; }

	private:
		BufferDesc desc;
		ResId id = BAD_RESID;
		ID3D12Resource* resource = nullptr;
		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;

		ID3D12Resource* uploadBuffer = nullptr;
		uint64 uploadFenceValue = 0;
	};
}