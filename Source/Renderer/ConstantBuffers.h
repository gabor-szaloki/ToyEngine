#pragma once

#include <Common.h>

struct PerFrameConstantBufferData
{
	XMFLOAT4 ambientLightBottomColor;
	XMFLOAT4 ambientLightTopColor;
	XMFLOAT4 mainLightColor;
	XMFLOAT4 mainLightDirection;
	XMMATRIX mainLightShadowMatrix;
	XMFLOAT4 mainLightShadowParams;
	XMFLOAT4 tonemappingParams;
	XMFLOAT4 timeParams;
};

struct PerCameraConstantBufferData
{
	XMMATRIX view;
	XMMATRIX projection;
	XMMATRIX viewProjection;
	XMFLOAT4 projectionParams; // x: zn*zf, y: zn-zf, z: zf, w: perspective = 1 : 0
	XMFLOAT4 cameraWorldPosition;
	XMFLOAT4 viewportResolution;
	XMFLOAT4 viewVecLT;
	XMFLOAT4 viewVecRT;
	XMFLOAT4 viewVecLB;
	XMFLOAT4 viewVecRB;
};

struct PerObjectConstantBufferData
{
	XMMATRIX world;
};

struct PerMaterialConstantBufferData
{
	XMFLOAT4 materialColor;
	XMFLOAT4 materialParams;
};