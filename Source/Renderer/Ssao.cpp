#include "Ssao.h"
#include <Shaders/HBAO.hlsli>

#include <3rdParty/imgui/imgui.h>
#include <Util/AutoImGui.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>

#include "WorldRenderer.h"
#include "Camera.h"

Ssao::Ssao(XMINT2 resolution_) : resolution(resolution_)
{
	ShaderSetDesc hbaoShaderDesc("HBAO", "Source/Shaders/HBAO.shader");
	hbaoShaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "DefaultPostFxVsFunc";
	hbaoShaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "HbaoPs";
	hbaoShader = drv->createShaderSet(hbaoShaderDesc);

	RenderStateDesc rsDesc;
	rsDesc.depthStencilDesc.depthTest = false;
	rsDesc.depthStencilDesc.depthWrite = false;
	renderState = drv->createRenderState(rsDesc);

	TextureDesc ssaoTexDesc("ssaoTex", resolution.x, resolution.y, TexFmt::R16_UNORM, 1,
		ResourceUsage::DEFAULT, BIND_RENDER_TARGET | BIND_SHADER_RESOURCE);
	ssaoTex.reset(drv->createTexture(ssaoTexDesc));

	TextureDesc randomTexDesc("ssaoRandomTex", RANDOM_TEX_SIZE, RANDOM_TEX_SIZE, TexFmt::R16G16B16A16_SNORM, 1,
		ResourceUsage::DEFAULT, BIND_SHADER_RESOURCE);
	randomTexDesc.hasSampler = false;
	randomTex.reset(drv->createTexture(randomTexDesc));

	signed short randomData[RANDOM_ELEMENTS * 4];
	for (int i = 0; i < RANDOM_ELEMENTS; i++)
	{
		float rand1 = rand() / float(RAND_MAX);
		float rand2 = rand() / float(RAND_MAX);

		// Use random rotation angles in [0,2PI/NUM_DIRECTIONS)
		float angle = 2.f * M_PI * rand1 / NUM_DIRECTIONS;

		XMFLOAT4 randomTexel;
		randomTexel.x = cosf(angle);
		randomTexel.y = sinf(angle);
		randomTexel.z = rand2;
		randomTexel.w = 0;

		randomData[i * 4 + 0] = (signed short)((1<<15) * randomTexel.x);
		randomData[i * 4 + 1] = (signed short)((1<<15) * randomTexel.y);
		randomData[i * 4 + 2] = (signed short)((1<<15) * randomTexel.z);
		randomData[i * 4 + 3] = (signed short)((1<<15) * randomTexel.w);
	}
	randomTex->updateData(0, nullptr, static_cast<void*>(randomData));

	BufferDesc cbDesc;
	cbDesc.bindFlags = BIND_CONSTANT_BUFFER;
	cbDesc.numElements = 1;
	cbDesc.name = "SsaoCb";
	cbDesc.elementByteSize = sizeof(cbData);
	cb.reset(drv->createBuffer(cbDesc));
	cb->updateData(&cbData);

	updateCb();
}

void Ssao::perform()
{
	PROFILE_SCOPE("SSAO");

	drv->setShader(hbaoShader, 0);
	drv->setRenderState(renderState);
	drv->setRenderTarget(ssaoTex->getId(), BAD_RESID);
	drv->setConstantBuffer(ShaderStage::PS, 3, cb->getId());
	drv->setTexture(ShaderStage::PS, 0, wr->getDepthTex()->getId(), true);
	drv->setTexture(ShaderStage::PS, 1, randomTex->getId(), false);

	if (tweak.enabled)
		drv->draw(3, 0);
	else
		drv->clearRenderTargets(RenderTargetClearParams::clear_color(1, 1, 1, 1));

	drv->setTexture(ShaderStage::PS, 0, BAD_RESID, false);
	drv->setTexture(ShaderStage::PS, 1, BAD_RESID, false);
	drv->setRenderTarget(BAD_RESID, BAD_RESID);
}

void Ssao::gui()
{
	bool changed = false;
	changed |= ImGui::Checkbox("Enabled", &tweak.enabled);
	changed |= ImGui::SliderFloat("Radius", &tweak.radius, 0.0f, 4.0f);
	changed |= ImGui::SliderFloat("Intensity", &tweak.intensity, 0.0f, 4.0f);
	changed |= ImGui::SliderFloat("Bias", &tweak.bias, 0.0f, 0.9999f);
	changed |= ImGui::Checkbox("Blur active", &tweak.blur);
	changed |= ImGui::SliderFloat("Blur sharpness", &tweak.blurSharpness, 0.0f, 128.0f);
	if (changed)
		updateCb();
}

void Ssao::updateCb()
{
	const Camera& cam = wr->getCamera();
	XMFLOAT2 resf((float)resolution.x, (float)resolution.y);

	float r = tweak.radius;
	float r2 = r * r;
	float negInvR2 = -1.0f / r2;
	float projScale = float(resf.y) / (tanf(cam.GetFOV() * 0.5f) * 2.0f);
	float radiusToScreen = r * 0.5f * projScale;
	float nDotVBias = std::clamp(tweak.bias, 0.f, 1.f);
	cbData._RadiusToScreen_R2_NegInvR2_NDotVBias = XMFLOAT4(radiusToScreen, r2, negInvR2, nDotVBias);

	float powExponent = std::max(0.f, tweak.intensity);
	float aoMultiplier = 1.0f / (1.0f - nDotVBias);
	cbData._PowExponent_AOMultiplier__ = XMFLOAT4(powExponent, aoMultiplier, 0, 0);

	cbData._Resolution_InvResolution = XMFLOAT4(resf.x, resf.y, 1.0f / resf.x, 1.0f / resf.y);

	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, cam.GetProjectionMatrix());
	cbData._ProjInfo = XMFLOAT4(
		2.0f / projMatrix.m[0][0],
		2.0f / projMatrix.m[1][1],
		-1.0f / resf.x,
		-1.0f / resf.y);

	cb->updateData(&cbData);
}