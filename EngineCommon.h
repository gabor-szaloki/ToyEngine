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
	XMFLOAT4 ambientLightColor;
	XMFLOAT4 mainLightColor;
	XMFLOAT4 mainLightDirection;
	//XMMATRIX mainLightShadowMatrix;
	XMMATRIX mainLightView;
	XMMATRIX mainLightProjection;
};

struct PerCameraConstantBufferData
{
	XMMATRIX view;
	XMMATRIX projection;
	XMFLOAT3 cameraWorldPosition;
};

struct PerObjectConstantBufferData 
{ 
	XMMATRIX world; 
};