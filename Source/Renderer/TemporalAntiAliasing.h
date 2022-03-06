#pragma once

#include <memory>

#include <Common.h>
#include <Util/ResIdHolder.h>

class TemporalAntiAliasing
{
public:
	struct PerformParams
	{
		ITexture* destTex = nullptr;
		ITexture* currentFrameTex = nullptr;
		ITexture* historyTex = nullptr;
		ITexture* depthTex = nullptr;
		XMMATRIX viewMatrix{};
		XMMATRIX projMatrix{};
		unsigned int frame = 0;
	};

	TemporalAntiAliasing();
	XMFLOAT2 getJitter(unsigned int frame) const;
	void addJitterToProjectionMatrix(XMMATRIX& proj, unsigned int frame, float viewport_width, float viewport_height) const;
	float getMipBias() const { return enabled ? mipBias : 0.f; }
	void perform(const PerformParams& textures);
	void gui();

private:
	struct SkyBakeCbData
	{
		float historyWeight = 0.95f;
		float neighborhoodClippingStrength = 1.0f;
		float sharpening = 0.5f;
		float pad;
		XMMATRIX prevViewProjMatrixWithCurrentFrameJitter{};
	} cbData;

	bool enabled = true;
	int jitterFrameMod = 16;
	float mipBias = -0.5;
	ResIdHolder taaShader;
	ResIdHolder linearClampSampler;
	std::unique_ptr<IBuffer> cb;
};