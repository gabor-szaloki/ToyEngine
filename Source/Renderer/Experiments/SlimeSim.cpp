#include "SlimeSim.h"

#include <Driver/IDriver.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>

#include <vector>

SlimeSim::SlimeSim(int display_width, int display_height)
{
	cbData.numAgents = 100;
	cbData.agentMoveSpeed = 100.0f;
	cbData.deltaTime = 0;

	initResolutionDependentResources(display_width, display_height);

	BufferDesc cbDesc;
	cbDesc.bindFlags = BIND_CONSTANT_BUFFER;
	cbDesc.numElements = 1;
	cbDesc.name = "SlimeSimCb";
	cbDesc.elementByteSize = sizeof(cbData);
	cb.reset(drv->createBuffer(cbDesc));

	BufferDesc agentBufferDesc;
	agentBufferDesc.bindFlags = BIND_UNORDERED_ACCESS;
	agentBufferDesc.miscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	agentBufferDesc.numElements = cbData.numAgents;
	agentBufferDesc.name = "SlimeAgentsBuffer";
	agentBufferDesc.elementByteSize = sizeof(Agent);
	agentBuffer.reset(drv->createBuffer(agentBufferDesc));

	std::vector<Agent> agentsInitialData;
	agentsInitialData.resize(cbData.numAgents);
	for (unsigned int i = 0; i < cbData.numAgents; i++)
	{
		agentsInitialData[i].position = XMFLOAT2(random_01() * width, random_01() * height);
		agentsInitialData[i].angle = random_01() * XM_2PI;
	}
	agentBuffer->updateData(agentsInitialData.data());

	ComputeShaderDesc shaderDesc("SlimeSimComputeShader", "Source/Shaders/Experiments/SlimeSim.shader", "Update");
	slimeSimShader = drv->createComputeShader(shaderDesc);
}

void SlimeSim::onResize(int display_width, int display_height)
{
	closeResolutionDependentResources();
	initResolutionDependentResources(display_width, display_height);
}

void SlimeSim::update(float delta_time)
{
	cbData.deltaTime = delta_time;
}

void SlimeSim::render(ITexture& target)
{
	PROFILE_SCOPE("SlimeSim");

	drv->setShader(slimeSimShader, 0);

	cb->updateData(&cbData);
	drv->setConstantBuffer(ShaderStage::CS, 0, cb->getId());

	drv->setRwBuffer(0, agentBuffer->getId());
	drv->setRwTexture(1, slimeMap->getId());

	drv->dispatch((cbData.numAgents + (64-1)) / 64, 1, 1);

	// Cleanup
	drv->setRwTexture(0, BAD_RESID);

	// Copy to final target
	slimeMap->copyResource(target.getId());
}

void SlimeSim::initResolutionDependentResources(int display_width, int display_height)
{
	width = display_width;
	height = display_height;
	cbData.width = width;
	cbData.height = height;

	TextureDesc slimeMapDesc("slimeMap", display_width, display_height);
	slimeMapDesc.bindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
	slimeMap.reset(drv->createTexture(slimeMapDesc));
}

void SlimeSim::closeResolutionDependentResources()
{
	slimeMap.reset();
}