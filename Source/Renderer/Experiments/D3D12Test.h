#pragma once

#include "IFullscreenExperiment.h"

#include <memory>
#include <Common.h>
#include <Util/ResIdHolder.h>

class D3D12Test : public IFullscreenExperiment
{
public:
	D3D12Test(int display_width, int display_height);
	virtual void onResize(int display_width, int display_height) override;
	virtual void render(class ITexture& target) override;

private:
	void initResolutionDependentResources();
	void closeResolutionDependentResources();

	int displayWidth, displayHeight;
	std::unique_ptr<ITexture> depthTex;
	std::unique_ptr<IBuffer> cubeVb, cubeIb;
	ResIdHolder shaderSet;
	ResIdHolder inputLayout;
	ResIdHolder renderState;
};