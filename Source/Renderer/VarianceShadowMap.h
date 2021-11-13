#pragma once

#include <memory>
#include <Common.h>
#include <Util/ResIdHolder.h>

class ITexture;

class VarianceShadowMap
{
public:
	VarianceShadowMap(unsigned int shadow_resolution);
	void render(ITexture* source_shadow_map);
	ITexture* getTexture() const { return texture.get(); }

private:
	ResIdHolder shader;
	ResIdHolder renderState;
	ResIdHolder sampler;
	std::unique_ptr<ITexture> texture;
};