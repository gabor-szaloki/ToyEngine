#include "SlimeSim.h"

#include <Driver/IDriver.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>
#include <Renderer/RenderUtil.h>
#include <3rdParty/ImGui/imgui.h>
#include <vector>

SlimeSim::SlimeSim(int display_width, int display_height)
{
	cbData.numAgents = 300;
	cbData.agentMoveSpeed = 50.0f;
	cbData.deltaTime = 0;
	cbData.evaporateSpeed = 0.15f;
	cbData.diffuseSpeed = 20.f;

	BufferDesc cbDesc;
	cbDesc.bindFlags = BIND_CONSTANT_BUFFER;
	cbDesc.numElements = 1;
	cbDesc.name = "SlimeSimCb";
	cbDesc.elementByteSize = sizeof(cbData);
	cb.reset(drv->createBuffer(cbDesc));

	ComputeShaderDesc shaderDesc;
	shaderDesc.sourceFilePath = "Source/Shaders/Experiments/SlimeSim.shader";
	shaderDesc.name = "SlimeSimClear";
	shaderDesc.shaderFuncName = "Clear";
	clearShader = drv->createComputeShader(shaderDesc);
	shaderDesc.name = "SlimeSimUpdate";
	shaderDesc.shaderFuncName = "Update";
	updateShader = drv->createComputeShader(shaderDesc);
	shaderDesc.name = "SlimeSimPostProcess";
	shaderDesc.shaderFuncName = "PostProcess";
	postProcessShader = drv->createComputeShader(shaderDesc);

	initResolutionDependentResources(display_width, display_height);

	BufferDesc agentBufferDesc;
	agentBufferDesc.bindFlags = BIND_UNORDERED_ACCESS;
	agentBufferDesc.miscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	agentBufferDesc.numElements = cbData.numAgents;
	agentBufferDesc.name = "SlimeAgentsBuffer";
	agentBufferDesc.elementByteSize = sizeof(Agent);
	agentBuffer.reset(drv->createBuffer(agentBufferDesc));

	fillAgentBuffer();
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

	cb->updateData(&cbData);
	drv->setConstantBuffer(ShaderStage::CS, 0, cb->getId());

	drv->setRwBuffer(0, agentBuffer->getId());
	drv->setRwTexture(1, slimeMaps[slimeMapIndex]->getId());
	drv->setRwTexture(2, slimeMaps[postProcessedSlimeMapIndex]->getId());

	{
		PROFILE_SCOPE("Update");
		drv->setShader(updateShader, 0);
		drv->dispatch((cbData.numAgents + (64 - 1)) / 64, 1, 1);
	}
	{
		PROFILE_SCOPE("PostProcess");
		drv->setShader(postProcessShader, 0);
		drv->dispatch((width + 16 - 1) / 16, (height + 16 - 1) / 16, 1);
	}
	// Cleanup
	drv->setRwTexture(1, BAD_RESID);
	drv->setRwTexture(2, BAD_RESID);

	// Blit to final target
	// TODO: figure out why it looks bad if we do the SRGB conversion o.O
	render_util::blit(*slimeMaps[postProcessedSlimeMapIndex], target, false);
	//render_util::blit_linear_to_srgb(*slimeMaps[postProcessedSlimeMapIndex], target, false);

	// Swap targets
	std::swap(slimeMapIndex, postProcessedSlimeMapIndex);
}

void SlimeSim::gui()
{
	ImGui::Begin("Slime Simulation");

	if (ImGui::Button("Reset"))
		reset();

	ImGui::End();
}

void SlimeSim::initResolutionDependentResources(int display_width, int display_height)
{
	width = display_width / 4;
	height = display_height / 4;
	cbData.width = width;
	cbData.height = height;

	TextureDesc slimeMapDesc("slimeMap_0", width, height, TexFmt::R16G16B16A16_UNORM);
	slimeMapDesc.bindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
	slimeMaps[0].reset(drv->createTexture(slimeMapDesc));
	slimeMapDesc.name = "slimeMap_1";
	slimeMaps[1].reset(drv->createTexture(slimeMapDesc));

	clear();
}

void SlimeSim::closeResolutionDependentResources()
{
	for (auto& sm : slimeMaps)
		sm.reset();
}

void SlimeSim::fillAgentBuffer()
{
	std::vector<Agent> agentsInitialData;
	agentsInitialData.resize(cbData.numAgents);
	for (unsigned int i = 0; i < cbData.numAgents; i++)
	{
		agentsInitialData[i].position = XMFLOAT2(random_01() * width, random_01() * height);
		agentsInitialData[i].angle = random_01() * XM_2PI;
	}
	agentBuffer->updateData(agentsInitialData.data());
}

void SlimeSim::clear()
{
	drv->setShader(clearShader, 0);
	drv->setRwTexture(1, slimeMaps[slimeMapIndex]->getId());
	drv->setRwTexture(2, slimeMaps[postProcessedSlimeMapIndex]->getId());
	drv->dispatch((width + 16 - 1) / 16, (height + 16 - 1) / 16, 1);
	drv->setRwTexture(1, BAD_RESID);
	drv->setRwTexture(2, BAD_RESID);
}

void SlimeSim::reset()
{
	clear();
	fillAgentBuffer();
}