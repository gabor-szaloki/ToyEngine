#include "WorldRenderer.h"

#include <stdio.h>
#include <array>
#include <3rdParty/LodePNG/lodepng.h>
#include <Renderer/Camera.h>
#include <Renderer/Light.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>

#include "ConstantBuffers.h"
#include "Material.h"
#include "MeshRenderer.h"
#include "Primitives.h"

WorldRenderer::WorldRenderer()
{
	camera = std::make_unique<Camera>();
	camera->SetEye(XMVectorSet(2.0f, 3.5f, -3.0f, 0.0f));
	camera->SetRotation(XM_PI / 6, -XM_PIDIV4);

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

	initShaders();

	setShadowResolution(2048);
	setShadowBias(50000, 1.0f);
	initResolutionDependentResources();

	primitives::init();
	initScene();
}

WorldRenderer::~WorldRenderer()
{
	for (ITexture* t : managedTextures)
		delete t;
	managedTextures.clear();
	for (Material* m : managedMaterials)
		delete m;
	managedMaterials.clear();
	for (MeshRenderer* mr : managedMeshRenderers)
		delete mr;
	managedMeshRenderers.clear();

	closeShaders();
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
	time += delta_time;;

	// Update camera movement
	XMVECTOR cameraMoveDir =
		camera->GetRight() * ((cameraInputState.isMovingRight ? 1.0f : 0.0f) + (cameraInputState.isMovingLeft ? -1.0f : 0.0f)) +
		camera->GetUp() * ((cameraInputState.isMovingUp ? 1.0f : 0.0f) + (cameraInputState.isMovingDown ? -1.0f : 0.0f)) +
		camera->GetForward() * ((cameraInputState.isMovingForward ? 1.0f : 0.0f) + (cameraInputState.isMovingBackward ? -1.0f : 0.0f));
	XMVECTOR cameraMoveDelta = cameraMoveDir * cameraMoveSpeed * delta_time;
	if (cameraInputState.isSpeeding)
		cameraMoveDelta *= 2.0f;
	camera->MoveEye(cameraMoveDelta);

	// Update camera rotation
	camera->Rotate(cameraInputState.deltaPitch * cameraTurnSpeed, cameraInputState.deltaYaw * cameraTurnSpeed);
	cameraInputState.deltaPitch = cameraInputState.deltaYaw = 0;

	// Update scene
	MeshRenderer* box = managedMeshRenderers[1];
	box->worldTransform = XMMatrixRotationY(time * 0.05f) * XMMatrixTranslation(-1.5f, 1.5f, 0.0f);
	MeshRenderer* sphere = managedMeshRenderers[2];
	sphere->worldTransform = XMMatrixRotationY(time * 0.05f) * XMMatrixTranslation(1.5f, 1.5f, 0.0f);
}

template<typename T>
static XMFLOAT4 get_final_light_color(const T& color, float intensity)
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
	// Update per-frame constant buffer
	PerFrameConstantBufferData perFrameCbData;
	perFrameCbData.ambientLightColor = get_final_light_color(ambientLightColor, ambientLightIntensity);
	perFrameCbData.mainLightColor = get_final_light_color(mainLight->GetColor(), mainLight->GetIntensity());
	XMMATRIX mainLightTranslateMatrix = XMMatrixTranslationFromVector(camera->GetEye() + camera->GetForward() * shadowDistance * 0.5f);
	XMMATRIX inverseLightViewMatrix = XMMatrixRotationRollPitchYaw(mainLight->GetPitch(), mainLight->GetYaw(), 0.0f) * mainLightTranslateMatrix;
	XMStoreFloat4(&perFrameCbData.mainLightDirection, inverseLightViewMatrix.r[2]);
	XMMATRIX lightViewMatrix = XMMatrixInverse(nullptr, inverseLightViewMatrix);
	XMMATRIX lightProjectionMatrix = XMMatrixOrthographicLH(shadowDistance, shadowDistance, -directionalShadowDistance, directionalShadowDistance);
	perFrameCbData.mainLightShadowMatrix = get_shadow_matrix(lightViewMatrix, lightProjectionMatrix);
	const float shadowResolution = (float)shadowMap->getDesc().width;
	const float invShadowResolution = 1.0f / shadowResolution;
	perFrameCbData.mainLightShadowResolution = XMFLOAT4(invShadowResolution, invShadowResolution, shadowResolution, shadowResolution);
	perFrameCb->updateData(&perFrameCbData);

	drv->setConstantBuffer(ShaderStage::VS, 0, perFrameCb->getId());
	drv->setConstantBuffer(ShaderStage::PS, 0, perFrameCb->getId());

	performShadowPass(lightViewMatrix, lightProjectionMatrix);
	performForwardPass();
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
	shadowRenderStateId = drv->createRenderState(desc);
}

void WorldRenderer::initResolutionDependentResources()
{
	RenderStateDesc forwaredRsDesc;
	forwardRenderStateId = drv->createRenderState(forwaredRsDesc);
	forwaredRsDesc.rasterizerDesc.wireframe = true;
	forwardWireframeRenderStateId = drv->createRenderState(forwaredRsDesc);

	glm::ivec2 displayResolution;
	drv->getDisplaySize(displayResolution.x, displayResolution.y);
	TextureDesc depthTexDesc("depthTex", displayResolution.x, displayResolution.y,
		TexFmt::D32_FLOAT_S8X24_UINT, 1, ResourceUsage::DEFAULT, BIND_DEPTH_STENCIL);
	depthTex.reset(drv->createTexture(depthTexDesc));
}

void WorldRenderer::closeResolutionDependentResources()
{
	depthTex.reset();
	forwardRenderStateId.close();
	forwardWireframeRenderStateId.close();
}

void WorldRenderer::initShaders()
{
	ShaderSetDesc errorShaderDesc("Source/Shaders/Error.shader");
	errorShaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "ErrorVS";
	errorShaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "ErrorPS";
	drv->setErrorShaderDesc(errorShaderDesc);

	ShaderSetDesc shaderDesc("Source/Shaders/Standard.shader");
	shaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "StandardForwardVS";
	shaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "StandardOpaqueForwardPS";
	standardShaders[(int)RenderPass::FORWARD] = drv->createShaderSet(shaderDesc);
	shaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "StandardShadowVS";
	shaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "StandardOpaqueShadowPS";
	standardShaders[(int)RenderPass::SHADOW] = drv->createShaderSet(shaderDesc);

	constexpr unsigned int NUM_STANDARD_INPUT_LAYOUT_ELEMENTS = 4;
	InputLayoutElementDesc standardInputLayoutDesc[NUM_STANDARD_INPUT_LAYOUT_ELEMENTS] =
	{
		{ VertexInputSemantic::POSITION, 0, TexFmt::R32G32B32_FLOAT    },
		{ VertexInputSemantic::NORMAL,   0, TexFmt::R32G32B32_FLOAT    },
		{ VertexInputSemantic::COLOR,    0, TexFmt::R32G32B32A32_FLOAT },
		{ VertexInputSemantic::TEXCOORD, 0, TexFmt::R32G32_FLOAT       },
	};
	standardInputLayout = drv->createInputLayout(standardInputLayoutDesc,
		NUM_STANDARD_INPUT_LAYOUT_ELEMENTS, standardShaders[(int)RenderPass::FORWARD]);
}

void WorldRenderer::closeShaders()
{
	standardInputLayout.close();
	for (ResId& id : standardShaders)
	{
		drv->destroyResource(id);
		id = BAD_RESID;
	}
}

ITexture* WorldRenderer::loadTextureFromPng(const char* path)
{
	std::vector<unsigned char> pngData;
	unsigned int width, height;
	unsigned int error = lodepng::decode(pngData, width, height, path);
	if (error != 0)
		PLOG_ERROR << "Error loading texture." << std::endl
			<< "\tFile: " << path << std::endl
			<< "\tError: " << lodepng_error_text(error);

	TextureDesc tDesc(path, width, height, TexFmt::R8G8B8A8_UNORM, 0);
	tDesc.bindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
	tDesc.miscFlags = RESOURCE_MISC_GENERATE_MIPS;
	ITexture* texture = drv->createTexture(tDesc);

	texture->updateData(0, nullptr, (void*)pngData.data());
	texture->generateMips();

	return texture;
}

void WorldRenderer::initScene()
{
	std::array<ITexture*, 2> testMaterialTextures;
	testMaterialTextures[0] = loadTextureFromPng("Assets/Textures/test_base.png");
	testMaterialTextures[1] = loadTextureFromPng("Assets/Textures/test_nrm.png");
	managedTextures.insert(managedTextures.end(), testMaterialTextures.begin(), testMaterialTextures.end());

	std::array<ITexture*, 2> blueTilesMaterialTextures;
	blueTilesMaterialTextures[0] = loadTextureFromPng("Assets/Textures/Tiles20_base.png");
	blueTilesMaterialTextures[1] = loadTextureFromPng("Assets/Textures/Tiles20_nrm.png");
	managedTextures.insert(managedTextures.end(), blueTilesMaterialTextures.begin(), blueTilesMaterialTextures.end());

	Material* test = new Material(standardShaders);
	test->setTextures(ShaderStage::PS, testMaterialTextures.data(), 2);
	managedMaterials.push_back(test);

	Material* blueTiles = new Material(standardShaders);
	blueTiles->setTextures(ShaderStage::PS, blueTilesMaterialTextures.data(), 2);
	managedMaterials.push_back(blueTiles);

	MeshRenderer* floor = new MeshRenderer("floor", blueTiles, standardInputLayout);
	floor->loadVertices(primitives::PLANE_VERTEX_COUNT, sizeof(primitives::plane_vertices[0]), (void*)primitives::plane_vertices);
	floor->loadIndices(primitives::PLANE_INDEX_COUNT, (void*)primitives::plane_indices);
	floor->worldTransform = XMMatrixScaling(10.0f, 10.0f, 10.0f);
	managedMeshRenderers.push_back(floor);

	MeshRenderer* box = new MeshRenderer("box", blueTiles, standardInputLayout);
	box->loadVertices(primitives::BOX_VERTEX_COUNT, sizeof(primitives::box_vertices[0]), (void*)primitives::box_vertices);
	box->loadIndices(primitives::BOX_INDEX_COUNT, (void*)primitives::box_indices);
	box->worldTransform = XMMatrixTranslation(-1.5f, 1.5f, 0.0f);
	managedMeshRenderers.push_back(box);

	MeshRenderer* sphere = new MeshRenderer("sphere", blueTiles, standardInputLayout);
	sphere->loadVertices(primitives::SPHERE_VERTEX_COUNT, sizeof(primitives::sphere_vertices[0]), (void*)primitives::sphere_vertices);
	sphere->loadIndices(primitives::SPHERE_INDEX_COUNT, (void*)primitives::sphere_indices);
	sphere->worldTransform = XMMatrixTranslation(1.5f, 1.5f, 0.0f);
	managedMeshRenderers.push_back(sphere);
}

void WorldRenderer::performShadowPass(const XMMATRIX& lightViewMatrix, const XMMATRIX& lightProjectionMatrix)
{
	drv->setRenderState(shadowRenderStateId);
	drv->setRenderTarget(BAD_RESID, shadowMap->getId());
	drv->setView(0, 0, (float)shadowMap->getDesc().width, (float)shadowMap->getDesc().height, 0, 1);

	RenderTargetClearParams clearParams(CLEAR_FLAG_DEPTH, 0, 1.0f);
	drv->clearRenderTargets(clearParams);

	PerCameraConstantBufferData perCameraCbData;
	perCameraCbData.view = lightViewMatrix;
	perCameraCbData.projection = lightProjectionMatrix;
	XMStoreFloat3(&perCameraCbData.cameraWorldPosition, camera->GetEye());
	perCameraCb->updateData(&perCameraCbData);

	drv->setConstantBuffer(ShaderStage::VS, 1, perCameraCb->getId());
	drv->setConstantBuffer(ShaderStage::PS, 1, perCameraCb->getId());

	for (MeshRenderer* mr : managedMeshRenderers)
		mr->render(RenderPass::SHADOW);
}

void WorldRenderer::performForwardPass()
{
	drv->setRenderState(showWireframe ? forwardWireframeRenderStateId : forwardRenderStateId);

	ITexture* backbuffer = drv->getBackbufferTexture();
	const TextureDesc& bbDesc = backbuffer->getDesc();
	drv->setRenderTarget(backbuffer->getId(), depthTex->getId());
	drv->setView(0, 0, (float)bbDesc.width, (float)bbDesc.height, 0, 1);

	RenderTargetClearParams clearParams;
	clearParams.color[0] = 0.0f;
	clearParams.color[1] = 0.2f;
	clearParams.color[2] = 0.4f;
	clearParams.color[3] = 1.0f;
	drv->clearRenderTargets(clearParams);

	PerCameraConstantBufferData perCameraCbData;
	perCameraCbData.view = camera->GetViewMatrix();
	perCameraCbData.projection = camera->GetProjectionMatrix();
	XMStoreFloat3(&perCameraCbData.cameraWorldPosition, camera->GetEye());
	perCameraCb->updateData(&perCameraCbData);

	drv->setConstantBuffer(ShaderStage::VS, 1, perCameraCb->getId());
	drv->setConstantBuffer(ShaderStage::PS, 1, perCameraCb->getId());

	drv->setTexture(ShaderStage::PS, 2, shadowMap->getId(), true);

	for (MeshRenderer* mr : managedMeshRenderers)
		mr->render(RenderPass::FORWARD);

	drv->setTexture(ShaderStage::PS, 2, BAD_RESID, true);
}
