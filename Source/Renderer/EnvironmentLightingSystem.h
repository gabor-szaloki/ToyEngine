#pragma once

#include <memory>

#include <Common.h>
#include <Util/ResIdHolder.h>
#include "EnvironmentProbe.h"

class ITexture;
class IBuffer;

class EnvironmentLightingSystem
{
public:
	EnvironmentLightingSystem();
	void bake(const ITexture* envi_cube);
	bool isDirty() const { return dirty; }
	void markDirty() { dirty = true; }
	float getEnvironmentRadianceCutoff() const { return radianceCutoff; }
	void setEnvironmentRadianceCutoff(float radiance_cutoff) { radianceCutoff = radiance_cutoff; markDirty(); }
	ITexture* getIrradianceCube() const { return skyProbe.getIrradianceCube(); };
	ITexture* getSpecularCube() const { return skyProbe.getSpecularCube(); };
	ITexture* getBrdfLut() const { return brdfLut.get(); };

private:
	EnvironmentProbe skyProbe;
	std::unique_ptr<ITexture> brdfLut;
	ResIdHolder irradianceBakeShader, specularBakeShader, integrateBrdfShader;
	ResIdHolder linearSampler;
	bool dirty = true;
	float radianceCutoff = -1;
};

