#pragma once

#include "RendererCommon.h"
#include <memory>

struct SkyCbData
{
	XMFLOAT4 topColor_Exponent = XMFLOAT4(0.37f, 0.52f, 0.73f, 8.5f);
	XMFLOAT4 bottomColor_Exponent = XMFLOAT4(0.23f, 0.23f, 0.23f, 20.0f);
	XMFLOAT4 horizonColor = XMFLOAT4(0.89f, 0.96f, 1.f, 0.f);
	XMFLOAT4 skyIntensity_SunIntensity_SunAlpha_SunBeta = XMFLOAT4(1.0f, 2.0f, 550.f, 1.f);
};

class Sky
{
public:
	Sky();
	void render();
	void gui();

private:
	ResIdHolder skyShader;
	ResIdHolder skyRenderState;
	SkyCbData cbData;
	std::unique_ptr<IBuffer> cb;
};