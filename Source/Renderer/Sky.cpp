#include "Sky.h"

#include <vector>

#include <3rdParty/imgui/imgui.h>
#include <Util/AutoImGui.h>
#include <Util/ImGuiExtensions.h>
#include <Driver/IBuffer.h>
#include <Driver/ITexture.h>
#include <Engine/AssetManager.h>
#include "WorldRenderer.h"
#include "CubeRenderHelper.h"

Sky::Sky()
{
	ShaderSetDesc skyShaderDesc("SkyBakeProcedural", "Source/Shaders/Sky.shader");
	skyShaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "DefaultPostFxVsFunc";
	skyShaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "SkyBakeProceduralPS";
	proceduralBakeShader = drv->createShaderSet(skyShaderDesc);

	skyShaderDesc.name = "SkyBakeFromPanoramicTexture";
	skyShaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "SkyBakeFromPanoramicTexturePS";
	textureBakeShader = drv->createShaderSet(skyShaderDesc);

	skyShaderDesc.name = "SkyRender";
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

	cbDesc.name = "SkyRenderCb";
	cbDesc.elementByteSize = sizeof(renderCbData);
	renderCb.reset(drv->createBuffer(cbDesc));
	renderCb->updateData(&renderCbData);

	TextureDesc bakedCubeMapDesc("SkyBakedCubeMap", 2048, 2048, TexFmt::R32G32B32A32_FLOAT, 0); // TODO: reduce bit depth
	bakedCubeMapDesc.bindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
	bakedCubeMapDesc.miscFlags = RESOURCE_MISC_GENERATE_MIPS | RESOURCE_MISC_TEXTURECUBE;
	bakedCubeMap.reset(drv->createTexture(bakedCubeMapDesc));

	linearSampler = drv->createSampler(SamplerDesc(FILTER_MIN_MAG_MIP_LINEAR));
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

void Sky::render(const ITexture* sky_cube_override, float mip_override)
{
	if (!enabled)
		return;

	PROFILE_SCOPE("SkyRender");

	drv->setShader(renderShader, 0);
	drv->setRenderState(renderState);

	renderCbData.mip.x = mip_override;
	renderCb->updateData(&renderCbData);
	drv->setConstantBuffer(ShaderStage::PS, 4, renderCb->getId());

	ResId skyCubeId = sky_cube_override != nullptr ? sky_cube_override->getId() : bakedCubeMap->getId();
	drv->setTexture(ShaderStage::PS, 1, skyCubeId);
	drv->setSampler(ShaderStage::PS, 0, linearSampler);

	drv->draw(3, 0);

	drv->setTexture(ShaderStage::PS, 1, BAD_RESID);
	drv->setSampler(ShaderStage::PS, 0, BAD_RESID);
}

void Sky::gui()
{
	ImGui::Checkbox("Enabled", &enabled);

	if (isBakedFromTexture)
	{
		ImGui::TextUnformatted("Sky is baked from texture");
		return;
	}

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

	if (changed)
	{
		bakeCb->updateData(&bakeCbData);
		markDirty();
	}
}

void Sky::bakeInternal(const ITexture* panoramic_environment_map)
{
	bool procedural = panoramic_environment_map == nullptr;
	drv->setInputLayout(am->getDefaultInputLayout());
	drv->setShader(procedural ? proceduralBakeShader : textureBakeShader, 0);
	drv->setConstantBuffer(ShaderStage::PS, 3, bakeCb->getId());
	if (!procedural)
	{
		drv->setTexture(ShaderStage::PS, 0, panoramic_environment_map->getId());
		drv->setSampler(ShaderStage::PS, 0, linearSampler);
	}

	CubeRenderHelper cubeRenderHelper;
	cubeRenderHelper.beginRender(XMVectorZero(), bakedCubeMap.get());
	cubeRenderHelper.renderAllFaces();
	cubeRenderHelper.finishRender();

	if (!procedural)
	{
		drv->setTexture(ShaderStage::PS, 0, BAD_RESID);
		drv->setSampler(ShaderStage::PS, 0, BAD_RESID);
	}

	bakedCubeMap->generateMips();

	dirty = false;
	isBakedFromTexture = !procedural;
}

REGISTER_IMGUI_WINDOW("Sky parameters", [] { wr->getSky().gui(); });