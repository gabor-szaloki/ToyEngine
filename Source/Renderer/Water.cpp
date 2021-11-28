#include "Water.h"
#include "ConstantBuffers.h"
#include <Driver/IBuffer.h>
#include <Engine/AssetManager.h>
#include <vector>

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

	BufferDesc cbDesc;
	cbDesc.bindFlags = BIND_CONSTANT_BUFFER;
	cbDesc.numElements = 1;
	cbDesc.name = "HbaoCb";
	cbDesc.elementByteSize = sizeof(cbData);
	cb.reset(drv->createBuffer(cbDesc));
	cb->updateData(&cbData);

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
	drv->setConstantBuffer(ShaderStage::VS, PER_OBJECT_CONSTANT_BUFFER_SLOT, cb->getId());
	drv->setConstantBuffer(ShaderStage::PS, PER_OBJECT_CONSTANT_BUFFER_SLOT, cb->getId());

	drv->draw(6, 0);
}
