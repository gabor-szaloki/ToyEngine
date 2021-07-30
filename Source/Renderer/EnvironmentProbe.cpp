#include "EnvironmentProbe.h"

#include <Driver/IDriver.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>
#include "WorldRenderer.h"
#include "CubeRenderHelper.h"

EnvironmentProbe::EnvironmentProbe(const XMVECTOR& pos) : position(pos)
{
	TextureDesc irradianceCubeDesc("IrradianceCube", 32, 32, TexFmt::R16G16B16A16_FLOAT);
	irradianceCubeDesc.bindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
	irradianceCubeDesc.miscFlags = RESOURCE_MISC_TEXTURECUBE;
	irradianceCube.reset(drv->createTexture(irradianceCubeDesc));

	TextureDesc specularCubeDesc("SpecularCube", 128, 128, TexFmt::R16G16B16A16_FLOAT, SPECULAR_CUBE_MIPS);
	specularCubeDesc.bindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
	specularCubeDesc.miscFlags = RESOURCE_MISC_TEXTURECUBE;
	specularCube.reset(drv->createTexture(specularCubeDesc));

	bakeCbData._SourceCubeWidth_InvSourceCubeWidth_Roughness_RadianceCutoff = XMFLOAT4(2048.f, 1.f / 2048.f, 0.f, 99999.f);

	BufferDesc bakeCbDesc;
	bakeCbDesc.bindFlags = BIND_CONSTANT_BUFFER;
	bakeCbDesc.numElements = 1;
	bakeCbDesc.name = "SpecularBakeCb";
	bakeCbDesc.elementByteSize = sizeof(bakeCbData);
	bakeCb.reset(drv->createBuffer(bakeCbDesc));
	bakeCb->updateData(&bakeCbData);
}

void EnvironmentProbe::renderEnvironmentCube(ITexture* cube_target)
{
	std::unique_ptr<ITexture> depthTex;
	unsigned int cubeFaceWidth = cube_target->getDesc().width;
	TextureDesc depthTexDesc("EnviCubeRenderDepth", cubeFaceWidth, cubeFaceWidth,
		TexFmt::R24G8_TYPELESS, 1, ResourceUsage::DEFAULT, BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE);
	depthTexDesc.srvFormatOverride = TexFmt::R24_UNORM_X8_TYPELESS;
	depthTexDesc.dsvFormatOverride = TexFmt::D24_UNORM_S8_UINT;
	depthTex.reset(drv->createTexture(depthTexDesc));

	CubeRenderHelper cubeRenderHelper;
	cubeRenderHelper.beginRender(position, cube_target);
	for (int i = 0; i < 6; i++)
	{
		cubeRenderHelper.setupCamera((CubeFace)i);
		wr->render(cubeRenderHelper.getCamera(), *cube_target, nullptr, *depthTex.get(), i, 0, 0, false);
	}

	drv->setRenderTarget(BAD_RESID, BAD_RESID);
}

void EnvironmentProbe::bake(const EnvironmentBakeResources& bake_resources, const ITexture* envi_cube, float radiance_cutoff)
{
	PROFILE_SCOPE("EnvironmentProbe_Bake");

	bakeCbData._SourceCubeWidth_InvSourceCubeWidth_Roughness_RadianceCutoff.w = radiance_cutoff < 0 ? 99999.f : radiance_cutoff;
	bakeCb->updateData(&bakeCbData);

	drv->setTexture(ShaderStage::PS, 0, envi_cube->getId());
	drv->setSampler(ShaderStage::PS, 0, bake_resources.linearSampler);
	drv->setConstantBuffer(ShaderStage::PS, 3, bakeCb->getId());

	{
		PROFILE_SCOPE("Irradiance");

		drv->setShader(bake_resources.irradianceBakeShader, 0);

		CubeRenderHelper cubeRenderHelper;
		cubeRenderHelper.beginRender(XMVectorZero(), irradianceCube.get());
		cubeRenderHelper.renderAllFaces();
		cubeRenderHelper.finishRender();
	}
	{
		PROFILE_SCOPE("Specular");

		drv->setShader(bake_resources.specularBakeShader, 0);

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

			cubeRenderHelper.beginRender(XMVectorZero(), specularCube.get(), mip);
			cubeRenderHelper.renderAllFaces();
			cubeRenderHelper.finishRender();
		}
	}

	drv->setRenderTarget(BAD_RESID, BAD_RESID);
	drv->setTexture(ShaderStage::PS, 0, BAD_RESID);
	drv->setSampler(ShaderStage::PS, 0, BAD_RESID);
}
