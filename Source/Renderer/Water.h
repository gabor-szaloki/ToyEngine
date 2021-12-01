#pragma once

#include <memory>

#include <Common.h>
#include <Util/ResIdHolder.h>
#include <Engine/Transform.h>

class IBuffer;
class ITexture;

struct WaterCbData
{
	XMMATRIX worldTransform;

	XMFLOAT3 fogColor;
	float fogDensity;

	float roughness;
	float refractionStrength;
	int numRefractionIterations;
	float pad;

	float normalStrength1;
	float normalTiling1;
	XMFLOAT2 normalAnimSpeed1;

	float normalStrength2;
	float normalTiling2;
	XMFLOAT2 normalAnimSpeed2;
};

class Water
{
public:
	Water(const Transform& transform_);
	void onGlobalShaderKeywordsChanged();
	void render(const ITexture* scene_grab_texture, const ITexture* depth_copy_texture);
	void gui();

private:
	Transform transform;

	ResIdHolder shader;
	unsigned int currentVariant = 0;

	ResIdHolder renderState;

	WaterCbData cbData;
	std::unique_ptr<IBuffer> cb;

	std::unique_ptr<ITexture> normalMap;
	ResIdHolder wrapSampler, clampSampler, pointClampSampler;
};