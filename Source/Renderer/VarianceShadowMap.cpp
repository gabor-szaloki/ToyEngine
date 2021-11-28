#include "VarianceShadowMap.h"
#include <Driver/ITexture.h>

VarianceShadowMap::VarianceShadowMap(unsigned int shadow_resolution)
{
	ShaderSetDesc shaderDesc("PostFx", "Source/Shaders/VarianceShadowMap.shader");
	shaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "DefaultPostFxVsFunc";
	shaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "VarianceShadowMapPS";
	shader = drv->createShaderSet(shaderDesc);

	RenderStateDesc rsDesc;
	rsDesc.depthStencilDesc.depthTest = false;
	rsDesc.depthStencilDesc.depthWrite = false;
	renderState = drv->createRenderState(rsDesc);

	sampler = drv->createSampler(SamplerDesc(FILTER_MIN_MAG_MIP_POINT, TexAddr::CLAMP));

	TextureDesc vsmDesc("varianceShadowMap", shadow_resolution, shadow_resolution,
		TexFmt::R32G32_FLOAT, 1, ResourceUsage::DEFAULT, BIND_SHADER_RESOURCE | BIND_RENDER_TARGET);
	texture.reset(drv->createTexture(vsmDesc));
}

void VarianceShadowMap::render(ITexture* source_shadow_map)
{
	PROFILE_SCOPE("VarianceShadowMap");

	drv->setRenderTarget(texture->getId(), BAD_RESID);
	drv->setTexture(ShaderStage::PS, 0, source_shadow_map->getId());
	drv->setSampler(ShaderStage::PS, 0, sampler);

	drv->setShader(shader, 0);
	drv->setRenderState(renderState);

	drv->draw(3, 0);
}

void VarianceShadowMap::clear()
{
	PROFILE_SCOPE("VarianceShadowMap_Clear");
	drv->setRenderTarget(texture->getId(), BAD_RESID);
	drv->clearRenderTargets(RenderTargetClearParams::clear_color(1, 1, 1, 1));
	drv->setRenderTarget(BAD_RESID, BAD_RESID);
}
