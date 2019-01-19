#pragma once

#include <windows.h>
#include <vector>

#include "EngineCommon.h"
#include "Camera.h"
#include "Drawable.h"

class Engine
{
public:
	
	Engine();
	~Engine();

	void Init(HWND hWnd, float viewportWidth, float viewportHeight);
	void Release();

	void InitDevice();
	void ReleaseDevice();
	void InitRenderTargets(float width, float height);
	void ReleaseRenderTargets();
	void InitImGui();
	void ReleaseImGui();
	void InitPipeline();
	void ReleasePipeline();
	void InitScene();
	void ReleaseScene();

	void Resize(HWND hWnd, float width, float height);

	void Update(float elapsedTime);
	void UpdateGUI();
	void RenderFrame();

	float GetTime() { return time; }
	float GetDeltaTime() { return deltaTime; }

	void ToggleGUI() { guiState.enabled = !guiState.enabled; }

	ID3D11ShaderResourceView *LoadTextureFromPNG(const char *filename);

	struct CameraInputState
	{
		bool isMovingForward;
		bool isMovingBackward;
		bool isMovingRight;
		bool isMovingLeft;
		bool isMovingUp;
		bool isMovingDown;
		bool isSpeeding;
		int deltaYaw;
		int deltaPitch;
	};
	CameraInputState cameraInputState;

private:

	float time, deltaTime;

	Camera *camera;
	float cameraMoveSpeed, cameraTurnSpeed;

	float mainLightIntensity;
	XMFLOAT4 mainLightColor;
	float mainLightYaw, mainLightPitch;

	HWND hWnd;

	ID3D11Device *device;
	ID3D11DeviceContext *context;
	IDXGISwapChain *swapchain;
	ID3D11RenderTargetView *backbuffer;
	ID3D11DepthStencilView *depthStencilView;
	ID3D11DepthStencilState *depthStencilState;

	ID3D11Buffer *perFrameCB;
	ID3D11Buffer *perObjectCB;

	ID3D11InputLayout *standardInputLayout;
	ID3D11VertexShader *standardVS;
	ID3D11PixelShader *standardOpaquePS;

	ID3D11SamplerState *samplerLinearWrap;
	ID3D11SamplerState *samplerAnisotropicWrap;
	bool anisotropicFilteringEnabled;

	Drawable *box;
	Drawable *floor;

	std::vector<Drawable*> drawables;
	std::vector<Material*> materials;
	std::vector<ID3D11ShaderResourceView*> textureRVs;

	struct GuiState
	{
		bool enabled;
		bool showDemoWindow;
		bool showEngineSettingsWindow;
		bool showLightSettingsWindow;
	};
	GuiState guiState;

	XMFLOAT4 GetMainLightColorIntensity();
};

extern Engine *gEngine;