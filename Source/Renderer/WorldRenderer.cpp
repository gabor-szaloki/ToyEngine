#include "WorldRenderer.h"

#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>
#include <Engine/AssetManager.h>
#include <Engine/MeshRenderer.h>

#include "ConstantBuffers.h"
#include "Light.h"
#include "Hbao.h"
#include "Sky.h"

WorldRenderer::WorldRenderer()
{
	mainLight = std::make_unique<Light>();
	mainLight->SetRotation(-XM_PI / 3, XM_PI / 3);

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

	RenderStateDesc depthPrepassRsDesc;
	depthPrepassRenderStateId = drv->createRenderState(depthPrepassRsDesc);
	depthPrepassRsDesc.rasterizerDesc.wireframe = true;
	depthPrepassWireframeRenderStateId = drv->createRenderState(depthPrepassRsDesc);

	RenderStateDesc forwardRsDesc;
	forwardRsDesc.depthStencilDesc.depthWrite = false;
	forwardRsDesc.depthStencilDesc.DepthFunc = ComparisonFunc::EQUAL;
	forwardRenderStateId = drv->createRenderState(forwardRsDesc);
	forwardRsDesc.rasterizerDesc.wireframe = true;
	forwardWireframeRenderStateId = drv->createRenderState(forwardRsDesc);

	am->setGlobalShaderKeyword("SOFT_SHADOWS_TENT", true);
	setShadowResolution(2048);
	setShadowBias(50000, 1.0f);
}

WorldRenderer::~WorldRenderer()
{
}

void WorldRenderer::init()
{
	initResolutionDependentResources();
	sky = std::make_unique<Sky>();
}

void WorldRenderer::onResize(int display_width, int display_height)
{
	int w, h;
	drv->getDisplaySize(w, h);
	if (display_width == w && display_height == h)
		return;
	closeResolutionDependentResources();
	drv->resize(display_width, display_height);
	initResolutionDependentResources();
}

void WorldRenderer::update(float delta_time)
{
	// Update time
	time += delta_time;

	// Update camera movement
	XMVECTOR cameraMoveDir =
		camera.GetRight() * ((cameraInputState.isMovingRight ? 1.0f : 0.0f) + (cameraInputState.isMovingLeft ? -1.0f : 0.0f)) +
		camera.GetUp() * ((cameraInputState.isMovingUp ? 1.0f : 0.0f) + (cameraInputState.isMovingDown ? -1.0f : 0.0f)) +
		camera.GetForward() * ((cameraInputState.isMovingForward ? 1.0f : 0.0f) + (cameraInputState.isMovingBackward ? -1.0f : 0.0f));
	XMVECTOR cameraMoveDelta = cameraMoveDir * cameraMoveSpeed * delta_time;
	if (cameraInputState.isSpeeding)
		cameraMoveDelta *= 2.0f;
	camera.MoveEye(cameraMoveDelta);

	// Update camera rotation
	camera.Rotate(cameraInputState.deltaPitch * cameraTurnSpeed, cameraInputState.deltaYaw * cameraTurnSpeed);
	cameraInputState.deltaPitch = cameraInputState.deltaYaw = 0;
}

static XMFLOAT4 get_final_light_color(const XMFLOAT4& color, float intensity)
{
	return XMFLOAT4(color.x * intensity, color.y * intensity, color.z * intensity, 1.0f);
}

static XMMATRIX get_shadow_matrix(XMMATRIX& lightView, XMMATRIX& lightProj)
{
	XMMATRIX worldToLightSpace = XMMatrixMultiply(lightView, lightProj);

	//XMMATRIX textureScaleBias = XMMatrixMultiply(XMMatrixScaling(0.5f, -0.5f, 1.0f), XMMatrixTranslation(0.5f, 0.5f, 0.0f));
	const XMMATRIX textureScaleBias = XMMatrixSet(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	return XMMatrixMultiply(worldToLightSpace, textureScaleBias);
}

void WorldRenderer::render()
{
	PROFILE_SCOPE("RenderWorld");

	XMMATRIX lightViewMatrix;
	XMMATRIX lightProjectionMatrix;
	setupFrame(lightViewMatrix, lightProjectionMatrix);

	setupShadowPass(lightViewMatrix, lightProjectionMatrix);
	performShadowPass();

	setupDepthAndForwardPasses();
	performDepthPrepass();

	ssao->perform();

	performForwardPass();

	sky->render();

	drv->setRenderTarget(drv->getBackbufferTexture()->getId(), BAD_RESID);
	drv->setTexture(ShaderStage::PS, 0, hdrTarget->getId(), true);
	postFx.perform();

	drv->setTexture(ShaderStage::PS, 0, BAD_RESID, true);
}

void WorldRenderer::setAmbientLighting(const XMFLOAT4& bottom_color, const XMFLOAT4& top_color, float intensity)
{
	ambientLightBottomColor = bottom_color;
	ambientLightTopColor = top_color;
	ambientLightIntensity = intensity;
}

unsigned int WorldRenderer::getShadowResolution()
{
	if (shadowMap == nullptr)
		return 0;
	return shadowMap->getDesc().width;
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
	shadowDepthBias = depth_bias;
	shadowSlopeScaledDepthBias = slope_scaled_depth_bias;

	RenderStateDesc desc;
	desc.rasterizerDesc.depthBias = shadowDepthBias;
	desc.rasterizerDesc.slopeScaledDepthBias = shadowSlopeScaledDepthBias;
	desc.rasterizerDesc.depthClipEnable = false;
	shadowRenderStateId = drv->createRenderState(desc);
}

void WorldRenderer::initResolutionDependentResources()
{
	XMINT2 displayResolution;
	drv->getDisplaySize(displayResolution.x, displayResolution.y);

	TextureDesc hdrTargetDesc("hdrTarget", displayResolution.x, displayResolution.y,
		TexFmt::R16G16B16A16_FLOAT, 1, ResourceUsage::DEFAULT, BIND_RENDER_TARGET | BIND_SHADER_RESOURCE);
	hdrTarget.reset(drv->createTexture(hdrTargetDesc));

	TextureDesc depthTexDesc("depthTex", displayResolution.x, displayResolution.y,
		TexFmt::R24G8_TYPELESS, 1, ResourceUsage::DEFAULT, BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE);
	depthTexDesc.srvFormatOverride = TexFmt::R24_UNORM_X8_TYPELESS;
	depthTexDesc.dsvFormatOverride = TexFmt::D24_UNORM_S8_UINT;
	depthTexDesc.samplerDesc.filter = FILTER_MIN_MAG_MIP_POINT;
	depthTex.reset(drv->createTexture(depthTexDesc));

	camera.Resize((float)displayResolution.x, (float)displayResolution.y);

	XMINT2 ssaoResolution(displayResolution.x * ssaoResolutionScale, displayResolution.y * ssaoResolutionScale);
	ssao = std::make_unique<Hbao>(ssaoResolution);
}

void WorldRenderer::closeResolutionDependentResources()
{
	depthTex.reset();
	hdrTarget.reset();
	ssao.reset();
}

XMVECTOR calculate_shadow_camera_pos(const Camera& camera, float shadow_distance)
{
	return camera.GetEye() + camera.GetForward() * shadow_distance * 0.5f; // TODO: align to texel
}

void WorldRenderer::setupFrame(XMMATRIX& out_light_view_matrix, XMMATRIX& out_light_proj_matrix)
{
	PerFrameConstantBufferData perFrameCbData;
	perFrameCbData.ambientLightBottomColor = get_final_light_color(ambientLightBottomColor, ambientLightIntensity);
	perFrameCbData.ambientLightTopColor = get_final_light_color(ambientLightTopColor, ambientLightIntensity);
	perFrameCbData.mainLightColor = get_final_light_color(mainLight->GetColor(), mainLight->GetIntensity());
	XMMATRIX mainLightTranslateMatrix = XMMatrixTranslationFromVector(calculate_shadow_camera_pos(camera, shadowDistance));
	XMMATRIX inverseLightViewMatrix = XMMatrixRotationRollPitchYaw(mainLight->GetPitch(), mainLight->GetYaw(), 0.0f) * mainLightTranslateMatrix;
	XMStoreFloat4(&perFrameCbData.mainLightDirection, inverseLightViewMatrix.r[2]);
	out_light_view_matrix = XMMatrixInverse(nullptr, inverseLightViewMatrix);
	out_light_proj_matrix = XMMatrixOrthographicLH(shadowDistance, shadowDistance, -directionalShadowDistance, directionalShadowDistance);
	perFrameCbData.mainLightShadowMatrix = get_shadow_matrix(out_light_view_matrix, out_light_proj_matrix);
	const float shadowResolution = (float)shadowMap->getDesc().width;
	const float invShadowResolution = 1.0f / shadowResolution;
	perFrameCbData.mainLightShadowParams = XMFLOAT4(shadowResolution, invShadowResolution, 2 * directionalShadowDistance, poissonShadowSoftness);
	perFrameCbData.timeParams = XMFLOAT4(time, 0, 0, 0);
	perFrameCb->updateData(&perFrameCbData);

	drv->setConstantBuffer(ShaderStage::VS, 0, perFrameCb->getId());
	drv->setConstantBuffer(ShaderStage::PS, 0, perFrameCb->getId());
}

static XMFLOAT4 get_projection_params(float zn, float zf, bool persp)
{
	return XMFLOAT4(zn * zf, zn - zf, zf, persp ? 1.0f : 0.0f);
}

void WorldRenderer::setupShadowPass(const XMMATRIX& light_view_matrix, const XMMATRIX& light_proj_matrix)
{
	// Shadow camera
	PerCameraConstantBufferData perCameraCbData{};
	perCameraCbData.view = light_view_matrix;
	perCameraCbData.projection = light_proj_matrix;
	perCameraCbData.viewProjection = light_view_matrix * light_proj_matrix;
	perCameraCbData.projectionParams = get_projection_params(-directionalShadowDistance, directionalShadowDistance, false);
	XMStoreFloat4(&perCameraCbData.cameraWorldPosition, calculate_shadow_camera_pos(camera, shadowDistance));
	float shadowResolution = getShadowResolution();
	perCameraCbData.viewportResolution = XMFLOAT4(shadowResolution, shadowResolution, 1.f / shadowResolution, 1.f / shadowResolution);
	perCameraCb->updateData(&perCameraCbData);
	drv->setConstantBuffer(ShaderStage::VS, 1, perCameraCb->getId());
	drv->setConstantBuffer(ShaderStage::PS, 1, perCameraCb->getId());

	// Setup state and render target
	drv->setRenderState(shadowRenderStateId);
	drv->setRenderTarget(BAD_RESID, shadowMap->getId());
	drv->setView(0, 0, (float)shadowMap->getDesc().width, (float)shadowMap->getDesc().height, 0, 1);
	RenderTargetClearParams clearParams(CLEAR_FLAG_DEPTH, 0, 1.0f);
	drv->clearRenderTargets(clearParams);
}

void WorldRenderer::setupDepthAndForwardPasses()
{
	// Main camera
	{
		PerCameraConstantBufferData perCameraCbData;
		perCameraCbData.view = camera.GetViewMatrix();
		perCameraCbData.projection = camera.GetProjectionMatrix();
		perCameraCbData.viewProjection = perCameraCbData.view * perCameraCbData.projection;
		perCameraCbData.projectionParams = get_projection_params(camera.GetNearPlane(), camera.GetFarPlane(), true);
		XMStoreFloat4(&perCameraCbData.cameraWorldPosition, camera.GetEye());
		perCameraCbData.viewportResolution = XMFLOAT4(
			camera.GetViewportWidth(), camera.GetViewportHeight(),
			1.f / camera.GetViewportWidth(), 1.f / camera.GetViewportHeight());

		XMMATRIX viewRotMatrix = perCameraCbData.view;
		viewRotMatrix.r[3] = XMVectorSet(0, 0, 0, 1);
		XMMATRIX viewRotProjMatrix = viewRotMatrix * perCameraCbData.projection;
		XMMATRIX invViewRotProjMatrix = XMMatrixInverse(nullptr, viewRotProjMatrix);
		float invFarPlane = 1.0f / camera.GetFarPlane();
		XMStoreFloat4(&perCameraCbData.viewVecLT, XMVector3TransformCoord(XMVectorSet(-1.f, +1.f, 1.f, 0.f), invViewRotProjMatrix) * invFarPlane);
		XMStoreFloat4(&perCameraCbData.viewVecRT, XMVector3TransformCoord(XMVectorSet(+1.f, +1.f, 1.f, 0.f), invViewRotProjMatrix) * invFarPlane);
		XMStoreFloat4(&perCameraCbData.viewVecLB, XMVector3TransformCoord(XMVectorSet(-1.f, -1.f, 1.f, 0.f), invViewRotProjMatrix) * invFarPlane);
		XMStoreFloat4(&perCameraCbData.viewVecRB, XMVector3TransformCoord(XMVectorSet(+1.f, -1.f, 1.f, 0.f), invViewRotProjMatrix) * invFarPlane);

		perCameraCb->updateData(&perCameraCbData);

		drv->setConstantBuffer(ShaderStage::VS, 1, perCameraCb->getId());
		drv->setConstantBuffer(ShaderStage::PS, 1, perCameraCb->getId());
	}

	// Clear targets
	{
		const TextureDesc& targetDesc = hdrTarget->getDesc();
		drv->setRenderTarget(hdrTarget->getId(), depthTex->getId());
		drv->setView(0, 0, (float)targetDesc.width, (float)targetDesc.height, 0, 1);

		drv->clearRenderTargets(RenderTargetClearParams::clear_all(0.0f, 0.2f, 0.4f, 1.0f, 1.0f));
	}
}

void WorldRenderer::performShadowPass()
{
	PROFILE_SCOPE("ShadowPass");

	if (!shadowEnabled)
		return;

	for (MeshRenderer* mr : am->getSceneMeshRenderers())
		mr->render(RenderPass::DEPTH);
}

void WorldRenderer::performDepthPrepass()
{
	PROFILE_SCOPE("DepthPrepass");

	drv->setRenderState(showWireframe ? depthPrepassWireframeRenderStateId : depthPrepassRenderStateId);
	drv->setRenderTarget(BAD_RESID, depthTex->getId());

	for (MeshRenderer* mr : am->getSceneMeshRenderers())
		mr->render(RenderPass::DEPTH);
}

void WorldRenderer::performForwardPass()
{
	PROFILE_SCOPE("ForwardPass");

	drv->setRenderState(showWireframe ? forwardWireframeRenderStateId : forwardRenderStateId);
	drv->setRenderTarget(hdrTarget->getId(), depthTex->getId());

	drv->setTexture(ShaderStage::PS, 2, shadowMap->getId(), true);
	drv->setTexture(ShaderStage::PS, 3, ssao->getResultTex()->getId(), true);

	for (MeshRenderer* mr : am->getSceneMeshRenderers())
		mr->render(RenderPass::FORWARD);

	drv->setTexture(ShaderStage::PS, 2, BAD_RESID, true);
	drv->setTexture(ShaderStage::PS, 3, BAD_RESID, true);
}
