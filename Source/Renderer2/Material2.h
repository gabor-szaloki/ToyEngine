#pragma once

#include "RendererCommon.h"
#include <array>
#include <vector>

class ITexture;

namespace renderer
{
	class Material
	{
	public:
		Material(const std::array<ResId, (int)RenderPass::_COUNT>& shaders_);
		void setTextures(ShaderStage stage, ITexture** textures_, unsigned int num_textures);
		void set(RenderPass render_pass);

	private:
		std::array<ResId, (int)RenderPass::_COUNT> shaders;
		std::array<std::vector<ITexture*>, (int)ShaderStage::GRAPHICS_STAGE_COUNT> textures;
	};
}