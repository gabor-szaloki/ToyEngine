#pragma once

#include <memory>

#include <Common.h>
#include <Util/ResIdHolder.h>

class ITexture;

class EnvironmentLightingSystem
{
public:
	EnvironmentLightingSystem();
	void bake(const ITexture* envi_cube);
	ITexture* getIrradianceCube() const { return irradianceCube.get(); };

private:
	std::unique_ptr<ITexture> irradianceCube;
	ResIdHolder irradianceBakeShader;
	ResIdHolder linearSampler;
};

