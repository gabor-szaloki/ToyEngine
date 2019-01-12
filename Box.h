#pragma once

#include "Drawable.h"

class Box : public Drawable
{
public:

	Box();
	~Box();

	void Init(ID3D11Device *device, ID3D11DeviceContext *context) override;
};

