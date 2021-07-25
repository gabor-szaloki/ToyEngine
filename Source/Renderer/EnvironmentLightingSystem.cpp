#include "EnvironmentLightingSystem.h"

#include <Driver/IDriver.h>
#include <Driver/ITexture.h>
#include "CubeRenderHelper.h"

EnvironmentLightingSystem::EnvironmentLightingSystem()
{
	ShaderSetDesc irradianceBakeShaderDesc("IrradianceBake", "Source/Shaders/IrradianceBake.shader");
	irradianceBakeShaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "DefaultPostFxVsFunc";
	irradianceBakeShaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "IrradianceBakePS";
	irradianceBakeShader = drv->createShaderSet(irradianceBakeShaderDesc);

	TextureDesc irradianceCubeDesc("IrradianceCube", 32, 32, TexFmt::R16G16B16A16_FLOAT);
	irradianceCubeDesc.bindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
	irradianceCubeDesc.miscFlags = RESOURCE_MISC_TEXTURECUBE;
	irradianceCube.reset(drv->createTexture(irradianceCubeDesc));

	linearSampler = drv->createSampler(SamplerDesc(FILTER_MIN_MAG_MIP_LINEAR));
}

void EnvironmentLightingSystem::bake(const ITexture* envi_cube)
{
	PROFILE_SCOPE("EnvironmentLightingSystemBake");

	drv->setShader(irradianceBakeShader, 0);
	drv->setTexture(ShaderStage::PS, 0, envi_cube->getId());
	drv->setSampler(ShaderStage::PS, 0, linearSampler);

	CubeRenderHelper cubeRenderHelper;
	cubeRenderHelper.beginRender(irradianceCube.get());
	cubeRenderHelper.renderAllFaces();
	cubeRenderHelper.finishRender();

	drv->setTexture(ShaderStage::PS, 0, BAD_RESID);
	drv->setSampler(ShaderStage::PS, 0, BAD_RESID);
}
