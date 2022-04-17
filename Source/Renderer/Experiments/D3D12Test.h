#pragma once

#include "IFullscreenExperiment.h"

#include <memory>
#include <Common.h>
#include <Util/ResIdHolder.h>

class ITexture;

class D3D12Test : public IFullscreenExperiment
{
public:
	D3D12Test(int display_width, int display_height);
	virtual void onResize(int display_width, int display_height) override;
	virtual void render(class ITexture& target) override;

private:
	int displayWidth, displayHeight;
	ResIdHolder shaderSet;
};