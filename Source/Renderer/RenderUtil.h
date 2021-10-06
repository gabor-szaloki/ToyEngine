#pragma once

class ITexture;

namespace render_util
{
	void init();
	void shutdown();

	void blit(const ITexture& src_tex, const ITexture& dst_tex, bool bilinear = true);
	void blit_linear_to_srgb(const ITexture& src_tex, const ITexture& dst_tex, bool bilinear = true);
}