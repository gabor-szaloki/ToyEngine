#include "Material2.h"

#include <assert.h>

#include <Driver/ITexture.h>

using namespace renderer;

Material::Material(const std::array<ResId, (int)RenderPass::_COUNT>& shaders_)
{
	for (int i = 0; i < (int)RenderPass::_COUNT; i++)
		shaders[i] = shaders_[i];
}

void renderer::Material::setTextures(ShaderStage stage, ITexture** textures_, unsigned int num_textures)
{
	assert(textures_ != nullptr);
	textures[(int)stage].assign(textures_, textures_ + num_textures);
}

void Material::set(RenderPass render_pass)
{
	drv->setShaderSet(shaders[(int)render_pass]);
	for (int stage = 0; stage < (int)ShaderStage::GRAPHICS_STAGE_COUNT; stage++)
	{
		for (unsigned int i = 0; i < textures[stage].size(); i++)
		{
			ITexture* tex = textures[stage][i];
			ResId texId = tex != nullptr ? tex->getId() : BAD_RESID;
			drv->setTexture((ShaderStage)stage, i, textures[stage][i]->getId(), true);
		}
	}
}