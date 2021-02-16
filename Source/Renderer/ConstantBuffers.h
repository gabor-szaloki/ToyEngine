#pragma once

#include <Common.h>

struct PerFrameConstantBufferData
{
	XMFLOAT4 ambientLightColor;
	XMFLOAT4 mainLightColor;
	XMFLOAT4 mainLightDirection;
	XMMATRIX mainLightShadowMatrix;
	XMFLOAT4 mainLightShadowResolution;
};

struct PerCameraConstantBufferData
{
	XMMATRIX view;
	XMMATRIX projection;
	XMFLOAT4 cameraWorldPosition;
	XMFLOAT4 viewVecLT;
	XMFLOAT4 viewVecRT;
	XMFLOAT4 viewVecLB;
	XMFLOAT4 viewVecRB;
};

struct PerObjectConstantBufferData
{
	XMMATRIX world;
};
