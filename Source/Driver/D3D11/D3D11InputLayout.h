#pragma once

#include <Driver/DriverCommon.h>
#include "D3D11Driver.h"

class D3D11InputLayout
{
public:
	D3D11InputLayout(const InputLayoutElementDesc* descs, unsigned int num_descs, const D3D11ShaderSet& shader_set);
	~D3D11InputLayout();
	const ResId& getId() const { return id; }

private:
	ResId id = BAD_RESID;
	ComPtr<ID3D11InputLayout> inputLayout;
};