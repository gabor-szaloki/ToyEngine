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

	{
		// Vertex data for a colored cube.
		struct VertexPosColor
		{
			XMFLOAT3 Position;
			XMFLOAT3 Color;
		};

		VertexPosColor cubeVertices[8] =
		{
			{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
			{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
			{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
			{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
			{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
			{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
			{ XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
			{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
		};

		BufferDesc cubeVbDesc("DemoCubeVb", sizeof(VertexPosColor), _countof(cubeVertices), ResourceUsage::DEFAULT, BIND_VERTEX_BUFFER);
		cubeVbDesc.initialData = cubeVertices;
		cubeVb.reset(drv->createBuffer(cubeVbDesc));

		uint cubeIndices[36] =
		{
			0, 1, 2, 0, 2, 3,
			4, 6, 5, 4, 7, 6,
			4, 5, 1, 4, 1, 0,
			3, 2, 6, 3, 6, 7,
			1, 5, 6, 1, 6, 2,
			4, 0, 3, 4, 3, 7
		};

		BufferDesc cubeIbDesc("DemoCubeIb", sizeof(uint), _countof(cubeIndices), ResourceUsage::DEFAULT, BIND_INDEX_BUFFER);
		cubeIbDesc.initialData = cubeIndices;
		cubeIb.reset(drv->createBuffer(cubeIbDesc));
	}

	initResolutionDependentResources();
}

void D3D12Test::onResize(int display_width, int display_height)
{
	closeResolutionDependentResources();

	displayWidth = display_width;
	displayHeight = display_height;

	initResolutionDependentResources();
}

void D3D12Test::render(ITexture& target)
{
	drv->setView(0, 0, displayWidth, displayHeight, 0, 1);
	drv->setShader(shaderSet, 0);
	drv->setRenderState(renderState);
	drv->setInputLayout(inputLayout);
	drv->setVertexBuffer(cubeVb->getId());
	drv->setIndexBuffer(cubeIb->getId());
	drv->setRenderTarget(drv->getBackbufferTexture()->getId(), depthTex->getId());

	drv->clearRenderTargets(RenderTargetClearParams::clear_all(0.4f, 0.6f, 0.9f, 1.0f, 1.0f));

	drv->drawIndexed(36, 0, 0);
}

void D3D12Test::initResolutionDependentResources()
{
	TextureDesc depthTexDesc("depthTex", displayWidth, displayHeight,
		TexFmt::R24G8_TYPELESS, 1, ResourceUsage::DEFAULT, BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE);
	depthTexDesc.srvFormatOverride = TexFmt::R24_UNORM_X8_TYPELESS;
	depthTexDesc.dsvFormatOverride = TexFmt::D24_UNORM_S8_UINT;
	depthTex.reset(drv->createTexture(depthTexDesc));
}

void D3D12Test::closeResolutionDependentResources()
{
	depthTex.reset();
}
