#include "WorldRenderer.h"

#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>
#include <Engine/AssetManager.h>
#include <Engine/MeshRenderer.h>

#include "ConstantBuffers.h"
#include "Light.h"
#include "Hbao.h"
#include "Sky.h"
#include "EnvironmentLightingSystem.h"

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

	linearWrapSampler = drv->createSampler(SamplerDesc(FILTER_MIN_MAG_MIP_LINEAR));
	linearClampSampler = drv->createSampler(SamplerDesc(FILTER_MIN_MAG_MIP_LINEAR, TexAddr::CLAMP));
	shadowSampler = drv->createSampler(SamplerDesc(FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, TexAddr::BORDER, ComparisonFunc::LESS_EQUAL));

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
	enviLightSystem = std::make_unique<EnvironmentLightingSystem>();
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

	if (sky->isDirty())
	{
		if (panoramicEnvironmentMap != nullptr && !panoramicEnvironmentMap->isStub())
		{
			sky->bakeFromPanoramicTexture(panoramicEnvironmentMap.get());
			panoramicEnvironmentMap.reset();
		}
		else
			sky->bakeProcedural();

		enviLightSystem->markDirty();
	}
	if (enviLightSystem->isDirty() || debugRecalculateEnvironmentLightingEveryFrame)
	{
		drv->setTexture(ShaderStage::PS, 10, BAD_RESID);
		drv->setTexture(ShaderStage::PS, 11, BAD_RESID);
		drv->setTexture(ShaderStage::PS, 12, BAD_RESID);

		enviLightSystem->bake(sky->getBakedCube());
	}

	// Set resources for lighting
	drv->setTexture(ShaderStage::PS, 10, enviLightSystem->getIrradianceCube()->getId());
	drv->setTexture(ShaderStage::PS, 11, enviLightSystem->getSpecularCube()->getId());
	drv->setTexture(ShaderStage::PS, 12, enviLightSystem->getBrdfLut()->getId());
	drv->setSampler(ShaderStage::PS, 10, linearClampSampler);

	XMVECTOR shadowCameraPos;
	XMMATRIX lightViewMatrix;
	XMMATRIX lightProjectionMatrix;
	setupFrame(shadowCameraPos, lightViewMatrix, lightProjectionMatrix);

	setupShadowPass(shadowCameraPos, lightViewMatrix, lightProjectionMatrix);
	performShadowPass();

	setupDepthAndForwardPasses();
	performDepthPrepass();

	ssao->perform();

	performForwardPass();

	if (debugShowSpecularMap)
		sky->render(enviLightSystem->getSpecularCube(), debugSpecularMapLod);
	else if (debugShowIrradianceMap)
		sky->render(enviLightSystem->getIrradianceCube());
	else
		sky->render();

	drv->setRenderTarget(drv->getBackbufferTexture()->getId(), BAD_RESID);
	drv->setTexture(ShaderStage::PS, 0, hdrTarget->getId());
	drv->setSampler(ShaderStage::PS, 0, linearClampSampler);
	postFx.perform();

	drv->setTexture(ShaderStage::PS, 0, BAD_RESID);
	drv->setSampler(ShaderStage::PS, 0, BAD_RESID);
}

void WorldRenderer::setEnvironment(ITexture* panoramic_environment_map, float radiance_cutoff)
{
	panoramicEnvironmentMap.reset(panoramic_environment_map);
	sky->markDirty();
	enviLightSystem->setEnvironmentRadianceCutoff(radiance_cutoff);
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

static XMFLOAT4 get_projection_params(float zn, float zf, bool persp)
{
	return XMFLOAT4(zn * zf, zn - zf, zf, persp ? 1.0f : 0.0f);
}

void WorldRenderer::setCameraForShaders(const Camera& cam)
{
	PerCameraConstantBufferData perCameraCbData;
	perCameraCbData.view = cam.GetViewMatrix();
	perCameraCbData.projection = cam.GetProjectionMatrix();
	perCameraCbData.viewProjection = perCameraCbData.view * perCameraCbData.projection;
	perCameraCbData.projectionParams = get_projection_params(cam.GetNearPlane(), cam.GetFarPlane(), true);
	XMStoreFloat4(&perCameraCbData.cameraWorldPosition, cam.GetEye());
	perCameraCbData.viewportResolution = XMFLOAT4(
		cam.GetViewportWidth(), cam.GetViewportHeight(),
		1.f / cam.GetViewportWidth(), 1.f / cam.GetViewportHeight());

	XMMATRIX viewRotMatrix = perCameraCbData.view;
	viewRotMatrix.r[3] = XMVectorSet(0, 0, 0, 1);
	XMMATRIX viewRotProjMatrix = viewRotMatrix * perCameraCbData.projection;
	XMMATRIX invViewRotProjMatrix = XMMatrixInverse(nullptr, viewRotProjMatrix);
	float invFarPlane = 1.0f / cam.GetFarPlane();
	XMStoreFloat4(&perCameraCbData.viewVecLT, XMVector3TransformCoord(XMVectorSet(-1.f, +1.f, 1.f, 0.f), invViewRotProjMatrix) * invFarPlane);
	XMStoreFloat4(&perCameraCbData.viewVecRT, XMVector3TransformCoord(XMVectorSet(+1.f, +1.f, 1.f, 0.f), invViewRotProjMatrix) * invFarPlane);
	XMStoreFloat4(&perCameraCbData.viewVecLB, XMVector3TransformCoord(XMVectorSet(-1.f, -1.f, 1.f, 0.f), invViewRotProjMatrix) * invFarPlane);
	XMStoreFloat4(&perCameraCbData.viewVecRB, XMVector3TransformCoord(XMVectorSet(+1.f, -1.f, 1.f, 0.f), invViewRotProjMatrix) * invFarPlane);

	perCameraCb->updateData(&perCameraCbData);

	drv->setConstantBuffer(ShaderStage::VS, 1, perCameraCb->getId());
	drv->setConstantBuffer(ShaderStage::PS, 1, perCameraCb->getId());
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

XMVECTOR calculate_shadow_camera_pos(const Camera& camera, const XMMATRIX& light_rotation_matrix, float shadow_distance, float shadow_resolution)
{
	XMVECTOR pos = camera.GetEye() + camera.GetForward() * shadow_distance * 0.5f;

	// Align to shadowmap texel
	XMMATRIX lightViewRotationMatrix = XMMatrixInverse(nullptr, light_rotation_matrix);
	XMVECTOR viewSpacePos = XMVector3TransformCoord(pos, lightViewRotationMatrix);
	float shadowTexelSize = shadow_distance / shadow_resolution;
	viewSpacePos = XMVectorSetX(viewSpacePos, roundf(XMVectorGetX(viewSpacePos) / shadowTexelSize) * shadowTexelSize);
	viewSpacePos = XMVectorSetY(viewSpacePos, roundf(XMVectorGetY(viewSpacePos) / shadowTexelSize) * shadowTexelSize);
	pos = XMVector3TransformCoord(viewSpacePos, light_rotation_matrix);

	return pos;
}

void WorldRenderer::setupFrame(XMVECTOR& out_shadow_camera_pos, XMMATRIX& out_light_view_matrix, XMMATRIX& out_light_proj_matrix)
{
	PerFrameConstantBufferData perFrameCbData;
	perFrameCbData.mainLightColor = get_final_light_color(mainLight->GetColor(), mainLightEnabled ? mainLight->GetIntensity() : 0.f);
	const float shadowResolution = (float)shadowMap->getDesc().width;
	XMMATRIX mainLightRotationMatrix = XMMatrixRotationRollPitchYaw(mainLight->GetPitch(), mainLight->GetYaw(), 0.0f);
	out_shadow_camera_pos = calculate_shadow_camera_pos(camera, mainLightRotationMatrix, shadowDistance, shadowResolution);
	XMMATRIX mainLightTranslateMatrix = XMMatrixTranslationFromVector(out_shadow_camera_pos);
	XMMATRIX inverseLightViewMatrix = mainLightRotationMatrix * mainLightTranslateMatrix;
	XMStoreFloat4(&perFrameCbData.mainLightDirection, inverseLightViewMatrix.r[2]);
	out_light_view_matrix = XMMatrixInverse(nullptr, inverseLightViewMatrix);
	out_light_proj_matrix = XMMatrixOrthographicLH(shadowDistance, shadowDistance, -directionalShadowDistance, directionalShadowDistance);
	perFrameCbData.mainLightShadowMatrix = get_shadow_matrix(out_light_view_matrix, out_light_proj_matrix);
	perFrameCbData.mainLightShadowParams = XMFLOAT4(shadowResolution, 1.0f / shadowResolution, 2 * directionalShadowDistance, poissonShadowSoftness);
	perFrameCbData.tonemappingParams = XMFLOAT4(exposure, 0, 0, 0);
	perFrameCbData.timeParams = XMFLOAT4(time, 0, 0, 0);
	perFrameCb->updateData(&perFrameCbData);

	drv->setConstantBuffer(ShaderStage::VS, 0, perFrameCb->getId());
	drv->setConstantBuffer(ShaderStage::PS, 0, perFrameCb->getId());
}

void WorldRenderer::setupShadowPass(const XMVECTOR& shadow_camera_pos, const XMMATRIX& light_view_matrix, const XMMATRIX& light_proj_matrix)
{
	// Shadow camera
	PerCameraConstantBufferData perCameraCbData{};
	perCameraCbData.view = light_view_matrix;
	perCameraCbData.projection = light_proj_matrix;
	perCameraCbData.viewProjection = light_view_matrix * light_proj_matrix;
	perCameraCbData.projectionParams = get_projection_params(-directionalShadowDistance, directionalShadowDistance, false);
	XMStoreFloat4(&perCameraCbData.cameraWorldPosition, shadow_camera_pos);
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
	setCameraForShaders(camera);

	// Clear targets
	const TextureDesc& targetDesc = hdrTarget->getDesc();
	drv->setRenderTarget(hdrTarget->getId(), depthTex->getId());
	drv->setView(0, 0, (float)targetDesc.width, (float)targetDesc.height, 0, 1);
	drv->clearRenderTargets(RenderTargetClearParams::clear_all(0.0f, 0.2f, 0.4f, 1.0f, 1.0f));
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

	drv->setTexture(ShaderStage::PS, 2, shadowMap->getId());
	drv->setSampler(ShaderStage::PS, 2, shadowSampler);
	drv->setTexture(ShaderStage::PS, 3, ssao->getResultTex()->getId());
	drv->setSampler(ShaderStage::PS, 3, linearClampSampler);

	for (MeshRenderer* mr : am->getSceneMeshRenderers())
		mr->render(RenderPass::FORWARD);

	drv->setTexture(ShaderStage::PS, 2, BAD_RESID);
	drv->setSampler(ShaderStage::PS, 2, BAD_RESID);
	drv->setTexture(ShaderStage::PS, 3, BAD_RESID);
	drv->setSampler(ShaderStage::PS, 3, BAD_RESID);
}
