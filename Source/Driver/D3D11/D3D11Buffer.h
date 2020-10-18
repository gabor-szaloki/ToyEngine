#pragma once

#include <Driver/IBuffer.h>
#include "D3D11Driver.h"

class D3D11Buffer : public IBuffer
{
public:
	D3D11Buffer(const BufferDesc& desc_);
	~D3D11Buffer();

	const BufferDesc& getDesc() const override { return desc; }
	const ResId& getId() const override { return id; };
	void updateData(const void* src_data) override;

	ID3D11Buffer* getResource() const { return resource.Get(); }
	ID3D11ShaderResourceView* getSrv() const { return srv.Get(); }
	ID3D11UnorderedAccessView* getUav() const { return uav.Get(); }

private:
	void createViews();

	BufferDesc desc;
	ResId id = BAD_RESID;
	ComPtr<ID3D11Buffer> resource;
	ComPtr<ID3D11ShaderResourceView> srv;
	ComPtr<ID3D11UnorderedAccessView> uav;
};