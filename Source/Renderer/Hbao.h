#pragma once

#include <memory>

#include <Common.h>
#include <Util/ResIdHolder.h>

struct HbaoCbData
{
	XMFLOAT4 _RadiusToScreen_R2_NegInvR2_NDotVBias;
	XMFLOAT4 _PowExponent_AOMultiplier_BlurSharpness_;
	XMFLOAT4 _Resolution_InvResolution;
	XMFLOAT4 _ProjInfo;
};

class Hbao
{
public:
	Hbao(XMINT2 resolution_);
	void perform();
	void gui(float& resolution_scale);
	ITexture* getResultTex();

private:
	void updateCb();

	XMINT2 resolution;
	ViewportParams viewport;
	ResIdHolder renderState;
	ResIdHolder hbaoCalcShader;
	ResIdHolder hbaoBlurShader;
	HbaoCbData cbData;
	std::unique_ptr<IBuffer> cb;
	std::unique_ptr<ITexture> hbaoTex[2];
	std::unique_ptr<ITexture> randomTex;

	struct Tweak {
		bool enabled = true;
		float intensity = 1.5f;
		float bias = 0.1f;
		float radius = 2.0f;
		float blurSharpness = 40.0f;
		bool blur = true;
	} tweak;
};