#pragma once

#include "RendererCommon.h"

#include <memory>
#include <vector>
#include <array>
#include <3rdParty/glm/glm.hpp>
#include <Driver/IDriver.h>
#include "Camera.h"
#include "Material.h"

class Light;
class ITexture;
class IBuffer;

class MeshRenderer;

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
	glm::vec4 ambientLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
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
	ITexture* loadTextureFromPng(const char* path, bool sync = false);
	bool loadMeshFromObjToMeshRenderer(const char* path, MeshRenderer& mesh_renderer);
	void loadMeshFromObjToMeshRendererAsync(const char* path, MeshRenderer& mesh_renderer);
	void initScene();
	void performShadowPass(const XMMATRIX& lightViewMatrix, const XMMATRIX& lightProjectionMatrix);
	void performForwardPass();

	float time;

	Camera camera;
	float cameraMoveSpeed = 5.0f;
	float cameraTurnSpeed = 0.002f;

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

	std::vector<ITexture*> managedTextures;
	std::vector<Material*> managedMaterials;
	std::vector<MeshRenderer*> managedMeshRenderers;
};