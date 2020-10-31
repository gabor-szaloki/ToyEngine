#include "Material.h"

#include <assert.h>
#include <Driver/ITexture.h>

#include "WorldRenderer.h"

Material::Material(const std::array<ResId, (int)RenderPass::_COUNT>& shaders_)
{
	for (int i = 0; i < (int)RenderPass::_COUNT; i++)
		shaders[i] = shaders_[i];
}

void Material::setTexture(ShaderStage stage, unsigned int slot, ITexture* tex, MaterialTexture::Purpose purpose)
{
	std::vector<MaterialTexture>& stageTextures = textures[(int)stage];
	if (stageTextures.size() <= slot)
		stageTextures.resize(slot + 1);
	stageTextures[slot].tex = tex;
	stageTextures[slot].purpose = purpose;
}

void Material::set(RenderPass render_pass)
{
	drv->setShaderSet(shaders[(int)render_pass]);
	for (int stage = 0; stage < (int)ShaderStage::GRAPHICS_STAGE_COUNT; stage++)
	{
		for (unsigned int i = 0; i < textures[stage].size(); i++)
		{
			ITexture* tex = textures[stage][i].tex;
			if (tex != nullptr && tex->isStub())
				tex = wr->defaultTextures[(int)textures[stage][i].purpose];
			ResId texId = tex != nullptr ? tex->getId() : BAD_RESID;
			drv->setTexture((ShaderStage)stage, i, texId, true);
		}
	}
}