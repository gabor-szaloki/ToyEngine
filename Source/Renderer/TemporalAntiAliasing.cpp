#include "TemporalAntiAliasing.h"

#include <vector>
#include <3rdParty/imgui/imgui.h>
#include <Util/AutoImGui.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>
#include "WorldRenderer.h"

static float halton(const unsigned int base, unsigned int index)
{
	// https://en.wikipedia.org/wiki/Halton_sequence
	float f = 1;
	float r = 0;
	while (index > 0)
	{
		f /= (float)base;
		r += f * (index % base);
		index = (unsigned int)floorf(index / base);
	}
	return r;
}

TemporalAntiAliasing::TemporalAntiAliasing()
{
	ComputeShaderDesc shaderDesc;
	shaderDesc.sourceFilePath = "Source/Shaders/TAA.shader";
	shaderDesc.name = "TAA";
	shaderDesc.shaderFuncName = "TAA";
	taaShader = drv->createComputeShader(shaderDesc);

	BufferDesc cbDesc;
	cbDesc.bindFlags = BIND_CONSTANT_BUFFER;
	cbDesc.numElements = 1;
	cbDesc.name = "TaaCb";
	cbDesc.elementByteSize = sizeof(cbData);
	cb.reset(drv->createBuffer(cbDesc));
	cb->updateData(&cbData);

	linearClampSampler = drv->createSampler(SamplerDesc(FILTER_MIN_MAG_MIP_LINEAR, TexAddr::CLAMP));
}

XMFLOAT2 TemporalAntiAliasing::getJitter(unsigned int frame) const
{
	unsigned int haltonFrame = frame % jitterFrameMod + 1; // Halton sequence generator should be indexed from 1
	return enabled ? XMFLOAT2(halton(2, haltonFrame) - 0.5f, halton(3, haltonFrame) - 0.5f) : XMFLOAT2(0.f, 0.f);
}

void TemporalAntiAliasing::addJitterToProjectionMatrix(XMMATRIX& proj, unsigned int frame, float viewport_width, float viewport_height) const
{
	XMFLOAT2 jitter = getJitter(frame);
	proj.r[2].m128_f32[0] = (2.0f * jitter.x) / viewport_width;
	proj.r[2].m128_f32[1] = (2.0f * jitter.y) / viewport_height;
}

void TemporalAntiAliasing::perform(const PerformParams& params)
{
	PROFILE_SCOPE("TAA");

	if (!enabled)
	{
		params.currentFrameTex->copyResource(params.destTex->getId());
		return;
	}

	const TextureDesc& destTexDesc = params.destTex->getDesc();
	const float width = destTexDesc.width;
	const float height = destTexDesc.height;

	static XMMATRIX prevViewMatrix = params.viewMatrix;
	static XMMATRIX prevProjMatrix = params.projMatrix;
	addJitterToProjectionMatrix(prevProjMatrix, params.frame, width, height);
	cbData.prevViewProjMatrixWithCurrentFrameJitter = prevViewMatrix * prevProjMatrix;
	prevViewMatrix = params.viewMatrix;
	prevProjMatrix = params.projMatrix;

	cb->updateData(&cbData);

	drv->setShader(taaShader, 0);

	drv->setConstantBuffer(ShaderStage::CS, 4, cb->getId());

	drv->setRwTexture(0, params.destTex->getId());
	drv->setTexture(ShaderStage::CS, 0, params.currentFrameTex->getId());
	drv->setTexture(ShaderStage::CS, 1, params.historyTex->getId());
	drv->setTexture(ShaderStage::CS, 2, params.depthTex->getId());
	drv->setSampler(ShaderStage::CS, 0, linearClampSampler);

	drv->dispatch((width + 16 - 1) / 16, (height + 16 - 1) / 16, 1);

	drv->setRwTexture(0, BAD_RESID);
	drv->setTexture(ShaderStage::CS, 0, BAD_RESID);
	drv->setTexture(ShaderStage::CS, 1, BAD_RESID);
	drv->setTexture(ShaderStage::CS, 2, BAD_RESID);
	drv->setSampler(ShaderStage::CS, 0, BAD_RESID);
}

void TemporalAntiAliasing::gui()
{
	ImGui::Checkbox("Enabled", &enabled);
	ImGui::SliderInt("Jitter frame modulo", &jitterFrameMod, 1, 64);
	ImGui::SliderFloat("History weight", &cbData.historyWeight, 0, 1);
	ImGui::SliderFloat("Neighborhood clipping strength", &cbData.neighborhoodClippingStrength, 0, 1);
	ImGui::SliderFloat("Mip bias", &mipBias, -5, 5);
	ImGui::SliderFloat("Sharpening", &cbData.sharpening, 0, 1);
}

REGISTER_IMGUI_WINDOW("TAA settings", [] { wr->getTaa().gui(); });