#include "WorldRenderer.h"

#include <stdio.h>
#include <array>
#include <3rdParty/glm/glm.hpp>
#include <3rdParty/LodePNG/lodepng.h>
#include <3rdParty/tinyobjloader/tiny_obj_loader.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>
#include <Util/ThreadPool.h>

#include "ConstantBuffers.h"
#include "MeshRenderer.h"
#include "Light.h"
#include "Sky.h"

static constexpr bool ASYNC_LOADING_ENABLED = true;

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

	setShadowResolution(2048);
	setShadowBias(50000, 1.0f);
	initResolutionDependentResources();

	const char* materialsIniPath = "Assets/materials.ini";
	materialsIniFile = std::make_unique<mINI::INIFile>(materialsIniPath);
	if (!materialsIniFile->read(materialsIni))
	{
		PLOG_ERROR << "Couldn't materials ini file: " << materialsIniPath;
		return;
	}

	const char* modelsIniPath = "Assets/models.ini";
	modelsIniFile = std::make_unique<mINI::INIFile>(modelsIniPath);
	if (!modelsIniFile->read(modelsIni))
	{
		PLOG_ERROR << "Couldn't models ini file: " << modelsIniPath;
		return;
	}

	initDefaultAssets();
	sky = std::make_unique<Sky>();

	// TODO: scene selector
	//loadScene("Assets/Scenes/default.ini");
	loadScene("Assets/Scenes/sponza.ini");
}

WorldRenderer::~WorldRenderer()
{
	unloadCurrentScene();

	defaultMeshVb.reset();
	defaultMeshIb.reset();

	for (ITexture* t : managedTextures)
		delete t;
	managedTextures.clear();

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
	//MeshRenderer* box = sceneMeshRenderers[1];
	//box->setTransform(Transform(XMFLOAT3(-1.5f, 1.5f, 0.0f), XMFLOAT3(0.0f, time * 0.05f, 0.0f)));
	//MeshRenderer* sphere = sceneMeshRenderers[2];
	//sphere->setTransform(Transform(XMFLOAT3(1.5f, 1.5f, 0.0f), XMFLOAT3(0.0f, time * 0.05f, 0.0f)));
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
	ITexture* stubColor = loadTextureFromPng("Assets/Textures/gray_base.png", true, LoadExecutionMode::SYNC);
	ITexture* stubNormal = loadTextureFromPng("Assets/Textures/flat_nrm.png", false, LoadExecutionMode::SYNC);
	managedTextures.push_back(stubColor);
	managedTextures.push_back(stubNormal);
	defaultTextures[(int)MaterialTexture::Purpose::COLOR] = stubColor;
	defaultTextures[(int)MaterialTexture::Purpose::NORMAL] = stubNormal;
	defaultTextures[(int)MaterialTexture::Purpose::OTHER] = stubColor;

	MeshData defaultMesh;
	loadMesh("Box", defaultMesh);
	BufferDesc vbDesc("defaultMeshVb", sizeof(defaultMesh.vertexData[0]), (unsigned int)defaultMesh.vertexData.size(), ResourceUsage::DEFAULT, BIND_VERTEX_BUFFER);
	vbDesc.initialData = defaultMesh.vertexData.data();
	defaultMeshVb.reset(drv->createBuffer(vbDesc));
	unsigned int indexByteSize = ::get_byte_size_for_texfmt(drv->getIndexFormat());
	BufferDesc ibDesc("defaultMeshIb", indexByteSize, (unsigned int)defaultMesh.indexData.size(), ResourceUsage::DEFAULT, BIND_INDEX_BUFFER);
	ibDesc.initialData = defaultMesh.indexData.data();
	defaultMeshIb.reset(drv->createBuffer(ibDesc));

	defaultInputLayout = standardInputLayout;
}

ITexture* WorldRenderer::loadTextureFromPng(const std::string& path, bool srgb, LoadExecutionMode lem)
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

	if (lem == LoadExecutionMode::ASYNC && ASYNC_LOADING_ENABLED)
		threadPool->enqueue(load);
	else
		load();

	return texture;
}

bool WorldRenderer::loadMesh(const std::string& name, MeshData& mesh_data)
{
	if (!modelsIni.has(name))
	{
		PLOG_ERROR << "No model found with name: " << name;
		return false;
	}

	std::string path = modelsIni[name]["path"];
	float importScale = modelsIni[name]["scale"].length() > 0 ? std::stof(modelsIni[name]["scale"]) : 1.0f;

	PLOG_INFO << "Loading mesh '" << name << "' from file: " << path;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str());
	if (!warn.empty())
		PLOG_WARNING << warn;
	if (!err.empty())
		PLOG_ERROR << err;
	if (!success)
		return false;

	mesh_data.submeshes.clear();

	unsigned int startIndex = 0;
	unsigned int startVertex = 0;

	for (const tinyobj::shape_t& shape : shapes)
	{
		const size_t numFaces = shape.mesh.num_face_vertices.size();
		constexpr size_t NUM_VERTICES_PER_FACE = 3; // Only triangles are supported now.

		if (attrib.vertices.size() % 3 != 0)
		{
			PLOG_ERROR << "Error loading mesh. \"attrib.vertices.size() % 3 != 0\"";
			return false;
		}

		SubmeshData submesh;
		submesh.startIndex = startIndex;
		submesh.numIndices = (unsigned int)(numFaces * NUM_VERTICES_PER_FACE);
		submesh.startVertex = startVertex;
		submesh.numVertices = (unsigned int)attrib.vertices.size() / 3;
		mesh_data.submeshes.push_back(submesh);

		mesh_data.indexData.resize(startIndex + submesh.numIndices);
		mesh_data.vertexData.resize(startVertex + submesh.numVertices);

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

				mesh_data.indexData[startIndex + f * NUM_VERTICES_PER_FACE + v] = idx.vertex_index;
				StandardVertexData& vertex = mesh_data.vertexData[startVertex + idx.vertex_index];
				vertex.position.x = attrib.vertices[3 * idx.vertex_index + 0] * importScale;
				vertex.position.y = attrib.vertices[3 * idx.vertex_index + 1] * importScale;
				vertex.position.z = attrib.vertices[3 * idx.vertex_index + 2] * importScale;
				vertex.normal.x = attrib.normals[3 * idx.normal_index + 0];
				vertex.normal.y = attrib.normals[3 * idx.normal_index + 1];
				vertex.normal.z = attrib.normals[3 * idx.normal_index + 2];
				vertex.uv.x = attrib.texcoords[2 * idx.texcoord_index + 0];
				vertex.uv.y = attrib.texcoords[2 * idx.texcoord_index + 1];

				XMFLOAT4 colorF4;
				colorF4.x = attrib.colors[3 * idx.vertex_index + 0];
				colorF4.y = attrib.colors[3 * idx.vertex_index + 1];
				colorF4.z = attrib.colors[3 * idx.vertex_index + 2];
				colorF4.w = attrib.texcoord_ws.size() == 0 ? 1.0f : attrib.texcoord_ws[idx.vertex_index];
				vertex.color = (((unsigned int)(colorF4.x * 255u)) & 0xFFu) << 0 |
					(((unsigned int)(colorF4.y * 255u)) & 0xFFu) << 8 |
					(((unsigned int)(colorF4.z * 255u)) & 0xFFu) << 16 |
					(((unsigned int)(colorF4.w * 255u)) & 0xFFu) << 24;
			}
		}

		startIndex += submesh.numIndices;
		startVertex += submesh.numVertices;
	}

	return true;
}

bool WorldRenderer::loadMeshToMeshRenderer(const std::string& name, MeshRenderer& mesh_renderer, LoadExecutionMode lem)
{
	auto load = [&, name]
	{
		MeshData meshData;
		if (!loadMesh(name, meshData))
			return false;
		mesh_renderer.load(meshData);
		return true;
	};

	if (lem == LoadExecutionMode::ASYNC && ASYNC_LOADING_ENABLED)
	{
		threadPool->enqueue(load);
		return true;
	}
	else
		return load();
}

void WorldRenderer::loadScene(const std::string& scene_file)
{
	currentSceneIniFile = std::make_unique<mINI::INIFile>(scene_file);
	if (!currentSceneIniFile->read(currentSceneIni))
	{
		PLOG_ERROR << "Couldn't find scene file: " << scene_file;
		return;
	}
	currentSceneIniFilePath = scene_file;

	std::map<std::string, ITexture*> texturePathMap;

	for (auto& sceneElem : currentSceneIni)
	{
		auto& elemProperties = currentSceneIni[sceneElem.first];

		std::string materialName = elemProperties["material"];
		Material* material = nullptr;
		auto it = std::find_if(sceneMaterials.begin(), sceneMaterials.end(), [&materialName](Material* m) { return materialName == m->name; });
		if (it != sceneMaterials.end())
			material = *it;
		else
		{
			if (!materialsIni.has(materialName))
			{
				PLOG_ERROR << "No material found with name: " << materialName;
				material = new Material(materialName.c_str(), { drv->getErrorShader(), drv->getErrorShader() });
			}
			else
			{
				material = new Material(materialName.c_str(), standardShaders);
				std::array<std::string, 2> texturePaths;
				texturePaths[0] = materialsIni[materialName]["texture0"];
				texturePaths[1] = materialsIni[materialName]["texture1"];
				for (int i = 0; i < texturePaths.size(); i++)
				{
					ITexture* tex;
					if (texturePathMap.find(texturePaths[i]) != texturePathMap.end())
						tex = texturePathMap[texturePaths[i]];
					else
					{
						tex = loadTextureFromPng(texturePaths[i].c_str(), i == 0); // This is ugly :(
						texturePathMap[texturePaths[i]] = tex;
						sceneTextures.push_back(tex);
					}
					material->setTexture(ShaderStage::PS, i, tex, (MaterialTexture::Purpose)i); // This is even uglier :((
				}
			}
			sceneMaterials.push_back(material);
		}

		MeshRenderer* mr = new MeshRenderer(sceneElem.first.c_str(), material, standardInputLayout);
		sceneMeshRenderers.push_back(mr);

		std::string modelName = elemProperties["model"];
		loadMeshToMeshRenderer(modelName, *mr);

		auto strToVector = [](std::string s)
		{
			if (s.length() == 0)
				return XMVectorZero();
			char delimiter = ',';
			size_t pos = s.find(delimiter);
			assert(pos != std::string::npos); // No comma
			float x = std::stof(s.substr(0, pos));
			s.erase(0, pos + 1);
			pos = s.find(delimiter);
			assert(pos != std::string::npos); // Only 1 comma
			float y = std::stof(s.substr(0, pos));
			s.erase(0, pos + 1);
			pos = s.find(delimiter);
			assert(pos == std::string::npos); // More than 2 comma
			float z = std::stof(s.substr(0, pos));
			return XMVectorSet(x, y, z, 0);
		};

		Transform tr;
		tr.position = strToVector(elemProperties["position"]);
		tr.rotation = strToVector(elemProperties["rotation"]);
		tr.scale = elemProperties["scale"].length() > 0 ? std::stof(elemProperties["scale"]) : 1.0f;
		mr->setTransform(tr);
	}
}

void WorldRenderer::unloadCurrentScene()
{
	for (MeshRenderer* mr : sceneMeshRenderers)
		delete mr;
	sceneMeshRenderers.clear();
	for (Material* m : sceneMaterials)
		delete m;
	sceneMaterials.clear();
	for (ITexture* t : sceneTextures)
		delete t;
	sceneTextures.clear();
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

	for (MeshRenderer* mr : sceneMeshRenderers)
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

	for (MeshRenderer* mr : sceneMeshRenderers)
		mr->render(RenderPass::FORWARD);

	drv->setTexture(ShaderStage::PS, 2, BAD_RESID, true);
}
