#pragma once

#include "Drawable.h"

class Plane : public Drawable
{

public:
	Plane();
	~Plane();

	void Init(ID3D11Device *device, ID3D11DeviceContext *context) override;
};

