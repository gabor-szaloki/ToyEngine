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
class EnvironmentLightingSystem;

struct MeshData;

class WorldRenderer
{
public:
	WorldRenderer();
	~WorldRenderer();
	void init();

	void onResize(int display_width, int display_height);
	void update(float delta_time);
	void beforeRender();
	void render();
	void render(const Camera& camera, ITexture& hdr_color_target, ITexture* tonemapped_color_target, ITexture& depth_target,
		unsigned int hdr_color_slice, unsigned int tonemapped_color_slice, unsigned int depth_slice, bool ssao_enabled);

	void toggleWireframe() { showWireframe = !showWireframe; }
	void setEnvironment(ITexture* panoramic_environment_map, float radiance_cutoff, bool world_probe_enabled, const XMVECTOR& world_probe_pos); // <0 radiance cutoff means no cutoff
	void resetEnvironment();
	void onMeshLoaded();
	void onMaterialTexturesLoaded();
	unsigned int getShadowResolution();
	void setShadowResolution(unsigned int shadow_resolution);
	void setShadowBias(int depth_bias, float slope_scaled_depth_bias);
	ITexture* getDepthTex() const { return depthTex.get(); }
	ITexture* getShadowMap() const { return shadowMap.get(); };
	Camera& getSceneCamera() { return sceneCamera; };
	void setCameraForShaders(const Camera& cam);
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
	CameraInputState sceneCameraInputState;

	bool showWireframe = false;

	std::unique_ptr<Light> mainLight;
	bool mainLightEnabled = true;
	bool shadowEnabled = true;
	int shadowDepthBias = 0;
	float shadowSlopeScaledDepthBias = 0.0f;
	float shadowDistance = 30.0f;
	float directionalShadowDistance = 20.0f;
	float poissonShadowSoftness = 0.003f;

private:
	void initResolutionDependentResources();
	void closeResolutionDependentResources();

	void setupFrame(const Camera& camera, XMVECTOR& out_shadow_camera_pos, XMMATRIX& out_light_view_matrix, XMMATRIX& out_light_proj_matrix);
	void setupShadowPass(const XMVECTOR& shadow_camera_pos, const XMMATRIX& light_view_matrix, const XMMATRIX& light_proj_matrix);
	void setupDepthAndForwardPasses(const Camera& camera, ITexture& hdr_color_target, ITexture& depth_target, unsigned int hdr_color_slice, unsigned int depth_slice);
	void performShadowPass();
	void performDepthPrepass(ITexture& depth_target, unsigned int depth_slice);
	void performForwardPass(ITexture& hdr_color_target, ITexture& depth_target, unsigned int hdr_color_slice, unsigned int depth_slice);

	float time;

	Camera sceneCamera;
	float sceneCameraMoveSpeed = 5.0f;
	float sceneCameraTurnSpeed = 0.002f;

	std::unique_ptr<ITexture> hdrTarget;
	std::unique_ptr<ITexture> depthTex;
	ResIdHolder linearClampSampler = BAD_RESID, linearWrapSampler = BAD_RESID;
	ResIdHolder depthPrepassRenderStateId = BAD_RESID;
	ResIdHolder depthPrepassWireframeRenderStateId = BAD_RESID;
	ResIdHolder forwardRenderStateId = BAD_RESID;
	ResIdHolder forwardWireframeRenderStateId = BAD_RESID;
	std::unique_ptr<ITexture> shadowMap;
	ResIdHolder shadowSampler = BAD_RESID;
	ResIdHolder shadowRenderStateId = BAD_RESID;

	std::unique_ptr<IBuffer> perFrameCb;
	std::unique_ptr<IBuffer> perCameraCb;
	std::unique_ptr<IBuffer> perObjectCb;

	std::unique_ptr<Hbao> ssao;
	float ssaoResolutionScale = 1.0f;
	std::unique_ptr<Sky> sky;
	std::unique_ptr<ITexture> panoramicEnvironmentMap;
	std::unique_ptr<EnvironmentLightingSystem> enviLightSystem;
	bool debugShowIrradianceMap = false;
	bool debugShowSpecularMap = false;
	bool debugShowPureImageBasedLighting = false;
	float debugSpecularMapLod = 0.f;
	bool debugRecalculateEnvironmentLightingEveryFrame = false;
	float environmentRadianceCutoff = -1;
	PostFx postFx;
	float exposure = 1.0f;
};