#pragma once

#include "DriverD3D12.h"

#include <vector>

namespace drv_d3d12
{
	class InputLayout
	{
	public:
		InputLayout(const InputLayoutElementDesc* descs, uint num_descs);
		~InputLayout();
		const ResId& getId() const { return id; }

		const std::vector<D3D12_INPUT_ELEMENT_DESC>& getInputElements() { return inputElements; };

	private:
		ResId id = BAD_RESID;
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
	};
}