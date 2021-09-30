#include "ClothSim.h"

#include <Driver/IDriver.h>
#include <Driver/ITexture.h>

ClothSim::ClothSim(int display_width, int display_height)
{
	initResolutionDependentResources(display_width, display_height);

	ComputeShaderDesc simShaderDesc("ClothSimComputeShader", "Source/Shaders/Experiments/ClothSim.shader", "ClothSimCS");
	simShader = drv->createComputeShader(simShaderDesc);
}

void ClothSim::onResize(int display_width, int display_height)
{
	closeResolutionDependentResources();
	initResolutionDependentResources(display_width, display_height);
}

void ClothSim::update(float delta_time)
{
}

void ClothSim::render(ITexture& target)
{
	PROFILE_SCOPE("ClothSim");

	drv->setRwTexture(0, simTarget->getId());
	drv->setShader(simShader, 0);

	drv->dispatch((width + (8-1)) / 8, (height + (8-1)) / 8, 1);

	// Cleanup
	drv->setRwTexture(0, BAD_RESID);

	// Copy to final target
	simTarget->copyResource(target.getId());
}

void ClothSim::initResolutionDependentResources(int display_width, int display_height)
{
	width = display_width;
	height = display_height;

	TextureDesc simTargetDesc("clothSimTarget", display_width, display_height);
	simTargetDesc.bindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
	simTarget.reset(drv->createTexture(simTargetDesc));
}

void ClothSim::closeResolutionDependentResources()
{
	simTarget.reset();
}