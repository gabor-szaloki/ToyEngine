#pragma once

#include "IFullscreenExperiment.h"

#include <memory>
#include <Common.h>
#include <Util/ResIdHolder.h>

class ITexture;

class SlimeSim : public IFullscreenExperiment
{
public:
	SlimeSim(int display_width, int display_height);
	virtual void onResize(int display_width, int display_height) override;
	virtual void update(float delta_time) override;
	virtual void render(class ITexture& target) override;

private:
	void initResolutionDependentResources(int display_width, int display_height);
	void closeResolutionDependentResources();

	int width = 0, height = 0;
	std::unique_ptr<ITexture> simTarget;
	ResIdHolder simShader;
};