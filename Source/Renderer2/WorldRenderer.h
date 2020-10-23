#pragma once

#include "RendererCommon.h"

#include <memory>
#include <vector>
#include <array>
#include <3rdParty/glm/glm.hpp>

#include <Driver/IDriver.h>

class Camera;
class Light;
class ITexture;
class IBuffer;

namespace renderer
{
	class Material;
	class MeshRenderer;

	class WorldRenderer
	{
	public:
		WorldRenderer();
		~WorldRenderer();

		void onResize(int display_width, int display_height);
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
		void initShaders();
		void closeShaders();
		ITexture* loadTextureFromPng(const char* path);
		void initScene();
		void performShadowPass(const XMMATRIX& lightViewMatrix, const XMMATRIX& lightProjectionMatrix);
		void performForwardPass();

		float time;

		std::unique_ptr<Camera> camera;
		float cameraMoveSpeed = 5.0f;
		float cameraTurnSpeed = 0.002f;

		float ambientLightIntensity = 0.5f;
		glm::vec4 ambientLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		std::unique_ptr<Light> mainLight;
		float shadowDistance = 20.0f;
		float directionalShadowDistance = 20.0f;

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

		bool showWireframe;


		std::vector<ITexture*> managedTextures;
		std::vector<Material*> managedMaterials;
		std::vector<MeshRenderer*> managedMeshRenderers;
	};
}