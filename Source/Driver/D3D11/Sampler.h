#pragma once

#include "Driver.h"

namespace drv_d3d11
{
	class Sampler
	{
	public:
		Sampler(const SamplerDesc& desc_);
		~Sampler();
		void recreate();
		const ResId& getId() const { return id; }
		ID3D11SamplerState* getResource() const { return resource; }

	private:
		void create();

		SamplerDesc desc;
		ResId id = BAD_RESID;
		ID3D11SamplerState* resource;
	};
}