#include "Hbao.h"
#include <Shaders/HBAO.hlsli>

#include <3rdParty/imgui/imgui.h>
#include <Util/AutoImGui.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>

#include "WorldRenderer.h"
#include "Camera.h"

Hbao::Hbao(XMINT2 resolution_) : resolution(resolution_)
{
	RenderStateDesc rsDesc;
	rsDesc.depthStencilDesc.depthTest = false;
	rsDesc.depthStencilDesc.depthWrite = false;
	renderState = drv->createRenderState(rsDesc);

	viewport.x = viewport.y = 0;
	viewport.w = resolution.x;
	viewport.h = resolution.y;
	viewport.z_min = viewport.z_max = 0;

	SamplerDesc smpDesc(FILTER_MIN_MAG_MIP_LINEAR, TexAddr::CLAMP);
	linearClampSampler = drv->createSampler(smpDesc);
	smpDesc.filter = FILTER_MIN_MAG_MIP_POINT;
	pointClampSampler = drv->createSampler(smpDesc);

	ShaderSetDesc hbaoCalcShaderDesc("HbaoCalc", "Source/Shaders/HBAO.shader");
	hbaoCalcShaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "DefaultPostFxVsFunc";
	hbaoCalcShaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "HbaoCalcPs";
	hbaoCalcShader = drv->createShaderSet(hbaoCalcShaderDesc);

	ShaderSetDesc hbaoBlurShaderDesc("HbaoBlur", "Source/Shaders/HBAO.shader");
	hbaoBlurShaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "DefaultPostFxVsFunc";
	hbaoBlurShaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "HbaoBlurPs";
	hbaoBlurShader = drv->createShaderSet(hbaoBlurShaderDesc);

	TextureDesc hbaoTexDesc("hbaoTex_0", resolution.x, resolution.y, TexFmt::R8_UNORM, 1,
		ResourceUsage::DEFAULT, BIND_RENDER_TARGET | BIND_SHADER_RESOURCE);
	hbaoTex[0].reset(drv->createTexture(hbaoTexDesc));
	hbaoTexDesc.name = "hbaoTex_1";
	hbaoTex[1].reset(drv->createTexture(hbaoTexDesc));

	TextureDesc randomTexDesc("ssaoRandomTex", RANDOM_TEX_SIZE, RANDOM_TEX_SIZE, TexFmt::R16G16B16A16_SNORM, 1,
		ResourceUsage::DEFAULT, BIND_SHADER_RESOURCE);
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
	cbDesc.name = "HbaoCb";
	cbDesc.elementByteSize = sizeof(cbData);
	cb.reset(drv->createBuffer(cbDesc));
	cb->updateData(&cbData);

	updateCb();
}

void Hbao::perform()
{
	PROFILE_SCOPE("HBAO");

	if (!tweak.enabled)
	{
		ResId targets[] = { hbaoTex[0]->getId(), hbaoTex[1]->getId() };
		drv->setRenderTargets(2, targets, BAD_RESID);
		drv->clearRenderTargets(RenderTargetClearParams::clear_color(1, 1, 1, 1));
		return;
	}

	ViewportParams originalViewport;
	drv->getView(originalViewport);
	drv->setView(viewport);
	drv->setRenderState(renderState);

	// Calc pass
	{
		PROFILE_SCOPE("Calc");

		drv->setShader(hbaoCalcShader, 0);
		drv->setRenderTarget(hbaoTex[0]->getId(), BAD_RESID);
		drv->setConstantBuffer(ShaderStage::PS, 3, cb->getId());
		drv->setTexture(ShaderStage::PS, 0, wr->getDepthTex()->getId());
		drv->setSampler(ShaderStage::PS, 0, pointClampSampler);
		drv->setTexture(ShaderStage::PS, 1, randomTex->getId());
		cbData._Resolution_InvResolution.z = 1.0f / resolution.x;
		cbData._Resolution_InvResolution.w = 1.0f / resolution.y;
		cb->updateData(&cbData);
		drv->draw(3, 0);
	}

	// Blur passes
	if (tweak.blur)
	{
		PROFILE_SCOPE("Blur");

		drv->setShader(hbaoBlurShader, 0);

		// Horizontal
		drv->setRenderTarget(hbaoTex[1]->getId(), BAD_RESID);
		drv->setTexture(ShaderStage::PS, 2, hbaoTex[0]->getId());
		drv->setSampler(ShaderStage::PS, 2, linearClampSampler);
		cbData._Resolution_InvResolution.z = 1.0f / resolution.x;
		cbData._Resolution_InvResolution.w = 0;
		cb->updateData(&cbData);
		drv->draw(3, 0);
		drv->setTexture(ShaderStage::PS, 2, BAD_RESID); // otherwise next debug layer complains in next setRenderTarget call that texture is still bound on input

		// Vertical
		drv->setRenderTarget(hbaoTex[0]->getId(), BAD_RESID);
		drv->setTexture(ShaderStage::PS, 2, hbaoTex[1]->getId()); // linear sampler is already set
		cbData._Resolution_InvResolution.z = 0;
		cbData._Resolution_InvResolution.w = 1.0f / resolution.y;
		cb->updateData(&cbData);
		drv->draw(3, 0);
	}

	// Cleanup
	drv->setTexture(ShaderStage::PS, 0, BAD_RESID);
	drv->setTexture(ShaderStage::PS, 1, BAD_RESID);
	drv->setTexture(ShaderStage::PS, 2, BAD_RESID);
	drv->setSampler(ShaderStage::PS, 0, BAD_RESID);
	drv->setSampler(ShaderStage::PS, 2, BAD_RESID);
	drv->setRenderTarget(BAD_RESID, BAD_RESID);
	drv->setView(originalViewport);
}

void Hbao::gui(float &resolution_scale)
{
	bool cbChanged = false;
	ImGui::Checkbox("Enabled", &tweak.enabled);
	ImGui::SliderFloat("Resolution scale", &resolution_scale, 0.1f, 1.0f);
	resolution_scale = std::clamp(resolution_scale, 0.1f, 1.0f);
	cbChanged |= ImGui::SliderFloat("Radius", &tweak.radius, 0.0f, 4.0f);
	cbChanged |= ImGui::SliderFloat("Intensity", &tweak.intensity, 0.0f, 4.0f);
	cbChanged |= ImGui::SliderFloat("Bias", &tweak.bias, 0.0f, 0.9999f);
	ImGui::Checkbox("Blur active", &tweak.blur);
	cbChanged |= ImGui::SliderFloat("Blur sharpness", &tweak.blurSharpness, 0.0f, 128.0f);
	if (cbChanged)
		updateCb();
	if (ImGui::Button("Show SSAO texture"))
		autoimgui::set_window_opened("SSAO tex debug", !autoimgui::is_window_opened("SSAO tex debug"));
}

ITexture* Hbao::getResultTex()
{
	return hbaoTex[0].get();
}

void Hbao::updateCb()
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
	cbData._PowExponent_AOMultiplier_BlurSharpness_ = XMFLOAT4(powExponent, aoMultiplier, tweak.blurSharpness, 0);

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