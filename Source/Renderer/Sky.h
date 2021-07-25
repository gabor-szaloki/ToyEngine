#pragma once

#include <memory>

#include <Common.h>
#include <Util/ResIdHolder.h>

struct SkyCbData
{
	XMFLOAT4 topColor_Exponent = XMFLOAT4(0.12f, 0.20f, 0.36f, 5.0f);
	XMFLOAT4 bottomColor_Exponent = XMFLOAT4(0.04f, 0.04f, 0.04f, 50.0f);
	XMFLOAT4 horizonColor = XMFLOAT4(0.77f, 0.91f, 1.f, 0.f);
	XMFLOAT4 skyIntensity_SunIntensity_SunAlpha_SunBeta = XMFLOAT4(1.0f, 2.0f, 4000.f, 1.f);
};

class Sky
{
public:
	Sky();
	bool isDirty() const { return dirty; }
	void markDirty() { dirty = true; }
	void bakeProcedural();
	void bakeFromPanoramicTexture(const ITexture* panoramic_environment_map);
	ITexture* getBakedCube() const { return bakedCubeMap.get(); }
	void render(const ITexture* sky_cube_override = nullptr);
	void gui();

private:
	void setWorldRendererAmbientLighting();
	void bakeInternal(const ITexture* panoramic_environment_map);

	bool enabled = true;
	bool dirty = true;
	ResIdHolder proceduralBakeShader, textureBakeShader, renderShader;
	SkyCbData bakeCbData;
	std::unique_ptr<IBuffer> bakeCb;
	std::unique_ptr<ITexture> bakedCubeMap;
	ResIdHolder linearSampler;
	ResIdHolder renderState;
};