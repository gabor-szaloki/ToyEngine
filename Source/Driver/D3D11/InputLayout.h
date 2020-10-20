#pragma once

#include <Driver/DriverCommon.h>
#include "Driver.h"

namespace drv_d3d11
{
	class InputLayout
	{
	public:
		InputLayout(const InputLayoutElementDesc* descs, unsigned int num_descs, const ShaderSet& shader_set);
		~InputLayout();
		const ResId& getId() const { return id; }

	private:
		ResId id = BAD_RESID;
		ComPtr<ID3D11InputLayout> inputLayout;
	};
}