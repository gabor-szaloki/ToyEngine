#pragma once

#include "RendererCommon.h"

struct StandardVertexData
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT4 color;
	XMFLOAT2 uv;
};

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
