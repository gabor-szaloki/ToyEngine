#pragma once

#include "RendererCommon.h"
#include <memory>

struct SkyCbData
{
	XMFLOAT4 topColor_Exponent = XMFLOAT4(0.11f, 0.24f, 0.50f, 5.0f);
	XMFLOAT4 bottomColor_Exponent = XMFLOAT4(0.04f, 0.04f, 0.04f, 50.0f);
	XMFLOAT4 horizonColor = XMFLOAT4(0.77f, 0.91f, 1.f, 0.f);
	XMFLOAT4 skyIntensity_SunIntensity_SunAlpha_SunBeta = XMFLOAT4(1.0f, 2.0f, 4000.f, 1.f);
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