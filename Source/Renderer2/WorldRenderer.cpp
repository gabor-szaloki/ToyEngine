#include "WorldRenderer.h"

#include <array>
#include <3rdParty/LodePNG/lodepng.h>

#include <Renderer/Camera.h>
#include <Renderer/Light.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>

#include "ConstantBuffers.h"
#include "Material2.h"
#include "MeshRenderer.h"
#include "Primitives2.h"

using namespace renderer;

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

	ShaderSetDesc shaderDesc("Source/Shaders/Standard.shader");
	shaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "StandardForwardVS";
	shaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "StandardOpaqueForwardPS";
	standardShaders[(int)RenderPass::FORWARD] = drv->createShaderSet(shaderDesc);
	shaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "StandardShadowVS";
	shaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "StandardOpaqueShadowPS";
	standardShaders[(int)RenderPass::SHADOW] = drv->createShaderSet(shaderDesc);

	// TODO: Standard input layout

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
}

void WorldRenderer::onResize()
{
	closeResolutionDependentResources();
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

void WorldRenderer::render()
{
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

ITexture* renderer::WorldRenderer::loadTextureFromPng(const char* path)
{
	std::vector<unsigned char> pngData;
	UINT width, height;
	UINT error = lodepng::decode(pngData, width, height, path);
	if (error != 0)
	{
		char errortext[MAX_PATH];
		sprintf_s(errortext, "error %d %s\n", error, lodepng_error_text(error));
		OutputDebugString(errortext);
	}

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
	testMaterialTextures[0] = loadTextureFromPng("Assets/Textures/Tiles20_base.png");
	testMaterialTextures[1] = loadTextureFromPng("Assets/Textures/Tiles20_nrm.png");
	managedTextures.insert(managedTextures.end(), blueTilesMaterialTextures.begin(), blueTilesMaterialTextures.end());

	Material* test = new Material(standardShaders);
	test->setTextures(ShaderStage::PS, testMaterialTextures.data(), 2);
	managedMaterials.push_back(test);

	Material* blueTiles = new Material(standardShaders);
	blueTiles->setTextures(ShaderStage::PS, blueTilesMaterialTextures.data(), 2);
	managedMaterials.push_back(blueTiles);

	MeshRenderer* floor = new MeshRenderer("floor", blueTiles);
	floor->loadVertices(primitives::PLANE_VERTEX_COUNT, sizeof(primitives::plane_vertices[0]), (void*)primitives::plane_vertices);
	floor->loadIndices(primitives::PLANE_INDEX_COUNT, (void*)primitives::plane_vertices);
	floor->worldTransform = XMMatrixScaling(10.0f, 10.0f, 10.0f);
	managedMeshRenderers.push_back(floor);

	MeshRenderer* box = new MeshRenderer("box", blueTiles);
	box->loadVertices(primitives::BOX_VERTEX_COUNT, sizeof(primitives::box_vertices[0]), (void*)primitives::box_vertices);
	box->loadIndices(primitives::PLANE_INDEX_COUNT, (void*)primitives::box_indices);
	box->worldTransform = XMMatrixTranslation(-1.5f, 1.5f, 0.0f);
	managedMeshRenderers.push_back(box);

	MeshRenderer* sphere = new MeshRenderer("sphere", blueTiles);
	sphere->loadVertices(primitives::BOX_VERTEX_COUNT, sizeof(primitives::sphere_vertices[0]), (void*)primitives::sphere_vertices);
	sphere->loadIndices(primitives::PLANE_INDEX_COUNT, (void*)primitives::sphere_indices);
	sphere->worldTransform = XMMatrixTranslation(1.5f, 1.5f, 0.0f);
	managedMeshRenderers.push_back(sphere);
}

void renderer::WorldRenderer::performForwardPass()
{
}

void renderer::WorldRenderer::performShadowPass(const XMMATRIX& lightViewMatrix, const XMMATRIX& lightProjectionMatrix)
{
}
