#include "Material.h"

#include <assert.h>
#include <Driver/ITexture.h>
#include <Renderer/WorldRenderer.h>
#include "AssetManager.h"

Material::Material(const std::string& name_, const std::array<ResId, (int)RenderPass::_COUNT>& shaders_) : name(name_)
{
	for (int i = 0; i < (int)RenderPass::_COUNT; i++)
	{
		shaders[i] = shaders_[i];
		currentVariants[i] = 0;
	}
	for (const std::string& kw : am->getGlobalShaderKeywords())
		setKeyword(kw, true);
}

void Material::setTexture(ShaderStage stage, unsigned int slot, ITexture* tex, MaterialTexture::Purpose purpose)
{
	std::vector<MaterialTexture>& stageTextures = textures[(int)stage];
	if (stageTextures.size() <= slot)
		stageTextures.resize(slot + 1);
	stageTextures[slot].tex = tex;
	stageTextures[slot].purpose = purpose;
}

void Material::setKeyword(const std::string& keyword, bool enable)
{
	auto updateCurrentVariants = [&]
	{
		std::vector<const char*> keywordCstrs;
		keywordCstrs.resize(keywords.size());
		for (int i = 0; i < keywords.size(); i++)
			keywordCstrs[i] = keywords[i].c_str();
		for (int i = 0; i < (int)RenderPass::_COUNT; i++)
			currentVariants[i] = drv->getShaderVariantIndexForKeywords(shaders[i], keywordCstrs.data(), keywordCstrs.size());
	};

	auto it = std::find(keywords.begin(), keywords.end(), keyword);
	if (it == keywords.end() && enable)
	{
		keywords.push_back(keyword);
		updateCurrentVariants();
	}
	else if (it != keywords.end() && !enable)
	{
		keywords.erase(it);
		updateCurrentVariants();
	}
}

void Material::set(RenderPass render_pass)
{
	drv->setShader(shaders[(int)render_pass], currentVariants[(int)render_pass]);
	for (int stage = 0; stage < (int)ShaderStage::GRAPHICS_STAGE_COUNT; stage++)
	{
		for (unsigned int i = 0; i < textures[stage].size(); i++)
		{
			ITexture* tex = textures[stage][i].tex;
			if (tex != nullptr && tex->isStub())
				tex = am->getDefaultTexture(textures[stage][i].purpose);
			ResId texId = tex != nullptr ? tex->getId() : BAD_RESID;
			drv->setTexture((ShaderStage)stage, i, texId);
			drv->setSampler((ShaderStage)stage, i, am->getDefaultMaterialTextureSampler());
		}
	}
}