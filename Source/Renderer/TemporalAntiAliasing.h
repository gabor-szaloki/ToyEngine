#pragma once

#include <memory>

#include <Common.h>
#include <Util/ResIdHolder.h>

class TemporalAntiAliasing
{
public:
	TemporalAntiAliasing();
	XMFLOAT2 getJitter(unsigned int frame) const;
	void perform(const ITexture& source_tex, const ITexture& dest_tex, const ITexture& history_tex);
	void gui();

private:
	bool enabled = false;
	int jitterFrameMod = 8;
};