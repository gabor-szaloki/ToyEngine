#include "WorldRenderer.h"

#include <Renderer/Camera.h>
#include <Renderer/Light.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>

#include "ConstantBuffers.h"

WorldRenderer::WorldRenderer()
{
	camera = std::make_unique<Camera>();
	camera->SetEye(XMVectorSet(2.0f, 3.5f, -3.0f, 0.0f));
	camera->SetRotation(XM_PI / 6, -XM_PIDIV4);

	mainLight = std::make_unique<Light>();
	mainLight->SetRotation(-XM_PI / 3, XM_PI / 3);

	RenderStateDesc forwaredRsDesc;
	forwardRenderStateId = drv->createRenderState(forwaredRsDesc);
	forwaredRsDesc.rasterizerDesc.wireframe = true;
	forwardWireframeRenderStateId = drv->createRenderState(forwaredRsDesc);

	BufferDesc cbDesc;
	cbDesc.bindFlags = BIND_CONSTANT_BUFFER;
	cbDesc.numElements = 1;
	cbDesc.name = "perFrameCb";
	cbDesc.elementByteSize = sizeof(PerFrameConstantBufferData);
	perFrameCb.reset(drv->createBuffer(cbDesc));
	cbDesc.name = "perCameraCb";
	cbDesc.elementByteSize = sizeof(PerCameraConstantBufferData);
	perCameraCb.reset(drv->createBuffer(cbDesc));
	cbDesc.name = "perObjectCb";
	cbDesc.elementByteSize = sizeof(PerObjectConstantBufferData);
	perObjectCb.reset(drv->createBuffer(cbDesc));

	ShaderSetDesc shaderDesc("Source\\Shaders\\Standard.shader");
	shaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "StandardForwardVS";
	shaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "StandardOpaqueForwardPS";
	standardShaderId = drv->createShaderSet(shaderDesc);
	shaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "StandardShadowVS";
	shaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "StandardOpaqueShadowPS";
	standardShadowShaderId = drv->createShaderSet(shaderDesc);

	setShadowResolution(2048);
	setShadowBias(50000, 1.0f);
	initResolutionDependentResources();
}

WorldRenderer::~WorldRenderer()
{
}

void WorldRenderer::onResize()
{
	closeResolutionDependentResources();
	initResolutionDependentResources();
}

void WorldRenderer::update(float delta_time)
{
	// TODO
}

void WorldRenderer::render()
{
	// TODO
}

void WorldRenderer::setShadowResolution(unsigned int shadow_resolution)
{
	TextureDesc shadowMapDesc("shadowMap", shadow_resolution, shadow_resolution,
		TexFmt::R32_TYPELESS, 1, ResourceUsage::DEFAULT, BIND_SHADER_RESOURCE|BIND_DEPTH_STENCIL);
	shadowMapDesc.srvFormatOverride = TexFmt::R32_FLOAT;
	shadowMapDesc.dsvFormatOverride = TexFmt::D32_FLOAT;
	shadowMapDesc.samplerDesc.filter = FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	shadowMapDesc.samplerDesc.comparisonFunc = ComparisonFunc::LESS_EQUAL;
	shadowMapDesc.samplerDesc.setAddressMode(TexAddr::BORDER);
	shadowMap.reset(drv->createTexture(shadowMapDesc));
}

void WorldRenderer::setShadowBias(int depth_bias, float slope_scaled_depth_bias)
{
	RenderStateDesc desc;
	desc.rasterizerDesc.depthBias = depth_bias;
	desc.rasterizerDesc.slopeScaledDepthBias = slope_scaled_depth_bias;
	shadowRenderStateId = drv->createRenderState(desc);
}

void WorldRenderer::initResolutionDependentResources()
{
	glm::ivec2 displayResolution;
	drv->getDisplaySize(displayResolution.x, displayResolution.y);
	TextureDesc depthTexDesc("depthTex", displayResolution.x, displayResolution.y,
		TexFmt::D32_FLOAT_S8X24_UINT, 1, ResourceUsage::DEFAULT, BIND_DEPTH_STENCIL);
	depthTex.reset(drv->createTexture(depthTexDesc));
}

void WorldRenderer::closeResolutionDependentResources()
{
	depthTex.reset();
}
