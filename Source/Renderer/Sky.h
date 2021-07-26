#pragma once

#include <memory>

#include <Common.h>
#include <Util/ResIdHolder.h>

class Sky
{
public:
	Sky();
	bool isDirty() const { return dirty; }
	void markDirty() { dirty = true; isBakedFromTexture = false; }
	void bakeProcedural();
	void bakeFromPanoramicTexture(const ITexture* panoramic_environment_map);
	ITexture* getBakedCube() const { return bakedCubeMap.get(); }
	void render(const ITexture* sky_cube_override = nullptr, float mip_override = 0);
	void gui();

private:
	void bakeInternal(const ITexture* panoramic_environment_map);

	struct SkyBakeCbData
	{
		XMFLOAT4 topColor_Exponent = XMFLOAT4(0.12f, 0.20f, 0.36f, 5.0f);
		XMFLOAT4 bottomColor_Exponent = XMFLOAT4(0.04f, 0.04f, 0.04f, 50.0f);
		XMFLOAT4 horizonColor = XMFLOAT4(0.77f, 0.91f, 1.f, 0.f);
		XMFLOAT4 skyIntensity_SunIntensity_SunAlpha_SunBeta = XMFLOAT4(1.0f, 2.0f, 4000.f, 1.f);
	} bakeCbData;

	struct SkyRenderCbData
	{
		XMFLOAT4 mip = XMFLOAT4(0, 0, 0, 0);
	} renderCbData;

	bool enabled = true;
	bool dirty = true;
	bool isBakedFromTexture = false;
	ResIdHolder proceduralBakeShader, textureBakeShader, renderShader;
	std::unique_ptr<IBuffer> bakeCb, renderCb;
	std::unique_ptr<ITexture> bakedCubeMap;
	ResIdHolder linearSampler;
	ResIdHolder renderState;
};