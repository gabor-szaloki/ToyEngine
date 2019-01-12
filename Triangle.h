#pragma once

#include "Drawable.h"

class Triangle : public Drawable
{
public:
	Triangle();
	~Triangle();

	void Init(ID3D11Device *device, ID3D11DeviceContext *context) override;
};

