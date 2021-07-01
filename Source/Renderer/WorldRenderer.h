#pragma once

#include <Common.h>

#include <memory>
#include <vector>
#include <string>

#include <Util/ResIdHolder.h>

#include "Camera.h"
#include "PostFx.h"

class ITexture;
class IBuffer;

class ThreadPool;
class Light;
class MeshRenderer;
class Hbao;
class Sky;

struct MeshData;

class WorldRenderer
{
public:
	WorldRenderer();
	~WorldRenderer();
	void init();

	void onResize(int display_width, int display_height);
	void update(float delta_time);
	void render();

	void toggleWireframe() { showWireframe = !showWireframe; }
	void setAmbientLighting(const XMFLOAT4& bottom_color, const XMFLOAT4& top_color, float intensity);
	unsigned int getShadowResolution();
	void setShadowResolution(unsigned int shadow_resolution);
	void setShadowBias(int depth_bias, float slope_scaled_depth_bias);
	ITexture* getDepthTex() const { return depthTex.get(); }
	ITexture* getShadowMap() const { return shadowMap.get(); };
	Camera& getCamera() { return camera; };
	float getTime() { return time; }
	Sky& getSky() { return *sky; }

	void lightingGui();
	void shadowMapGui();
	void ssaoTexGui();

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

	float ambientLightIntensity = 1.0f;
	XMFLOAT4 ambientLightBottomColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	XMFLOAT4 ambientLightTopColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	std::unique_ptr<Light> mainLight;
	bool shadowEnabled = true;
	int shadowDepthBias = 0;
	float shadowSlopeScaledDepthBias = 0.0f;
	float shadowDistance = 30.0f;
	float directionalShadowDistance = 20.0f;
	float poissonShadowSoftness = 0.003f;

private:
	void initResolutionDependentResources();
	void closeResolutionDependentResources();

	void setupFrame(XMMATRIX& out_light_view_matrix, XMMATRIX& out_light_proj_matrix);
	void setupShadowPass(const XMMATRIX& light_view_matrix, const XMMATRIX& light_proj_matrix);
	void setupDepthAndForwardPasses();
	void performShadowPass();
	void performDepthPrepass();
	void performForwardPass();

	float time;

	Camera camera;
	float cameraMoveSpeed = 5.0f;
	float cameraTurnSpeed = 0.002f;

	std::unique_ptr<ITexture> hdrTarget;
	std::unique_ptr<ITexture> depthTex;
	ResIdHolder depthPrepassRenderStateId = BAD_RESID;
	ResIdHolder depthPrepassWireframeRenderStateId = BAD_RESID;
	ResIdHolder forwardRenderStateId = BAD_RESID;
	ResIdHolder forwardWireframeRenderStateId = BAD_RESID;
	std::unique_ptr<ITexture> shadowMap;
	ResIdHolder shadowRenderStateId = BAD_RESID;

	std::unique_ptr<IBuffer> perFrameCb;
	std::unique_ptr<IBuffer> perCameraCb;
	std::unique_ptr<IBuffer> perObjectCb;

	std::unique_ptr<Hbao> ssao;
	float ssaoResolutionScale = 1.0f;
	std::unique_ptr<Sky> sky;
	PostFx postFx;
};