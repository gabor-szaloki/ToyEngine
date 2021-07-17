#pragma once

#include <array>
#include <vector>
#include <string>

#include <Common.h>
#include <Driver/IDriver.h>

struct MaterialTexturePaths
{
	std::string albedo;
	std::string opacity;
	std::string normal;
	std::string roughness;
	std::string metalness;
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
	void setConstants(const struct PerMaterialConstantBufferData& cb_data);
	void setTexture(ShaderStage stage, unsigned int slot, ITexture* tex, MaterialTexture::Purpose purpose = MaterialTexture::Purpose::COLOR);
	void setKeyword(const std::string& keyword, bool enable);
	void set(RenderPass render_pass);

	std::string name;

private:
	std::array<ResId, (int)RenderPass::_COUNT> shaders;
	std::array<unsigned int, (int)RenderPass::_COUNT> currentVariants;
	std::array<std::vector<MaterialTexture>, (int)ShaderStage::GRAPHICS_STAGE_COUNT> textures;
	std::vector<std::string> keywords;
	std::unique_ptr<IBuffer> cb;
};
