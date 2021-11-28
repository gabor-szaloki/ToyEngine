#pragma once

#include <Common.h>

static constexpr unsigned int PER_FRAME_CONSTANT_BUFFER_SLOT = 0;
static constexpr unsigned int PER_CAMERA_CONSTANT_BUFFER_SLOT = 1;
static constexpr unsigned int PER_OBJECT_CONSTANT_BUFFER_SLOT = 2;
static constexpr unsigned int PER_MATERIAL_CONSTANT_BUFFER_SLOT = 3;

struct PerFrameConstantBufferData
{
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
	XMFLOAT4 objectParams0;
};

struct PerMaterialConstantBufferData
{
	XMFLOAT4 materialColor;
	XMFLOAT4 materialParams0;
	XMFLOAT4 materialParams1;
};