#include "WorldRenderer.h"

#include <stdio.h>
#include <array>
#include <3rdParty/glm/glm.hpp>
#include <3rdParty/LodePNG/lodepng.h>
#include <3rdParty/tinyobjloader/tiny_obj_loader.h>
#include <3rdParty/mini/ini.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>
#include <Util/ThreadPool.h>

#include "ConstantBuffers.h"
#include "MeshRenderer.h"
#include "Light.h"
#include "Sky.h"

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

	initShaders();

	const int numCores = std::thread::hardware_concurrency();
	const int numWorkerThreads = glm::clamp(numCores - 1, 3, 12);
	PLOG_INFO << "Detected number of processor cores: " << numCores;
	PLOG_INFO << "Initializing thread pool with " << numWorkerThreads << " threads";
	threadPool = std::make_unique<ThreadPool>(numWorkerThreads);

	RenderStateDesc forwaredRsDesc;
	forwardRenderStateId = drv->createRenderState(forwaredRsDesc);
	forwaredRsDesc.rasterizerDesc.wireframe = true;
	forwardWireframeRenderStateId = drv->createRenderState(forwaredRsDesc);

	initDefaultAssets();

	setShadowResolution(2048);
	setShadowBias(50000, 1.0f);
	initResolutionDependentResources();

	initScene();
}

WorldRenderer::~WorldRenderer()
{
	defaultMeshVb.reset();
	defaultMeshIb.reset();

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
	camera.Resize((float)display_width, (float)display_height);
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
	box->setTransform(Transform(XMFLOAT3(-1.5f, 1.5f, 0.0f), XMFLOAT3(0.0f, time * 0.05f, 0.0f)));
	MeshRenderer* sphere = managedMeshRenderers[2];
	sphere->setTransform(Transform(XMFLOAT3(1.5f, 1.5f, 0.0f), XMFLOAT3(0.0f, time * 0.05f, 0.0f)));
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
	PROFILE_SCOPE("RenderWorld");

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

	sky->render();

	drv->setRenderTarget(drv->getBackbufferTexture()->getId(), BAD_RESID);
	drv->setTexture(ShaderStage::PS, 0, hdrTarget->getId(), true);
	postFx.perform();
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
	XMINT2 displayResolution;
	drv->getDisplaySize(displayResolution.x, displayResolution.y);

	TextureDesc hdrTargetDesc("hdrTarget", displayResolution.x, displayResolution.y,
		TexFmt::R16G16B16A16_FLOAT, 1, ResourceUsage::DEFAULT, BIND_RENDER_TARGET | BIND_SHADER_RESOURCE);
	hdrTarget.reset(drv->createTexture(hdrTargetDesc));

	TextureDesc depthTexDesc("depthTex", displayResolution.x, displayResolution.y,
		TexFmt::D32_FLOAT_S8X24_UINT, 1, ResourceUsage::DEFAULT, BIND_DEPTH_STENCIL);
	depthTex.reset(drv->createTexture(depthTexDesc));
}

void WorldRenderer::closeResolutionDependentResources()
{
	depthTex.reset();
	hdrTarget.reset();
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
		{ VertexInputSemantic::POSITION, 0, TexFmt::R32G32B32_FLOAT },
		{ VertexInputSemantic::NORMAL,   0, TexFmt::R32G32B32_FLOAT },
		{ VertexInputSemantic::COLOR,    0, TexFmt::R8G8B8A8_UNORM  },
		{ VertexInputSemantic::TEXCOORD, 0, TexFmt::R32G32_FLOAT    },
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

void WorldRenderer::initDefaultAssets()
{
	ITexture* stubColor = loadTextureFromPng("Assets/Textures/gray_base.png", true, true);
	ITexture* stubNormal = loadTextureFromPng("Assets/Textures/flat_nrm.png", false, true);
	managedTextures.push_back(stubColor);
	managedTextures.push_back(stubNormal);
	defaultTextures[(int)MaterialTexture::Purpose::COLOR] = stubColor;
	defaultTextures[(int)MaterialTexture::Purpose::NORMAL] = stubNormal;
	defaultTextures[(int)MaterialTexture::Purpose::OTHER] = stubColor;

	std::vector<StandardVertexData> defaultMeshVertexData;
	std::vector<unsigned short> defaultMeshIndexData;
	loadMeshFromObj("Assets/Models/box.obj", defaultMeshVertexData, defaultMeshIndexData);
	BufferDesc vbDesc("defaultMeshVb", sizeof(defaultMeshVertexData[0]), (unsigned int)defaultMeshVertexData.size(), ResourceUsage::DEFAULT, BIND_VERTEX_BUFFER);
	vbDesc.initialData = defaultMeshVertexData.data();
	defaultMeshVb.reset(drv->createBuffer(vbDesc));
	unsigned int indexByteSize = ::get_byte_size_for_texfmt(drv->getIndexFormat());
	BufferDesc ibDesc("defaultMeshIb", indexByteSize, (unsigned int)defaultMeshIndexData.size(), ResourceUsage::DEFAULT, BIND_INDEX_BUFFER);
	ibDesc.initialData = defaultMeshIndexData.data();
	defaultMeshIb.reset(drv->createBuffer(ibDesc));

	defaultInputLayout = standardInputLayout;
}

ITexture* WorldRenderer::loadTextureFromPng(const char* path, bool srgb, bool sync)
{
	ITexture* texture = drv->createTextureStub();

	auto load = [path, srgb, texture]
	{
		PLOG_INFO << "Loading texture from file: " << path;

		std::vector<unsigned char> pngData;
		unsigned int width, height;
		unsigned int error = lodepng::decode(pngData, width, height, path);
		if (error != 0)
			PLOG_ERROR << "Error loading texture." << std::endl
			<< "\tFile: " << path << std::endl
			<< "\tError: " << lodepng_error_text(error);

		TextureDesc tDesc(path, width, height, srgb ? TexFmt::R8G8B8A8_UNORM_SRGB : TexFmt::R8G8B8A8_UNORM, 0);
		tDesc.bindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
		tDesc.miscFlags = RESOURCE_MISC_GENERATE_MIPS;
		texture->recreate(tDesc);
		texture->updateData(0, nullptr, (void*)pngData.data());
		texture->generateMips();
	};

	if (sync)
		load();
	else
		threadPool->enqueue(load);

	return texture;
}

bool WorldRenderer::loadMeshFromObj(const char* path, std::vector<StandardVertexData>& vertex_data, std::vector<unsigned short>& index_data)
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

	std::string pathStr(path);
	std::string iniPath = pathStr.substr(0, pathStr.find_last_of('.')) + ".ini";
	mINI::INIFile iniFile(iniPath);
	mINI::INIStructure ini;
	float importScale = 1.0f;
	if (iniFile.read(ini))
	{
		mINI::INIMap importSettings = ini["import"];
		if (importSettings.has("scale"))
			importScale = std::stof(importSettings["scale"]);
	}

	index_data.resize(numFaces * NUM_VERTICES_PER_FACE);
	vertex_data.resize(attrib.vertices.size() / 3);

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

			index_data[f * NUM_VERTICES_PER_FACE + v] = idx.vertex_index;
			StandardVertexData& vertex = vertex_data[idx.vertex_index];
			vertex.position.x = attrib.vertices [3 * idx.vertex_index   + 0] * importScale;
			vertex.position.y = attrib.vertices [3 * idx.vertex_index   + 1] * importScale;
			vertex.position.z = attrib.vertices [3 * idx.vertex_index   + 2] * importScale;
			vertex.normal.x   = attrib.normals  [3 * idx.normal_index   + 0];
			vertex.normal.y   = attrib.normals  [3 * idx.normal_index   + 1];
			vertex.normal.z   = attrib.normals  [3 * idx.normal_index   + 2];
			vertex.uv.x       = attrib.texcoords[2 * idx.texcoord_index + 0];
			vertex.uv.y       = attrib.texcoords[2 * idx.texcoord_index + 1];

			XMFLOAT4 colorF4;
			colorF4.x = attrib.colors   [3 * idx.vertex_index   + 0];
			colorF4.y = attrib.colors   [3 * idx.vertex_index   + 1];
			colorF4.z = attrib.colors   [3 * idx.vertex_index   + 2];
			colorF4.w = attrib.texcoord_ws.size() == 0 ? 1.0f : attrib.texcoord_ws[idx.vertex_index];
			vertex.color = (((unsigned int)(colorF4.x * 255u)) & 0xFFu) <<  0 |
						   (((unsigned int)(colorF4.y * 255u)) & 0xFFu) <<  8 |
						   (((unsigned int)(colorF4.z * 255u)) & 0xFFu) << 16 |
						   (((unsigned int)(colorF4.w * 255u)) & 0xFFu) << 24;
		}
	}

	return true;
}

bool WorldRenderer::loadMeshFromObjToMeshRenderer(const char* path, MeshRenderer& mesh_renderer)
{
	std::vector<StandardVertexData> vertexData;
	std::vector<unsigned short> indexData;

	if (!loadMeshFromObj(path, vertexData, indexData))
		return false;

	mesh_renderer.loadIndices((unsigned int)indexData.size(), indexData.data());
	mesh_renderer.loadVertices((unsigned int)vertexData.size(), sizeof(vertexData[0]), vertexData.data());

	return true;
}

void WorldRenderer::loadMeshFromObjToMeshRendererAsync(const char* path, MeshRenderer& mesh_renderer)
{
	threadPool->enqueue([&, path] { loadMeshFromObjToMeshRenderer(path, mesh_renderer); });
}

void WorldRenderer::initScene()
{
	sky = std::make_unique<Sky>();

	std::array<ITexture*, 2> testMaterialTextures;
	testMaterialTextures[0] = loadTextureFromPng("Assets/Textures/test_base.png", true);
	testMaterialTextures[1] = loadTextureFromPng("Assets/Textures/test_nrm.png", false);
	managedTextures.insert(managedTextures.end(), testMaterialTextures.begin(), testMaterialTextures.end());

	std::array<ITexture*, 2> blueTilesMaterialTextures;
	blueTilesMaterialTextures[0] = loadTextureFromPng("Assets/Textures/Tiles20_base.png", true);
	blueTilesMaterialTextures[1] = loadTextureFromPng("Assets/Textures/Tiles20_nrm.png", false);
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

	MeshRenderer* floor = new MeshRenderer("floor", test, standardInputLayout);
	loadMeshFromObjToMeshRendererAsync("Assets/Models/plane.obj", *floor);
	floor->setScale(10.0f);
	managedMeshRenderers.push_back(floor);

	MeshRenderer* box = new MeshRenderer("box", test, standardInputLayout);
	loadMeshFromObjToMeshRendererAsync("Assets/Models/box.obj", *box);
	box->setPosition(-1.5f, 1.5f, 0.0f);
	managedMeshRenderers.push_back(box);

	MeshRenderer* sphere = new MeshRenderer("sphere", blueTiles, standardInputLayout);
	loadMeshFromObjToMeshRendererAsync("Assets/Models/sphere.obj", *sphere);
	sphere->setPosition(1.5f, 1.5f, 0.0f);
	managedMeshRenderers.push_back(sphere);

	MeshRenderer* teapot = new MeshRenderer("teapot", flatGray, standardInputLayout);
	teapot->setPosition(-4.5f, 0.5f, -3.0f);
	loadMeshFromObjToMeshRendererAsync("Assets/Models/UtahTeapot/utah-teapot.obj", *teapot);
	managedMeshRenderers.push_back(teapot);

	MeshRenderer* bunny = new MeshRenderer("bunny", flatGray, standardInputLayout);
	bunny->setPosition(-1.5f, 0.0f, -3.0f);
	loadMeshFromObjToMeshRendererAsync("Assets/Models/StanfordBunny/stanford-bunny.obj", *bunny);
	managedMeshRenderers.push_back(bunny);

	MeshRenderer* dragon = new MeshRenderer("dragon", flatGray, standardInputLayout);
	dragon->setPosition(1.5f, 0.0f, -3.0f);
	loadMeshFromObjToMeshRendererAsync("Assets/Models/StanfordDragon/stanford-dragon.obj", *dragon);
	managedMeshRenderers.push_back(dragon);
}

void WorldRenderer::performShadowPass(const XMMATRIX& lightViewMatrix, const XMMATRIX& lightProjectionMatrix)
{
	PROFILE_SCOPE("ShadowPass");

	drv->setRenderState(shadowRenderStateId);
	drv->setRenderTarget(BAD_RESID, shadowMap->getId());
	drv->setView(0, 0, (float)shadowMap->getDesc().width, (float)shadowMap->getDesc().height, 0, 1);

	RenderTargetClearParams clearParams(CLEAR_FLAG_DEPTH, 0, 1.0f);
	drv->clearRenderTargets(clearParams);

	PerCameraConstantBufferData perCameraCbData{};
	perCameraCbData.view = lightViewMatrix;
	perCameraCbData.projection = lightProjectionMatrix;
	perCameraCb->updateData(&perCameraCbData);

	drv->setConstantBuffer(ShaderStage::VS, 1, perCameraCb->getId());
	drv->setConstantBuffer(ShaderStage::PS, 1, perCameraCb->getId());

	for (MeshRenderer* mr : managedMeshRenderers)
		mr->render(RenderPass::SHADOW);
}

void WorldRenderer::performForwardPass()
{
	PROFILE_SCOPE("ForwardPass");

	drv->setRenderState(showWireframe ? forwardWireframeRenderStateId : forwardRenderStateId);

	const TextureDesc& targetDesc = hdrTarget->getDesc();
	drv->setRenderTarget(hdrTarget->getId(), depthTex->getId());
	drv->setView(0, 0, (float)targetDesc.width, (float)targetDesc.height, 0, 1);

	RenderTargetClearParams clearParams;
	clearParams.color[0] = 0.0f;
	clearParams.color[1] = 0.2f;
	clearParams.color[2] = 0.4f;
	clearParams.color[3] = 1.0f;
	drv->clearRenderTargets(clearParams);

	PerCameraConstantBufferData perCameraCbData;
	perCameraCbData.view = camera.GetViewMatrix();
	perCameraCbData.projection = camera.GetProjectionMatrix();
	XMStoreFloat4(&perCameraCbData.cameraWorldPosition, camera.GetEye());
	XMMATRIX viewRotMatrix = perCameraCbData.view;
	viewRotMatrix.r[3] = XMVectorSet(0, 0, 0, 1);
	XMMATRIX viewRotProjMatrix = viewRotMatrix * perCameraCbData.projection;
	XMMATRIX invViewRotProjMatrix = XMMatrixInverse(nullptr, viewRotProjMatrix);
	XMStoreFloat4(&perCameraCbData.viewVecLT, XMVector3TransformCoord(XMVectorSet(-1.f, +1.f, 1.f, 0.f), invViewRotProjMatrix));
	XMStoreFloat4(&perCameraCbData.viewVecRT, XMVector3TransformCoord(XMVectorSet(+1.f, +1.f, 1.f, 0.f), invViewRotProjMatrix));
	XMStoreFloat4(&perCameraCbData.viewVecLB, XMVector3TransformCoord(XMVectorSet(-1.f, -1.f, 1.f, 0.f), invViewRotProjMatrix));
	XMStoreFloat4(&perCameraCbData.viewVecRB, XMVector3TransformCoord(XMVectorSet(+1.f, -1.f, 1.f, 0.f), invViewRotProjMatrix));
	perCameraCb->updateData(&perCameraCbData);

	drv->setConstantBuffer(ShaderStage::VS, 1, perCameraCb->getId());
	drv->setConstantBuffer(ShaderStage::PS, 1, perCameraCb->getId());

	drv->setTexture(ShaderStage::PS, 2, shadowMap->getId(), true);

	for (MeshRenderer* mr : managedMeshRenderers)
		mr->render(RenderPass::FORWARD);

	drv->setTexture(ShaderStage::PS, 2, BAD_RESID, true);
}
