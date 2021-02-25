#pragma once

#include <memory>

#include <Common.h>
#include <Util/ResIdHolder.h>

struct SsaoCbData
{
	XMFLOAT4 _RadiusToScreen_R2_NegInvR2_NDotVBias;
	XMFLOAT4 _PowExponent_AOMultiplier__;
	XMFLOAT4 _Resolution_InvResolution;
	XMFLOAT4 _ProjInfo;
};

class Ssao
{
public:
	Ssao(XMINT2 resolution_);
	void perform();
	void gui();
	ITexture* getResultTex() { return ssaoTex.get(); };

private:
	void updateCb();

	XMINT2 resolution;
	ResIdHolder hbaoShader;
	ResIdHolder blurShader;
	ResIdHolder renderState;
	SsaoCbData cbData;
	std::unique_ptr<IBuffer> cb;
	std::unique_ptr<ITexture> ssaoTex;
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