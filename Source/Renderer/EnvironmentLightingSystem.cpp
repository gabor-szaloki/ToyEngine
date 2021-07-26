#include "EnvironmentLightingSystem.h"

#include <Driver/IDriver.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>
#include "CubeRenderHelper.h"

EnvironmentLightingSystem::EnvironmentLightingSystem()
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

	TextureDesc irradianceCubeDesc("IrradianceCube", 32, 32, TexFmt::R16G16B16A16_FLOAT);
	irradianceCubeDesc.bindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
	irradianceCubeDesc.miscFlags = RESOURCE_MISC_TEXTURECUBE;
	irradianceCube.reset(drv->createTexture(irradianceCubeDesc));

	TextureDesc specularCubeDesc("SpecularCube", 128, 128, TexFmt::R16G16B16A16_FLOAT, SPECULAR_CUBE_MIPS);
	specularCubeDesc.bindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
	specularCubeDesc.miscFlags = RESOURCE_MISC_TEXTURECUBE;
	specularCube.reset(drv->createTexture(specularCubeDesc));

	TextureDesc brdfLutDesc("BRDFLUT", 512, 512, TexFmt::R16G16_FLOAT);
	brdfLutDesc.bindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
	brdfLut.reset(drv->createTexture(brdfLutDesc));
	isBrdfLutBaked = false;

	bakeCbData._SourceCubeWidth_InvSourceCubeWidth_Roughness_RadianceCutoff = XMFLOAT4(2048.f, 1.f / 2048.f, 0.f, 99999.f);

	BufferDesc bakeCbDesc;
	bakeCbDesc.bindFlags = BIND_CONSTANT_BUFFER;
	bakeCbDesc.numElements = 1;
	bakeCbDesc.name = "SpecularBakeCb";
	bakeCbDesc.elementByteSize = sizeof(bakeCbData);
	bakeCb.reset(drv->createBuffer(bakeCbDesc));
	bakeCb->updateData(&bakeCbData);

	linearSampler = drv->createSampler(SamplerDesc(FILTER_MIN_MAG_MIP_LINEAR));
}

float EnvironmentLightingSystem::getEnvironmentRadianceCutoff() const
{
	return bakeCbData._SourceCubeWidth_InvSourceCubeWidth_Roughness_RadianceCutoff.w;
}

void EnvironmentLightingSystem::setEnvironmentRadianceCutoff(float radiance_cutoff)
{
	bakeCbData._SourceCubeWidth_InvSourceCubeWidth_Roughness_RadianceCutoff.w = radiance_cutoff < 0 ? 99999.f : radiance_cutoff;
	markDirty();
}

void EnvironmentLightingSystem::bake(const ITexture* envi_cube)
{
	PROFILE_SCOPE("EnvironmentLightingSystemBake");

	drv->setTexture(ShaderStage::PS, 0, envi_cube->getId());
	drv->setSampler(ShaderStage::PS, 0, linearSampler);
	drv->setConstantBuffer(ShaderStage::PS, 3, bakeCb->getId());
	bakeCb->updateData(&bakeCbData);

	{
		PROFILE_SCOPE("Irradiance");

		drv->setShader(irradianceBakeShader, 0);

		CubeRenderHelper cubeRenderHelper;
		cubeRenderHelper.beginRender(irradianceCube.get());
		cubeRenderHelper.renderAllFaces();
		cubeRenderHelper.finishRender();
	}
	{
		PROFILE_SCOPE("Specular");

		drv->setShader(specularBakeShader, 0);

		float sourceCubeWidth = envi_cube->getDesc().width;
		bakeCbData._SourceCubeWidth_InvSourceCubeWidth_Roughness_RadianceCutoff.x = sourceCubeWidth;
		bakeCbData._SourceCubeWidth_InvSourceCubeWidth_Roughness_RadianceCutoff.y = 1.f / sourceCubeWidth;

		CubeRenderHelper cubeRenderHelper;
		for (unsigned int mip = 0; mip < SPECULAR_CUBE_MIPS; mip++)
		{
			#if PROFILE_MARKERS_ENABLED
				std::string profileLabel = "Mip";
				profileLabel += std::to_string(mip);
				PROFILE_SCOPE(profileLabel.c_str());
			#endif

			bakeCbData._SourceCubeWidth_InvSourceCubeWidth_Roughness_RadianceCutoff.z = (float)mip / (SPECULAR_CUBE_MIPS - 1);
			bakeCb->updateData(&bakeCbData);

			cubeRenderHelper.beginRender(specularCube.get(), mip);
			cubeRenderHelper.renderAllFaces();
			cubeRenderHelper.finishRender();
		}
	}

	if (!isBrdfLutBaked)
	{
		PROFILE_SCOPE("IntegrateBrdf");

		drv->setShader(integrateBrdfShader, 0);
		drv->setRenderTarget(brdfLut->getId(), BAD_RESID);
		drv->setView(0, 0, brdfLut->getDesc().width, brdfLut->getDesc().height, 0, 1);
		drv->draw(3, 0);

		isBrdfLutBaked = true;
	}

	drv->setRenderTarget(BAD_RESID, BAD_RESID);
	drv->setTexture(ShaderStage::PS, 0, BAD_RESID);
	drv->setSampler(ShaderStage::PS, 0, BAD_RESID);

	dirty = false;
}
