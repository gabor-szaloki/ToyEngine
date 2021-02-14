#pragma once

#include "RendererCommon.h"

#include <array>
#include <vector>
#include <string>

class ITexture;

struct MaterialTexturePaths
{
	std::string albedo;
	std::string normal;
	std::string roughness;
	std::string metalness;
	// TODO: std::string opacity;
};

struct MaterialTexture
{
	enum class Purpose { COLOR, NORMAL, OTHER, _COUNT, };

	ITexture* tex = nullptr;
	Purpose purpose = Purpose::COLOR;
};

class Material
{
public:
	Material(const std::string& name_, const std::array<ResId, (int)RenderPass::_COUNT>& shaders_);
	const std::array<std::vector<MaterialTexture>, (int)ShaderStage::GRAPHICS_STAGE_COUNT>& getTextures() { return textures; }
	void setTexture(ShaderStage stage, unsigned int slot, ITexture* tex, MaterialTexture::Purpose purpose = MaterialTexture::Purpose::COLOR);
	void set(RenderPass render_pass);

	std::string name;

private:
	std::array<ResId, (int)RenderPass::_COUNT> shaders;
	std::array<std::vector<MaterialTexture>, (int)ShaderStage::GRAPHICS_STAGE_COUNT> textures;
};
