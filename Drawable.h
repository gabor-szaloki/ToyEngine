#pragma once

#include "Common.h"

class Drawable
{
public:

	Drawable();
	~Drawable();

	virtual void Init(ID3D11Device *device, ID3D11DeviceContext *context) = 0;
	void Draw(ID3D11DeviceContext *context);

protected:

	//XMMATRIX worldTransform;
	ID3D11Buffer *vertexBuffer;
};

