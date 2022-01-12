#include "WorldRenderer.h"

#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>
#include <Engine/AssetManager.h>
#include <Engine/MeshRenderer.h>

#include "ConstantBuffers.h"
#include "Light.h"
#include "TemporalAntiAliasing.h"
#include "Hbao.h"
#include "Water.h"
#include "Sky.h"
#include "EnvironmentLightingSystem.h"
#include "VarianceShadowMap.h"

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
	linearBorderSampler = drv->createSampler(SamplerDesc(FILTER_MIN_MAG_MIP_LINEAR, TexAddr::BORDER));
	shadowSampler = drv->createSampler(SamplerDesc(FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, TexAddr::BORDER, ComparisonFunc::LESS_EQUAL));

	setSoftShadowMode(SoftShadowMode::TENT);
	setShadowResolution(2048);
	setShadowBias(50000, 1.0f);

	RenderStateDesc shadowRenderStateNoBiasDesc;
	shadowRenderStateNoBiasDesc.rasterizerDesc.depthClipEnable = false;
	shadowRenderStateNoBiasId = drv->createRenderState(shadowRenderStateNoBiasDesc);
}

WorldRenderer::~WorldRenderer()
{
}

void WorldRenderer::init()
{
	initResolutionDependentResources();
	taa = std::make_unique<TemporalAntiAliasing>();
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
	frameCount++;

	// Update scene camera movement
	XMVECTOR sceneCameraMoveDir =
		sceneCamera.GetRight() * ((sceneCameraInputState.isMovingRight ? 1.0f : 0.0f) + (sceneCameraInputState.isMovingLeft ? -1.0f : 0.0f)) +
		sceneCamera.GetUp() * ((sceneCameraInputState.isMovingUp ? 1.0f : 0.0f) + (sceneCameraInputState.isMovingDown ? -1.0f : 0.0f)) +
		sceneCamera.GetForward() * ((sceneCameraInputState.isMovingForward ? 1.0f : 0.0f) + (sceneCameraInputState.isMovingBackward ? -1.0f : 0.0f));
	XMVECTOR sceneCameraMoveDelta = sceneCameraMoveDir * sceneCameraMoveSpeed * delta_time;
	if (sceneCameraInputState.isSpeeding)
		sceneCameraMoveDelta *= 2.0f;
	sceneCamera.MoveEye(sceneCameraMoveDelta);

	// Update scene camera rotation
	sceneCamera.Rotate(sceneCameraInputState.deltaPitch * sceneCameraTurnSpeed, sceneCameraInputState.deltaYaw * sceneCameraTurnSpeed);
	sceneCameraInputState.deltaPitch = sceneCameraInputState.deltaYaw = 0;
}

void WorldRenderer::beforeRender()
{
	PROFILE_SCOPE("BeforeRender");

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

		enviLightSystem->setEnvironmentRadianceCutoff(debugShowPureImageBasedLighting ? -1 : environmentRadianceCutoff);
		enviLightSystem->bake(sky->getBakedCube());
	}
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
	render(getSceneCamera(), *hdrTarget.get(), drv->getBackbufferTexture(), *depthTex.get(), 0, 0, 0, true);
}

void WorldRenderer::render(const Camera& camera, ITexture& hdr_color_target, ITexture* tonemapped_color_target, ITexture& depth_target,
	unsigned int hdr_color_slice, unsigned int tonemapped_color_slice, unsigned int depth_slice, bool ssao_enabled)
{
	PROFILE_SCOPE("RenderWorld");

	// Set resources for lighting
	drv->setTexture(ShaderStage::PS, 10, enviLightSystem->getIrradianceCube()->getId());
	drv->setTexture(ShaderStage::PS, 11, enviLightSystem->getSpecularCube()->getId());
	drv->setTexture(ShaderStage::PS, 12, enviLightSystem->getBrdfLut()->getId());
	drv->setSampler(ShaderStage::PS, 10, linearClampSampler);

	XMVECTOR shadowCameraPos;
	XMMATRIX lightViewMatrix;
	XMMATRIX lightProjectionMatrix;
	setupFrame(camera, shadowCameraPos, lightViewMatrix, lightProjectionMatrix);

	setupShadowPass(shadowCameraPos, lightViewMatrix, lightProjectionMatrix);
	if (shadowEnabled)
	{
		PROFILE_SCOPE("ShadowPass");
		performRenderPass(RenderPass::DEPTH);
	}
	if (softShadowMode == SoftShadowMode::VARIANCE)
	{
		if (shadowEnabled)
			varianceShadowMap->render(shadowMap.get());
		else
			varianceShadowMap->clear();
	}

	setupDepthAndForwardPasses(camera, hdr_color_target, depth_target, hdr_color_slice, depth_slice);

	drv->setRenderState(showWireframe ? depthPrepassWireframeRenderStateId : depthPrepassRenderStateId);
	drv->setRenderTarget(BAD_RESID, depth_target.getId(), 0, depth_slice);

	{
		PROFILE_SCOPE("DepthPrepass");
		performRenderPass(RenderPass::DEPTH);
	}

	const bool depthCopyNeeded = water != nullptr;
	if (depthCopyNeeded)
	{
		PROFILE_SCOPE("CopyDepthTexture");
		depthTex->copyResource(depthCopyTex->getId());
	}

	if (ssao_enabled)
		ssao->perform();
	else
		ssao->clear();

	drv->setRenderState(showWireframe ? forwardWireframeRenderStateId : forwardRenderStateId);
	drv->setRenderTarget(hdr_color_target.getId(), depth_target.getId(), hdr_color_slice, depth_slice);

	drv->setTexture(ShaderStage::PS, 2, softShadowMode == SoftShadowMode::VARIANCE ? varianceShadowMap->getTexture()->getId() : shadowMap->getId());
	drv->setSampler(ShaderStage::PS, 2, softShadowMode == SoftShadowMode::VARIANCE ? linearBorderSampler : shadowSampler);
	drv->setTexture(ShaderStage::PS, 3, ssao->getResultTex()->getId());
	drv->setSampler(ShaderStage::PS, 3, linearClampSampler);

	{
		PROFILE_SCOPE("ForwardPass");
		performRenderPass(RenderPass::FORWARD);
	}

	const bool sceneGrabNeeded = water != nullptr;
	if (sceneGrabNeeded)
	{
		PROFILE_SCOPE("GrabSceneTexture");
		hdrTarget->copyResource(hdrSceneGrabBeforeWaterTexture->getId());
	}

	if (water != nullptr)
		water->render(hdrSceneGrabBeforeWaterTexture.get(), depthCopyTex.get());

	drv->setTexture(ShaderStage::PS, 2, BAD_RESID);
	drv->setSampler(ShaderStage::PS, 2, BAD_RESID);
	drv->setTexture(ShaderStage::PS, 3, BAD_RESID);
	drv->setSampler(ShaderStage::PS, 3, BAD_RESID);

	if (debugShowSpecularMap)
		sky->render(enviLightSystem->getSpecularCube(), debugSpecularMapLod);
	else if (debugShowIrradianceMap)
		sky->render(enviLightSystem->getIrradianceCube());
	else
		sky->render();

	ITexture& antialiasedHdrTarget = *antialiasedHdrTargets[currentAntiAliasedTarget];
	ITexture& taaHistoryTex = *antialiasedHdrTargets[1 - currentAntiAliasedTarget];
	taa->perform(hdr_color_target, antialiasedHdrTarget, taaHistoryTex);

	if (tonemapped_color_target != nullptr)
	{
		drv->setRenderTarget(tonemapped_color_target->getId(), BAD_RESID, tonemapped_color_slice);
		drv->setTexture(ShaderStage::PS, 0, antialiasedHdrTarget.getId());
		drv->setSampler(ShaderStage::PS, 0, linearClampSampler);

		postFx.perform();

		drv->setTexture(ShaderStage::PS, 0, BAD_RESID);
		drv->setSampler(ShaderStage::PS, 0, BAD_RESID);
	}

	currentAntiAliasedTarget = 1 - currentAntiAliasedTarget;
}

void WorldRenderer::setEnvironment(ITexture* panoramic_environment_map, float radiance_cutoff, bool world_probe_enabled, const XMVECTOR& world_probe_pos)
{
	panoramicEnvironmentMap.reset(panoramic_environment_map);
	sky->markDirty();
	environmentRadianceCutoff = radiance_cutoff;
	enviLightSystem->setWorldProbe(world_probe_enabled, world_probe_pos);
}

void WorldRenderer::resetEnvironment()
{
	setEnvironment(nullptr, -1, false, XMVectorZero());
}

void WorldRenderer::onMeshLoaded()
{
	if (enviLightSystem->hasWorldProbe())
		enviLightSystem->markDirty();
}

void WorldRenderer::onMaterialTexturesLoaded()
{
	if (enviLightSystem->hasWorldProbe())
		enviLightSystem->markDirty();
}

void WorldRenderer::onGlobalShaderKeywordsChanged()
{
	if (water != nullptr)
		water->onGlobalShaderKeywordsChanged();
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

	if (varianceShadowMap != nullptr)
		varianceShadowMap.reset(new VarianceShadowMap(shadow_resolution));
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

void WorldRenderer::setSoftShadowMode(SoftShadowMode soft_shadow_mode)
{
	softShadowMode = soft_shadow_mode;

	for (int i = 0; i < NUM_SOFT_SHADOW_MODES; i++)
		am->setGlobalShaderKeyword(softShadowModeShaderKeywords[i], (SoftShadowMode)i == softShadowMode);

	if (softShadowMode == SoftShadowMode::VARIANCE && varianceShadowMap == nullptr)
		varianceShadowMap.reset(new VarianceShadowMap(getShadowResolution()));
	else if (softShadowMode != SoftShadowMode::VARIANCE && varianceShadowMap != nullptr)
		varianceShadowMap.reset();
}

void WorldRenderer::setWater(const Transform* water_transform)
{
	water.reset();
	if (water_transform != nullptr)
		water = std::make_unique<Water>(*water_transform);
}

static XMFLOAT4 get_projection_params(float zn, float zf, bool persp)
{
	return XMFLOAT4(zn * zf, zn - zf, zf, persp ? 1.0f : 0.0f);
}

void WorldRenderer::setCameraForShaders(const Camera& cam, bool allow_jitter)
{
	PerCameraConstantBufferData perCameraCbData;

	perCameraCbData.projection = cam.GetProjectionMatrix();
	if (allow_jitter)
	{
		XMFLOAT2 jitter = taa->getJitter(frameCount);
		perCameraCbData.projection.r[2].m128_f32[0] = (2.0f * jitter.x) / cam.GetViewportWidth();
		perCameraCbData.projection.r[2].m128_f32[1] = (2.0f * jitter.y) / cam.GetViewportHeight();
	}

	perCameraCbData.view = cam.GetViewMatrix();
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

	drv->setConstantBuffer(ShaderStage::VS, PER_CAMERA_CONSTANT_BUFFER_SLOT, perCameraCb->getId());
	drv->setConstantBuffer(ShaderStage::PS, PER_CAMERA_CONSTANT_BUFFER_SLOT, perCameraCb->getId());
}

void WorldRenderer::initResolutionDependentResources()
{
	XMINT2 displayResolution;
	drv->getDisplaySize(displayResolution.x, displayResolution.y);

	TextureDesc hdrTargetDesc("hdrTarget", displayResolution.x, displayResolution.y,
		TexFmt::R16G16B16A16_FLOAT, 1, ResourceUsage::DEFAULT, BIND_RENDER_TARGET | BIND_SHADER_RESOURCE);
	hdrTarget.reset(drv->createTexture(hdrTargetDesc));

	hdrTargetDesc.name = "antialiasedHdrTarget_0";
	antialiasedHdrTargets[0].reset(drv->createTexture(hdrTargetDesc));
	hdrTargetDesc.name = "antialiasedHdrTarget_1";
	antialiasedHdrTargets[1].reset(drv->createTexture(hdrTargetDesc));

	hdrTargetDesc.name = "hdrSceneGrabBeforeWaterTexture";
	hdrTargetDesc.bindFlags = BIND_SHADER_RESOURCE;
	hdrSceneGrabBeforeWaterTexture.reset(drv->createTexture(hdrTargetDesc));

	TextureDesc depthTexDesc("depthTex", displayResolution.x, displayResolution.y,
		TexFmt::R24G8_TYPELESS, 1, ResourceUsage::DEFAULT, BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE);
	depthTexDesc.srvFormatOverride = TexFmt::R24_UNORM_X8_TYPELESS;
	depthTexDesc.dsvFormatOverride = TexFmt::D24_UNORM_S8_UINT;
	depthTex.reset(drv->createTexture(depthTexDesc));

	depthTexDesc.name = "depthCopyTex";
	depthTexDesc.bindFlags = BIND_SHADER_RESOURCE;
	depthCopyTex.reset(drv->createTexture(depthTexDesc));

	sceneCamera.Resize((float)displayResolution.x, (float)displayResolution.y);

	XMINT2 ssaoResolution(displayResolution.x * ssaoResolutionScale, displayResolution.y * ssaoResolutionScale);
	ssao = std::make_unique<Hbao>(ssaoResolution);
}

void WorldRenderer::closeResolutionDependentResources()
{
	depthTex.reset();
	depthCopyTex.reset();
	hdrTarget.reset();
	hdrSceneGrabBeforeWaterTexture.reset();
	antialiasedHdrTargets[0].reset();
	antialiasedHdrTargets[1].reset();
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

void WorldRenderer::setupFrame(const Camera& camera, XMVECTOR& out_shadow_camera_pos, XMMATRIX& out_light_view_matrix, XMMATRIX& out_light_proj_matrix)
{
	PerFrameConstantBufferData perFrameCbData;
	perFrameCbData.mainLightColor = get_final_light_color(mainLight->GetColor(), (mainLightEnabled && !debugShowPureImageBasedLighting) ? mainLight->GetIntensity() : 0.f);
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

	drv->setConstantBuffer(ShaderStage::VS, PER_FRAME_CONSTANT_BUFFER_SLOT, perFrameCb->getId());
	drv->setConstantBuffer(ShaderStage::PS, PER_FRAME_CONSTANT_BUFFER_SLOT, perFrameCb->getId());
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
	drv->setConstantBuffer(ShaderStage::VS, PER_CAMERA_CONSTANT_BUFFER_SLOT, perCameraCb->getId());
	drv->setConstantBuffer(ShaderStage::PS, PER_CAMERA_CONSTANT_BUFFER_SLOT, perCameraCb->getId());

	// Setup state and render target
	drv->setRenderState(softShadowMode == SoftShadowMode::VARIANCE ? shadowRenderStateNoBiasId : shadowRenderStateId);
	drv->setRenderTarget(BAD_RESID, shadowMap->getId());
	drv->setView(0, 0, (float)shadowMap->getDesc().width, (float)shadowMap->getDesc().height, 0, 1);
	RenderTargetClearParams clearParams(CLEAR_FLAG_DEPTH, 0, 1.0f);
	drv->clearRenderTargets(clearParams);
}

void WorldRenderer::setupDepthAndForwardPasses(const Camera& camera, ITexture& hdr_color_target, ITexture& depth_target, unsigned int hdr_color_slice, unsigned int depth_slice)
{
	// Main camera
	setCameraForShaders(camera, true);

	// Clear targets
	const TextureDesc& targetDesc = hdr_color_target.getDesc();
	drv->setRenderTarget(hdr_color_target.getId(), depth_target.getId(), hdr_color_slice, depth_slice);
	drv->setView(0, 0, (float)targetDesc.width, (float)targetDesc.height, 0, 1);
	drv->clearRenderTargets(RenderTargetClearParams::clear_all(0.0f, 0.2f, 0.4f, 1.0f, 1.0f));
}

void WorldRenderer::performRenderPass(RenderPass pass)
{
	for (MeshRenderer* mr : am->getSceneMeshRenderers())
		mr->render(pass);
}