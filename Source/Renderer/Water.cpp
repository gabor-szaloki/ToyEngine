#include "Water.h"

#include <vector>

#include <3rdParty/ImGui/imgui.h>
#include <Util/AutoImGui.h>
#include <Driver/IBuffer.h>
#include <Driver/ITexture.h>
#include <Engine/AssetManager.h>
#include "WorldRenderer.h"
#include "ConstantBuffers.h"

Water::Water(const Transform& transform_) : transform(transform_)
{
	ShaderSetDesc shaderDesc("Water", "Source/Shaders/Water.shader");
	shaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "WaterVS";
	shaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "WaterPS";
	shader = drv->createShaderSet(shaderDesc);

	RenderStateDesc rsDesc;
	rsDesc.depthStencilDesc.depthWrite = false;
	rsDesc.depthStencilDesc.depthWrite = false;
	renderState = drv->createRenderState(rsDesc);

	cbData.worldTransform = transform.getMatrix();
	cbData.albedo = XMFLOAT3(0.03, 0.05, 0.06);
	cbData.roughness = 0.05;
	cbData.normalStrength1 = 0.2;
	cbData.normalTiling1 = 0.7;
	cbData.normalAnimSpeed1 = XMFLOAT2(-0.18, 0.2);
	cbData.normalStrength2 = 0.2;
	cbData.normalTiling2 = 0.8;
	cbData.normalAnimSpeed2 = XMFLOAT2(0.15, -0.06);

	BufferDesc cbDesc;
	cbDesc.bindFlags = BIND_CONSTANT_BUFFER;
	cbDesc.numElements = 1;
	cbDesc.name = "HbaoCb";
	cbDesc.elementByteSize = sizeof(cbData);
	cb.reset(drv->createBuffer(cbDesc));
	cb->updateData(&cbData);

	normalMap.reset(am->loadTexture("Assets/Textures/Water/water_normal_01.png", false));
	wrapSampler = drv->createSampler(SamplerDesc());

	onGlobalShaderKeywordsChanged();
}

void Water::onGlobalShaderKeywordsChanged()
{
	const std::vector<std::string>& globalKeywords = am->getGlobalShaderKeywords();
	std::vector<const char*> globalKeywordsCstr(globalKeywords.size());
	std::transform(globalKeywords.begin(), globalKeywords.end(), globalKeywordsCstr.begin(), [](const std::string& kw) { return kw.c_str(); });

	currentVariant = drv->getShaderVariantIndexForKeywords(shader, globalKeywordsCstr.data(), globalKeywordsCstr.size());
}

void Water::render()
{
	PROFILE_SCOPE("Water");

	drv->setShader(shader, currentVariant);
	drv->setRenderState(renderState);
	drv->setConstantBuffer(ShaderStage::VS, 4, cb->getId());
	drv->setConstantBuffer(ShaderStage::PS, 4, cb->getId());

	ResId normalTexId = normalMap->isStub() ? am->getDefaultTexture(MaterialTexture::Purpose::NORMAL)->getId() : normalMap->getId();
	drv->setTexture(ShaderStage::PS, 0, normalTexId);
	drv->setSampler(ShaderStage::PS, 0, wrapSampler);

	drv->draw(6, 0);

	drv->setTexture(ShaderStage::PS, 0, BAD_RESID);
	drv->setSampler(ShaderStage::PS, 0, BAD_RESID);
}

void Water::gui()
{
	bool changed = false;

	changed |= ImGui::ColorEdit3("Albedo", &cbData.albedo.x);
	changed |= ImGui::SliderFloat("Roughness", &cbData.roughness, 0, 1);

	ImGui::TextUnformatted("1st normal layer");
	changed |= ImGui::SliderFloat("Strength##Normal1", &cbData.normalStrength1, 0, 1);
	changed |= ImGui::SliderFloat("Tiling##Normal1", &cbData.normalTiling1, 0, 1);
	changed |= ImGui::SliderFloat2("Anim Speed##Normal1", &cbData.normalAnimSpeed1.x, -1, 1);

	ImGui::TextUnformatted("2nd normal layer");
	changed |= ImGui::SliderFloat("Strength##Normal2", &cbData.normalStrength2, 0, 1);
	changed |= ImGui::SliderFloat("Tiling##Normal2", &cbData.normalTiling2, 0, 1);
	changed |= ImGui::SliderFloat2("Anim Speed##Normal2", &cbData.normalAnimSpeed2.x, -1, 1);

	if (changed)
		cb->updateData(&cbData);
}

static void gui_helper()
{
	if (wr->getWater())
		wr->getWater()->gui();
	else
		ImGui::TextUnformatted("No water on this scene");
}

REGISTER_IMGUI_WINDOW("Water parameters", gui_helper);