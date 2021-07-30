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
	void markDirty();
	float getEnvironmentRadianceCutoff() const { return radianceCutoff; }
	void setEnvironmentRadianceCutoff(float radiance_cutoff) { radianceCutoff = radiance_cutoff; markDirty(); }
	void setWorldProbe(bool enabled, const XMVECTOR& pos = XMVectorZero());
	bool hasWorldProbe() const { return worldProbe.get() != nullptr; }
	ITexture* getIrradianceCube() const { return currentIrradianceCube; };
	ITexture* getSpecularCube() const { return currentSpecularCube; };
	ITexture* getBrdfLut() const { return brdfLut.get(); };

private:
	EnvironmentProbe skyProbe;
	std::unique_ptr<EnvironmentProbe> worldProbe;
	std::unique_ptr<ITexture> brdfLut;
	ResIdHolder irradianceBakeShader, specularBakeShader, integrateBrdfShader;
	ResIdHolder linearSampler;
	ITexture* currentIrradianceCube;
	ITexture* currentSpecularCube;
	bool dirty = true;
	float radianceCutoff = -1;
};

