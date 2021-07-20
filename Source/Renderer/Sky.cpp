#include "Sky.h"

#include <vector>

#include <3rdParty/imgui/imgui.h>
#include <Util/AutoImGui.h>
#include <Util/ImGuiExtensions.h>
#include <Driver/IBuffer.h>
#include <Driver/ITexture.h>
#include <Engine/AssetManager.h>
#include "WorldRenderer.h"

Sky::Sky()
{
	ShaderSetDesc skyShaderDesc("Sky", "Source/Shaders/Sky.shader");
	skyShaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "DefaultPostFxVsFunc";
	skyShaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "SkyBakeProceduralPS";
	proceduralBakeShader = drv->createShaderSet(skyShaderDesc);
	skyShaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "SkyBakeFromPanoramicTexturePS";
	textureBakeShader = drv->createShaderSet(skyShaderDesc);
	skyShaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "SkyRenderPS";
	renderShader = drv->createShaderSet(skyShaderDesc);

	RenderStateDesc skyRsDesc;
	skyRsDesc.depthStencilDesc.depthWrite = false;
	renderState = drv->createRenderState(skyRsDesc);

	BufferDesc cbDesc;
	cbDesc.bindFlags = BIND_CONSTANT_BUFFER;
	cbDesc.numElements = 1;
	cbDesc.name = "SkyBakeCb";
	cbDesc.elementByteSize = sizeof(bakeCbData);
	bakeCb.reset(drv->createBuffer(cbDesc));
	bakeCb->updateData(&bakeCbData);

	TextureDesc bakedCubeMapDesc("SkyBakedCubeMap", 2048, 2048, TexFmt::R32G32B32A32_FLOAT, 0); // TODO: reduce bit depth
	bakedCubeMapDesc.bindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
	bakedCubeMapDesc.miscFlags = RESOURCE_MISC_GENERATE_MIPS | RESOURCE_MISC_TEXTURECUBE;
	bakedCubeMap.reset(drv->createTexture(bakedCubeMapDesc));

	linearSampler = drv->createSampler(SamplerDesc(FILTER_MIN_MAG_MIP_LINEAR));

	setWorldRendererAmbientLighting();
}

void Sky::bakeProcedural()
{
	PROFILE_SCOPE("SkyBakeProcedural");

	bakeInternal(nullptr);
}

void Sky::bakeFromPanoramicTexture(const ITexture* panoramic_environment_map)
{
	PROFILE_SCOPE("SkyBakeFromPanoramicTexture");

	bakeInternal(panoramic_environment_map);
}

void Sky::render()
{
	if (!enabled)
		return;

	PROFILE_SCOPE("SkyRender");
	drv->setShader(renderShader, 0);
	drv->setRenderState(renderState);
	drv->setTexture(ShaderStage::PS, 1, bakedCubeMap->getId());
	drv->setSampler(ShaderStage::PS, 0, linearSampler);
	drv->draw(3, 0);

	drv->setTexture(ShaderStage::PS, 1, BAD_RESID);
	drv->setSampler(ShaderStage::PS, 0, BAD_RESID);
}

void Sky::gui()
{
	ImGui::Checkbox("Enabled", &enabled);

	bool changed = false;

	changed |= ImGui::ColorEdit3("Top color", &bakeCbData.topColor_Exponent.x);
	changed |= ImGui::ColorEdit3Srgb("Top color (SRGB)", &bakeCbData.topColor_Exponent.x);
	changed |= ImGui::DragFloat("Top exponent", &bakeCbData.topColor_Exponent.w);
	changed |= ImGui::ColorEdit3("Horizon color", &bakeCbData.horizonColor.x);
	changed |= ImGui::ColorEdit3Srgb("Horizon color (SRGB)", &bakeCbData.horizonColor.x);
	changed |= ImGui::ColorEdit3("Bottom color", &bakeCbData.bottomColor_Exponent.x);
	changed |= ImGui::ColorEdit3Srgb("Bottom color (SRGB)", &bakeCbData.bottomColor_Exponent.x);
	changed |= ImGui::DragFloat("Bottom exponent", &bakeCbData.bottomColor_Exponent.w);
	changed |= ImGui::SliderFloat("Sky intensity", &bakeCbData.skyIntensity_SunIntensity_SunAlpha_SunBeta.x, 0.f, 2.f);
	changed |= ImGui::SliderFloat("Sun intensity", &bakeCbData.skyIntensity_SunIntensity_SunAlpha_SunBeta.y, 0.f, 5.f);
	changed |= ImGui::DragFloat("Sun alpha", &bakeCbData.skyIntensity_SunIntensity_SunAlpha_SunBeta.z);
	changed |= ImGui::DragFloat("Sun beta", &bakeCbData.skyIntensity_SunIntensity_SunAlpha_SunBeta.w);

	static bool applyToAmbientLighting = true;
	changed |= ImGui::Checkbox("Apply changes to ambient lighting", &applyToAmbientLighting);

	if (changed)
	{
		bakeCb->updateData(&bakeCbData);
		if (applyToAmbientLighting)
			setWorldRendererAmbientLighting();
	}
}

void Sky::setWorldRendererAmbientLighting()
{
	XMFLOAT4 bottomColor = bakeCbData.bottomColor_Exponent;
	bottomColor.w = 1.f;
	XMFLOAT4 topColor = bakeCbData.topColor_Exponent;
	topColor.w = 1.f;
	float skyIntensity = bakeCbData.skyIntensity_SunIntensity_SunAlpha_SunBeta.x;
	wr->setAmbientLighting(bottomColor, topColor, skyIntensity);
}

void Sky::bakeInternal(const ITexture* panoramic_environment_map)
{
	bool procedural = panoramic_environment_map == nullptr;
	drv->setInputLayout(am->getDefaultInputLayout());
	drv->setShader(procedural ? proceduralBakeShader : textureBakeShader, 0);
	drv->setConstantBuffer(ShaderStage::PS, 3, bakeCb->getId()); // TODO: move to baking
	if (!procedural)
	{
		drv->setTexture(ShaderStage::PS, 0, panoramic_environment_map->getId());
		drv->setSampler(ShaderStage::PS, 0, linearSampler);
	}

	Camera cam;
	float cubeFaceWidth = bakedCubeMap->getDesc().width;
	cam.SetViewParams(XMVectorZero(), XMVectorSet(0, 1, 0, 0), 0, 0);
	cam.SetProjectionParams(cubeFaceWidth, cubeFaceWidth, XM_PIDIV2, 0.1f, 100.f);
	drv->setView(0, 0, cubeFaceWidth, cubeFaceWidth, 0, 1);

	// +X
	cam.SetRotation(0, XM_PIDIV2);
	wr->setCameraForShaders(cam);
	drv->setRenderTarget(bakedCubeMap->getId(), BAD_RESID, 0);
	drv->draw(3, 0);

	// -X
	cam.SetRotation(0, -XM_PIDIV2);
	wr->setCameraForShaders(cam);
	drv->setRenderTarget(bakedCubeMap->getId(), BAD_RESID, 1);
	drv->draw(3, 0);

	// +Y
	cam.SetRotation(-XM_PIDIV2, 0);
	wr->setCameraForShaders(cam);
	drv->setRenderTarget(bakedCubeMap->getId(), BAD_RESID, 2);
	drv->draw(3, 0);

	// -Y
	cam.SetRotation(XM_PIDIV2, 0);
	wr->setCameraForShaders(cam);
	drv->setRenderTarget(bakedCubeMap->getId(), BAD_RESID, 3);
	drv->draw(3, 0);

	// +Z
	cam.SetRotation(0, 0);
	wr->setCameraForShaders(cam);
	drv->setRenderTarget(bakedCubeMap->getId(), BAD_RESID, 4);
	drv->draw(3, 0);

	// -Z
	cam.SetRotation(0, XM_PI);
	wr->setCameraForShaders(cam);
	drv->setRenderTarget(bakedCubeMap->getId(), BAD_RESID, 5);
	drv->draw(3, 0);

	drv->setRenderTarget(BAD_RESID, BAD_RESID);
	if (!procedural)
	{
		drv->setTexture(ShaderStage::PS, 0, BAD_RESID);
		drv->setSampler(ShaderStage::PS, 0, BAD_RESID);
	}

	bakedCubeMap->generateMips();

	dirty = false;
}

REGISTER_IMGUI_WINDOW("Sky parameters", [] { wr->getSky().gui(); });