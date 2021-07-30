#pragma once

#include <memory>

#include <Common.h>
#include <Util/ResIdHolder.h>

class ITexture;
class IBuffer;

struct EnvironmentBakeResources
{
	ResId irradianceBakeShader;
	ResId specularBakeShader;
	ResId linearSampler;
};

class EnvironmentProbe
{
public:
	EnvironmentProbe(const XMVECTOR& pos);
	void renderEnvironmentCube(ITexture* cube_target);
	void bake(const EnvironmentBakeResources& bake_resources, const ITexture* envi_cube, float radiance_cutoff);
	ITexture* getIrradianceCube() const { return irradianceCube.get(); };
	ITexture* getSpecularCube() const { return specularCube.get(); };
	const XMVECTOR& getPosition() const { return position; }
	static const unsigned int SPECULAR_CUBE_MIPS = 5;

private:
	struct SpecularBakeCbData
	{
		XMFLOAT4 _SourceCubeWidth_InvSourceCubeWidth_Roughness_RadianceCutoff;
	} bakeCbData;

	std::unique_ptr<ITexture> irradianceCube, specularCube;
	std::unique_ptr<IBuffer> bakeCb;
	XMVECTOR position;
};

