#pragma once

#include <memory>

#include <Common.h>
#include <Util/ResIdHolder.h>

class ITexture;
class IBuffer;

class EnvironmentLightingSystem
{
public:
	EnvironmentLightingSystem();
	void bake(const ITexture* envi_cube);
	ITexture* getIrradianceCube() const { return irradianceCube.get(); };
	ITexture* getSpecularCube() const { return specularCube.get(); };
	ITexture* getBrdfLut() const { return brdfLut.get(); };
	static const unsigned int SPECULAR_CUBE_MIPS = 5;

private:
	struct SpecularBakeCbData
	{
		XMFLOAT4 _SourceCubeWidth_InvSourceCubeWidth_Roughness_RadianceCutoff;
	} bakeCbData;

	std::unique_ptr<ITexture> irradianceCube, specularCube, brdfLut;
	std::unique_ptr<IBuffer> bakeCb;
	ResIdHolder irradianceBakeShader, specularBakeShader, integrateBrdfShader;
	ResIdHolder linearSampler;
	bool isBrdfLutBaked;
};

