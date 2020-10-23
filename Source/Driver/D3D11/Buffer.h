#pragma once

#include <Driver/IBuffer.h>
#include "Driver.h"

namespace drv_d3d11
{
	class Buffer : public IBuffer
	{
	public:
		Buffer(const BufferDesc& desc_);
		~Buffer() override;

		const BufferDesc& getDesc() const override { return desc; }
		const ResId& getId() const override { return id; };
		void updateData(const void* src_data) override;

		ID3D11Buffer* getResource() const { return resource; }
		ID3D11ShaderResourceView* getSrv() const { return srv; }
		ID3D11UnorderedAccessView* getUav() const { return uav; }

	private:
		void createViews();

		BufferDesc desc;
		ResId id = BAD_RESID;
		ID3D11Buffer* resource;
		ID3D11ShaderResourceView* srv;
		ID3D11UnorderedAccessView* uav;
	};
}