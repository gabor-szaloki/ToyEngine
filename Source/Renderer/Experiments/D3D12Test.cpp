#include "SlimeSim.h"

#include <Driver/IDriver.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>
#include <Renderer/RenderUtil.h>
#include <3rdParty/ImGui/imgui.h>
#include <vector>
#include "D3D12Test.h"

D3D12Test::D3D12Test(int display_width, int display_height) : displayWidth(display_width), displayHeight(display_height)
{
	ShaderSetDesc shaderDesc("D3D12TestCube", "Source/Shaders/Experiments/D3D12Test.shader");
	shaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "VsMain";
	shaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "PsMain";
	shaderSet = drv->createShaderSet(shaderDesc);

	InputLayoutElementDesc standardInputLayoutDesc[2] =
	{
		{ VertexInputSemantic::POSITION, 0, TexFmt::R32G32B32_FLOAT },
		{ VertexInputSemantic::COLOR,    0, TexFmt::R32G32B32_FLOAT },
	};
	inputLayout = drv->createInputLayout(standardInputLayoutDesc, _countof(standardInputLayoutDesc), shaderSet);

	RenderStateDesc rsDesc;
	//rsDesc.rasterizerDesc.wireframe = true;
	renderState = drv->createRenderState(rsDesc);
}

void D3D12Test::onResize(int display_width, int display_height)
{
	displayWidth = display_width;
	displayHeight = display_height;
}

void D3D12Test::render(ITexture& target)
{
	drv->setView(0, 0, displayWidth, displayHeight, 0, 1);
	drv->setShader(shaderSet, 0);
	drv->setInputLayout(inputLayout);
	drv->setRenderState(renderState);
}