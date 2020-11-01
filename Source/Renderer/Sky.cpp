#include "Sky.h"

#include <3rdParty/imgui/imgui.h>
#include <Util/AutoImGui.h>
#include <Driver/IBuffer.h>
#include "WorldRenderer.h"

Sky::Sky()
{
	ShaderSetDesc skyShaderDesc("Source/Shaders/Sky.shader");
	skyShaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "DefaultPostFxVsFunc";
	skyShaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "SkyPS";
	skyShader = drv->createShaderSet(skyShaderDesc);

	RenderStateDesc skyRsDesc;
	skyRsDesc.depthStencilDesc.depthWrite = false;
	skyRenderState = drv->createRenderState(skyRsDesc);

	BufferDesc cbDesc;
	cbDesc.bindFlags = BIND_CONSTANT_BUFFER;
	cbDesc.numElements = 1;
	cbDesc.name = "SkyCb";
	cbDesc.elementByteSize = sizeof(cbData);
	cb.reset(drv->createBuffer(cbDesc));
	cb->updateData(&cbData);
}

void Sky::render()
{
	drv->setShaderSet(skyShader);
	drv->setRenderState(skyRenderState);
	drv->setConstantBuffer(ShaderStage::PS, 3, cb->getId());
	drv->draw(3, 0);
}

void Sky::gui()
{
	bool changed = false;
	changed |= ImGui::ColorEdit3("Top color", &cbData.topColor_Exponent.x);
	changed |= ImGui::DragFloat("Top exponent", &cbData.topColor_Exponent.w);
	changed |= ImGui::ColorEdit3("Horizon color", &cbData.horizonColor.x);
	changed |= ImGui::ColorEdit3("Bottom color", &cbData.bottomColor_Exponent.x);
	changed |= ImGui::DragFloat("Bottom exponent", &cbData.bottomColor_Exponent.w);
	changed |= ImGui::SliderFloat("Sky intensity", &cbData.skyIntensity_SunIntensity_SunAlpha_SunBeta.x, 0.f, 2.f);
	changed |= ImGui::SliderFloat("Sun intensity", &cbData.skyIntensity_SunIntensity_SunAlpha_SunBeta.y, 0.f, 5.f);
	changed |= ImGui::DragFloat("Sun alpha", &cbData.skyIntensity_SunIntensity_SunAlpha_SunBeta.z);
	changed |= ImGui::DragFloat("Sun beta", &cbData.skyIntensity_SunIntensity_SunAlpha_SunBeta.w);
	if (changed)
		cb->updateData(&cbData);
}

REGISTER_IMGUI_WINDOW("Sky parameters", [] { wr->getSky().gui(); });