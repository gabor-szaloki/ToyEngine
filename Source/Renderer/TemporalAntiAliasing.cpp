#include "TemporalAntiAliasing.h"

#include <vector>
#include <3rdParty/imgui/imgui.h>
#include <Util/AutoImGui.h>
#include <Driver/ITexture.h>
#include "WorldRenderer.h"

static float halton(const unsigned int base, unsigned int index)
{
	// https://en.wikipedia.org/wiki/Halton_sequence
	float f = 1;
	float r = 0;
	while (index > 0)
	{
		f /= (float)base;
		r += f * (index % base);
		index = (unsigned int)floorf(index / base);
	}
	return r;
}

TemporalAntiAliasing::TemporalAntiAliasing()
{
}

XMFLOAT2 TemporalAntiAliasing::getJitter(unsigned int frame) const
{
	unsigned int haltonFrame = frame % jitterFrameMod + 1; // Halton sequence generator should be indexed from 1
	return enabled ? XMFLOAT2(halton(2, haltonFrame) - 0.5f, halton(3, haltonFrame) - 0.5f) : XMFLOAT2(0.f, 0.f);
}

void TemporalAntiAliasing::perform(const ITexture& source_tex, const ITexture& dest_tex, const ITexture& history_tex)
{
	PROFILE_SCOPE("TAA");

	source_tex.copyResource(dest_tex.getId());
}

void TemporalAntiAliasing::gui()
{
	ImGui::Checkbox("Enabled", &enabled);
}

REGISTER_IMGUI_WINDOW("TAA settings", [] { wr->getTaa().gui(); });