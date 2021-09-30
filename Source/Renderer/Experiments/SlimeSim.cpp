#include "SlimeSim.h"

#include <Driver/IDriver.h>
#include <Driver/ITexture.h>

SlimeSim::SlimeSim(int display_width, int display_height)
{
	initResolutionDependentResources(display_width, display_height);

	ComputeShaderDesc simShaderDesc("SlimeSimComputeShader", "Source/Shaders/Experiments/SlimeSim.shader", "SlimeSimCS");
	simShader = drv->createComputeShader(simShaderDesc);
}

void SlimeSim::onResize(int display_width, int display_height)
{
	closeResolutionDependentResources();
	initResolutionDependentResources(display_width, display_height);
}

void SlimeSim::update(float delta_time)
{
}

void SlimeSim::render(ITexture& target)
{
	PROFILE_SCOPE("SlimeSim");

	drv->setRwTexture(0, simTarget->getId());
	drv->setShader(simShader, 0);

	drv->dispatch((width + (8-1)) / 8, (height + (8-1)) / 8, 1);

	// Cleanup
	drv->setRwTexture(0, BAD_RESID);

	// Copy to final target
	simTarget->copyResource(target.getId());
}

void SlimeSim::initResolutionDependentResources(int display_width, int display_height)
{
	width = display_width;
	height = display_height;

	TextureDesc simTargetDesc("slimeSimTarget", display_width, display_height);
	simTargetDesc.bindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
	simTarget.reset(drv->createTexture(simTargetDesc));
}

void SlimeSim::closeResolutionDependentResources()
{
	simTarget.reset();
}