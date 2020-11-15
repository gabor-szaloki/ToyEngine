#pragma once

#include "RendererCommon.h"

#include <memory>
#include <vector>
#include <array>
#include <string>

#define MINI_CASE_SENSITIVE
#include <3rdParty/mini/ini.h>

#include <Driver/IDriver.h>

#include "Camera.h"
#include "Material.h"
#include "PostFx.h"

class ITexture;
class IBuffer;

class ThreadPool;
class Light;
class MeshRenderer;
class Sky;

struct StandardVertexData;

class WorldRenderer
{
public:
	WorldRenderer();
	~WorldRenderer();

	void onResize(int display_width, int display_height);
	void update(float delta_time);
	void render();

	unsigned int getShadowResolution();
	void setShadowResolution(unsigned int shadow_resolution);
	void setShadowBias(int depth_bias, float slope_scaled_depth_bias);
	ITexture* getShadowMap() const { return shadowMap.get(); };
	Camera& getCamera() { return camera; };
	float getTime() { return time; }
	Sky& getSky() { return *sky; }

	void rendererSettingsGui();
	void lightingGui();
	void shadowMapGui();
	void meshRendererGui();

	struct CameraInputState
	{
		bool isMovingForward = false;
		bool isMovingBackward = false;
		bool isMovingRight = false;
		bool isMovingLeft = false;
		bool isMovingUp = false;
		bool isMovingDown = false;
		bool isSpeeding = false;
		int deltaYaw = 0;
		int deltaPitch = 0;
	};
	CameraInputState cameraInputState;

	bool showWireframe = false;

	float ambientLightIntensity = 0.5f;
	XMFLOAT4 ambientLightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	std::unique_ptr<Light> mainLight;
	int shadowDepthBias = 0;
	float shadowSlopeScaledDepthBias = 0.0f;
	float shadowDistance = 20.0f;
	float directionalShadowDistance = 20.0f;

	std::array<ITexture*, (int)MaterialTexture::Purpose::_COUNT> defaultTextures;
	std::unique_ptr<IBuffer> defaultMeshIb;
	std::unique_ptr<IBuffer> defaultMeshVb;
	ResId defaultInputLayout;

private:
	void initResolutionDependentResources();
	void closeResolutionDependentResources();
	void initShaders();
	void closeShaders();
	void initDefaultAssets();
	// TODO: Separate loader code from WorldRenderer
	enum class LoadExecutionMode { ASYNC, SYNC };
	ITexture* loadTextureFromPng(const std::string& path, bool srgb, LoadExecutionMode lem = LoadExecutionMode::ASYNC);
	bool loadMesh(const std::string& name, std::vector<StandardVertexData>& vertex_data, std::vector<unsigned short>& index_data);
	bool loadMeshToMeshRenderer(const std::string& name, MeshRenderer& mesh_renderer, LoadExecutionMode lem = LoadExecutionMode::ASYNC);
	void initScene(const char* scene_file);
	void performShadowPass(const XMMATRIX& lightViewMatrix, const XMMATRIX& lightProjectionMatrix);
	void performForwardPass();

	float time;

	Camera camera;
	float cameraMoveSpeed = 5.0f;
	float cameraTurnSpeed = 0.002f;

	std::unique_ptr<ITexture> hdrTarget;
	std::unique_ptr<ITexture> depthTex;
	ResIdHolder forwardRenderStateId = BAD_RESID;
	ResIdHolder forwardWireframeRenderStateId = BAD_RESID;
	std::unique_ptr<ITexture> shadowMap;
	ResIdHolder shadowRenderStateId = BAD_RESID;

	std::unique_ptr<IBuffer> perFrameCb;
	std::unique_ptr<IBuffer> perCameraCb;
	std::unique_ptr<IBuffer> perObjectCb;

	std::array<ResId, (int)RenderPass::_COUNT> standardShaders;
	ResIdHolder standardInputLayout = BAD_RESID;

	std::unique_ptr<mINI::INIFile> materialsIniFile;
	mINI::INIStructure materialsIni;
	std::unique_ptr<mINI::INIFile> modelsIniFile;
	mINI::INIStructure modelsIni;
	std::unique_ptr<mINI::INIFile> currentSceneIniFile;
	mINI::INIStructure currentSceneIni;

	std::vector<ITexture*> managedTextures;
	std::vector<Material*> managedMaterials;
	std::vector<MeshRenderer*> managedMeshRenderers;

	std::unique_ptr<ThreadPool> threadPool;

	std::unique_ptr<Sky> sky;
	PostFx postFx;
};