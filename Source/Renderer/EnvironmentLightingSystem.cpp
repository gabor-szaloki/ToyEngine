#include "EnvironmentLightingSystem.h"

#include <Driver/IDriver.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>
#include "CubeRenderHelper.h"

EnvironmentLightingSystem::EnvironmentLightingSystem() : skyProbe(XMVectorZero())
{
	ShaderSetDesc bakeShaderDesc("IrradianceBake", "Source/Shaders/EnvironmentLighting.shader");
	bakeShaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "DefaultPostFxVsFunc";
	bakeShaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "IrradianceBakePS";
	irradianceBakeShader = drv->createShaderSet(bakeShaderDesc);
	bakeShaderDesc.name = "SpecularBake";
	bakeShaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "SpecularBakePS";
	specularBakeShader = drv->createShaderSet(bakeShaderDesc);
	bakeShaderDesc.name = "IntegrateBrdf";
	bakeShaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "IntegrateBrdfPS";
	integrateBrdfShader = drv->createShaderSet(bakeShaderDesc);

	linearSampler = drv->createSampler(SamplerDesc(FILTER_MIN_MAG_MIP_LINEAR));

	TextureDesc brdfLutDesc("BRDFLUT", 512, 512, TexFmt::R16G16_FLOAT);
	brdfLutDesc.bindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
	brdfLut.reset(drv->createTexture(brdfLutDesc));

	{
		PROFILE_SCOPE("IntegrateBrdf");

		drv->setShader(integrateBrdfShader, 0);
		drv->setRenderTarget(brdfLut->getId(), BAD_RESID);
		drv->setView(0, 0, brdfLut->getDesc().width, brdfLut->getDesc().height, 0, 1);
		drv->draw(3, 0);
		drv->setRenderTarget(BAD_RESID, BAD_RESID);
	}
}

void EnvironmentLightingSystem::bake(const ITexture* envi_cube)
{
	PROFILE_SCOPE("EnvironmentLightingSystem_Bake");

	EnvironmentBakeResources bakeResources{ irradianceBakeShader, specularBakeShader, linearSampler };
	skyProbe.bake(bakeResources, envi_cube, radianceCutoff);

	if (worldProbe)
	{
		TextureDesc worldEnviCubeDesc("SpecularCube", 128, 128, TexFmt::R16G16B16A16_FLOAT, 1);
		worldEnviCubeDesc.bindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
		worldEnviCubeDesc.miscFlags = RESOURCE_MISC_TEXTURECUBE;
		std::unique_ptr<ITexture> worldEnviCube;
		worldEnviCube.reset(drv->createTexture(worldEnviCubeDesc));

		worldProbe->renderEnvironmentCube(worldEnviCube.get());
		worldProbe->bake(bakeResources, worldEnviCube.get(), radianceCutoff);
	}

	dirty = false;
}

void EnvironmentLightingSystem::setWorldProbe(bool enabled, const XMVECTOR& pos)
{
	worldProbe.reset(enabled ? new EnvironmentProbe(pos) : nullptr);
}

ITexture* EnvironmentLightingSystem::getIrradianceCube() const
{
	return (worldProbe.get() != nullptr && !dirty) ? worldProbe->getIrradianceCube() : skyProbe.getIrradianceCube();
}

ITexture* EnvironmentLightingSystem::getSpecularCube() const
{
	return (worldProbe.get() != nullptr && !dirty) ? worldProbe->getSpecularCube() : skyProbe.getSpecularCube();
}
