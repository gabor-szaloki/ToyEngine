#pragma once

#include "Common.h"

struct StandardVertexData
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT4 tangent;
	XMFLOAT4 color;
	XMFLOAT2 uv;
};

struct PerFrameConstantBufferData 
{ 
	XMMATRIX view; 
	XMMATRIX projection; 
	XMFLOAT4 ambientLightColor;
	XMFLOAT4 mainLightColor;
	XMFLOAT4 mainLightDirection;
};

struct PerObjectConstantBufferData 
{ 
	XMMATRIX world; 
};