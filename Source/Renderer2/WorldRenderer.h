#pragma once

#include <memory>
#include <3rdParty/glm/glm.hpp>

#include <Common.h>
#include <Driver/IDriver.h>

class Camera;
class Light;
class ITexture;
class IBuffer;

class WorldRenderer
{
public:
	WorldRenderer();
	~WorldRenderer();

	void onResize();
	void update(float delta_time);
	void render();

	void setShadowResolution(unsigned int shadow_resolution);
	void setShadowBias(int depth_bias, float slope_scaled_depth_bias);

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

private:
	void initResolutionDependentResources();
	void closeResolutionDependentResources();

	std::unique_ptr<Camera> camera;
	float cameraMoveSpeed = 5.0f;
	float cameraTurnSpeed = 0.002f;

	float ambientLightIntensity = 0.5f;
	glm::vec4 ambientLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	std::unique_ptr<Light> mainLight;
	float shadowDistance = 20.0f;
	float directionalShadowDistance = 20.0f;

	std::unique_ptr<ITexture> depthTex;
	ResId forwardRenderStateId;
	ResId forwardWireframeRenderStateId;
	std::unique_ptr<ITexture> shadowMap;
	ResId shadowRenderStateId;

	std::unique_ptr<IBuffer> perFrameCb;
	std::unique_ptr<IBuffer> perCameraCb;
	std::unique_ptr<IBuffer> perObjectCb;

	ResId standardShaderId;
	ResId standardShadowShaderId;

	bool showWireframe;
};