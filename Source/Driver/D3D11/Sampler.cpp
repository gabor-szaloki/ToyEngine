#include "Sampler.h"

#include <assert.h>

using namespace drv_d3d11;

Sampler::Sampler(const SamplerDesc& desc_) : desc(desc_)
{
	create();
	id = Driver::get().registerSampler(this);
}

Sampler::~Sampler()
{
	SAFE_RELEASE(resource);
	Driver::get().unregisterSampler(id);
}

void Sampler::recreate()
{
	SAFE_RELEASE(resource);
	create();
}

void Sampler::create()
{
	const unsigned int driverAnisotropy = Driver::get().getSettings().textureFilteringAnisotropy;
	D3D11_FILTER defaultFilter = driverAnisotropy > 0 ? D3D11_FILTER_ANISOTROPIC : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	D3D11_SAMPLER_DESC sd{};
	sd.Filter = desc.filter == FILTER_DEFAULT ? defaultFilter : (D3D11_FILTER)desc.filter;
	sd.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)desc.addressU;
	sd.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)desc.addressV;
	sd.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)desc.addressW;
	sd.MipLODBias = desc.mipBias;
	sd.MaxAnisotropy = driverAnisotropy;
	sd.ComparisonFunc = (D3D11_COMPARISON_FUNC)desc.comparisonFunc;
	sd.MinLOD = desc.minLOD;
	sd.MaxLOD = desc.maxLOD;
	for (int i = 0; i < 4; i++)
		sd.BorderColor[i] = desc.borderColor[i];

	HRESULT hr = Driver::get().getDevice().CreateSamplerState(&sd, &resource);
	assert(SUCCEEDED(hr));
}
