#include "WorldRenderer.h"

#include <stdio.h>
#include <array>
#include <3rdParty/LodePNG/lodepng.h>
#include <3rdParty/tinyobjloader/tiny_obj_loader.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>
#include <Util/ToyJobSystem.h>

#include "ConstantBuffers.h"
#include "MeshRenderer.h"
#include "Primitives.h"
#include "Light.h"

WorldRenderer::WorldRenderer()
{
	camera.SetEye(XMVectorSet(-4.5f, 3.0f, -6.0f, 1.0f));
	camera.SetRotation(XM_PI / 6, XM_PI / 6);

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

	ToyJobSystem::init();

	ITexture* stubColor = loadTextureFromPng("Assets/Textures/gray_base.png", true);
	ITexture* stubNormal = loadTextureFromPng("Assets/Textures/flat_nrm.png", true);
	managedTextures.push_back(stubColor);
	managedTextures.push_back(stubNormal);
	defaultTextures[(int)MaterialTexture::Purpose::COLOR] = stubColor;
	defaultTextures[(int)MaterialTexture::Purpose::NORMAL] = stubNormal;
	defaultTextures[(int)MaterialTexture::Purpose::OTHER] = stubColor;

	initShaders();

	setShadowResolution(2048);
	setShadowBias(50000, 1.0f);
	initResolutionDependentResources();

	primitives::init();
	initScene();
}

WorldRenderer::~WorldRenderer()
{
	ToyJobSystem::shutdown();

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
	XMMATRIX mainLightTranslateMatrix = XMMatrixTranslationFromVector(camera.GetEye() + camera.GetForward() * shadowDistance * 0.5f);
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

ITexture* WorldRenderer::loadTextureFromPng(const char* path, bool sync)
{
	PLOG_INFO << "Loading texture from file: " << path;

	ITexture* texture = drv->createTextureStub();

	ToyJobSystem::get()->schedule(
		[path, texture]
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
			texture->recreate(tDesc);
			texture->updateData(0, nullptr, (void*)pngData.data());
			texture->generateMips();
		},
		sync);

	return texture;
}

bool WorldRenderer::loadMeshFromObjToMeshRenderer(const char* path, MeshRenderer& mesh_renderer)
{
	PLOG_INFO << "Loading mesh from file: " << path;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path);
	if (!warn.empty())
		PLOG_WARNING << warn;
	if (!err.empty())
		PLOG_ERROR << err;
	if (!success)
		return false;

	if (shapes.size() != 1)
		PLOG_WARNING << "Warning loading mesh. Only 1 shape per .obj file is supported now. This file contains " << shapes.size() << ". Using only the first one.";

	tinyobj::shape_t shape = shapes[0];
	const size_t numFaces = shape.mesh.num_face_vertices.size();
	constexpr size_t NUM_VERTICES_PER_FACE = 3; // Only triangles are supported now.

	if (attrib.vertices.size() % 3 != 0)
	{
		PLOG_ERROR << "Error loading mesh. \"attrib.vertices.size() % 3 != 0\"";
		return false;
	}

	std::vector<unsigned short> indexData(numFaces * NUM_VERTICES_PER_FACE);
	std::vector<StandardVertexData> vertexData(attrib.vertices.size() / 3);

	// Loop over faces
	for (size_t f = 0; f < numFaces; f++)
	{
		if (shape.mesh.num_face_vertices[f] != NUM_VERTICES_PER_FACE)
		{
			PLOG_ERROR << "Error loading mesh. Only 3 vertices per face (triangles) are supported now.";
			return false;
		}

		// Loop over vertices in the face.
		for (size_t v = 0; v < NUM_VERTICES_PER_FACE; v++)
		{
			tinyobj::index_t idx = shape.mesh.indices[f * NUM_VERTICES_PER_FACE + v];

			indexData[f * NUM_VERTICES_PER_FACE + v] = idx.vertex_index;
			StandardVertexData& vertex = vertexData[idx.vertex_index];
			vertex.position.x = attrib.vertices [3 * idx.vertex_index   + 0];
			vertex.position.y = attrib.vertices [3 * idx.vertex_index   + 1];
			vertex.position.z = attrib.vertices [3 * idx.vertex_index   + 2];
			vertex.normal.x   = attrib.normals  [3 * idx.normal_index   + 0];
			vertex.normal.y   = attrib.normals  [3 * idx.normal_index   + 1];
			vertex.normal.z   = attrib.normals  [3 * idx.normal_index   + 2];
			vertex.uv.x       = attrib.texcoords[2 * idx.texcoord_index + 0];
			vertex.uv.y       = attrib.texcoords[2 * idx.texcoord_index + 1];
			vertex.color.x    = attrib.colors   [3 * idx.vertex_index   + 0];
			vertex.color.y    = attrib.colors   [3 * idx.vertex_index   + 1];
			vertex.color.z    = attrib.colors   [3 * idx.vertex_index   + 2];
			vertex.color.w    = attrib.texcoord_ws.size() == 0 ? 1.0f : attrib.texcoord_ws[idx.vertex_index];
		}
	}

	mesh_renderer.loadIndices((unsigned int)indexData.size(), indexData.data());
	mesh_renderer.loadVertices((unsigned int)vertexData.size(), sizeof(vertexData[0]), vertexData.data());

	return true;
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

	Material* flatGray = new Material(standardShaders);
	flatGray->setTexture(ShaderStage::PS, 0, defaultTextures[(int)MaterialTexture::Purpose::COLOR]);
	flatGray->setTexture(ShaderStage::PS, 1, defaultTextures[(int)MaterialTexture::Purpose::NORMAL]);
	managedMaterials.push_back(flatGray);

	Material* test = new Material(standardShaders);
	test->setTexture(ShaderStage::PS, 0, testMaterialTextures[0]);
	test->setTexture(ShaderStage::PS, 1, testMaterialTextures[1], MaterialTexture::Purpose::NORMAL);
	managedMaterials.push_back(test);

	Material* blueTiles = new Material(standardShaders);
	blueTiles->setTexture(ShaderStage::PS, 0, blueTilesMaterialTextures[0]);
	blueTiles->setTexture(ShaderStage::PS, 1, blueTilesMaterialTextures[1], MaterialTexture::Purpose::NORMAL);
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

	/*
	MeshRenderer* teapot = new MeshRenderer("teapot", flatGray, standardInputLayout);
	teapot->worldTransform = XMMatrixMultiply(XMMatrixScaling(0.05f, 0.05f, 0.05f), XMMatrixTranslation(-4.5f, 0.5f, -3.0f));
	if (loadMeshFromObjToMeshRenderer("Assets/Models/UtahTeapot/utah-teapot.obj", *teapot))
		managedMeshRenderers.push_back(teapot);
	else
		delete teapot;

	MeshRenderer* bunny = new MeshRenderer("bunny", flatGray, standardInputLayout);
	bunny->worldTransform = XMMatrixMultiply(XMMatrixScaling(10.0f, 10.f, 10.f), XMMatrixTranslation(-1.5f, 0.0f, -3.0f));
	if (loadMeshFromObjToMeshRenderer("Assets/Models/StanfordBunny/stanford-bunny.obj", *bunny))
		managedMeshRenderers.push_back(bunny);
	else
		delete bunny;

	MeshRenderer* dragon = new MeshRenderer("dragon", flatGray, standardInputLayout);
	dragon->worldTransform = XMMatrixMultiply(XMMatrixScaling(0.2f, 0.2f, 0.2f), XMMatrixTranslation(1.5f, 0.0f, -3.0f));
	if (loadMeshFromObjToMeshRenderer("Assets/Models/StanfordDragon/stanford-dragon.obj", *dragon))
		managedMeshRenderers.push_back(dragon);
	else
		delete dragon;
	*/
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
	XMStoreFloat3(&perCameraCbData.cameraWorldPosition, camera.GetEye());
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
	perCameraCbData.view = camera.GetViewMatrix();
	perCameraCbData.projection = camera.GetProjectionMatrix();
	XMStoreFloat3(&perCameraCbData.cameraWorldPosition, camera.GetEye());
	perCameraCb->updateData(&perCameraCbData);

	drv->setConstantBuffer(ShaderStage::VS, 1, perCameraCb->getId());
	drv->setConstantBuffer(ShaderStage::PS, 1, perCameraCb->getId());

	drv->setTexture(ShaderStage::PS, 2, shadowMap->getId(), true);

	for (MeshRenderer* mr : managedMeshRenderers)
		mr->render(RenderPass::FORWARD);

	drv->setTexture(ShaderStage::PS, 2, BAD_RESID, true);
}
