#pragma once

#include <windows.h>
#include <vector>

#include "EngineCommon.h"
#include "Camera.h"
#include "Light.h"
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
	void InitShadowmap();
	void ReleaseShadowmap();
	void InitImGui();
	void ReleaseImGui();
	void InitPipeline();
	void ReleasePipeline();
	void InitShaders();
	void ReleaseShaders();
	void InitScene();
	void ReleaseScene();

	void Resize(HWND hWnd, float width, float height);

	void Update(float deltaTime);
	void UpdateGUI();
	void RenderFrame();

	float GetTime() { return time; }
	float GetDeltaTime() { return deltaTime; }

	void ToggleGUI() { guiState.enabled = !guiState.enabled; }

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

	float ambientLightIntensity;
	XMFLOAT4 ambientLightColor;
	int shadowResolution;
	Light *mainLight;

	HWND hWnd;

	ID3D11Device *device;
	ID3D11DeviceContext *context;
	IDXGISwapChain *swapchain;
	
	// Forward Pass
	ID3D11RenderTargetView *backbuffer;
	ID3D11DepthStencilView *depthStencilView;
	ID3D11DepthStencilState *depthStencilState;
	D3D11_VIEWPORT forwardPassViewport;

	// Shadow Pass
	ID3D11RasterizerState *wireframeRasterizerState;
	ID3D11DepthStencilView *shadowmapDSV;
	ID3D11ShaderResourceView *shadowmapSRV;
	D3D11_VIEWPORT shadowPassViewport;

	ID3D11Buffer *perFrameCB;
	ID3D11Buffer *perObjectCB;

	ID3D11InputLayout *standardInputLayout;
	ID3D11VertexShader *standardForwardVS, *standardShadowVS;
	ID3D11PixelShader *standardOpaqueForwardPS, *standardOpaqueShadowPS;

	ID3D11SamplerState *samplerLinearWrap;
	ID3D11SamplerState *samplerAnisotropicWrap;
	bool showWireframe;
	bool anisotropicFilteringEnabled;
	int vsync;

	Drawable *floor;
	Drawable *box;
	Drawable* sphere;

	std::vector<Drawable*> drawables;
	std::vector<Material*> materials;
	std::vector<ID3D11ShaderResourceView*> textureRVs;

	struct GuiState
	{
		bool enabled;
		bool showDemoWindow;
		bool showEngineSettingsWindow;
		bool showLightSettingsWindow;
		bool showStatsWindow;
		bool showShadowmapDebugWindow;
	};
	GuiState guiState;

	void ShadowPass();
	void ForwardPass();

	ID3D11ShaderResourceView *LoadTextureFromPNG(const char *filename);

	XMFLOAT4 GetFinalLightColor(XMFLOAT4 color, float intensity);

	ID3D11VertexShader *CompileVertexShader(LPCWSTR fileName, LPCSTR entryPoint, ID3DBlob **outBlob = nullptr);
	ID3D11PixelShader *CompilePixelShader(LPCWSTR fileName, LPCSTR entryPoint, ID3DBlob **outBlob = nullptr);
};

extern Engine *gEngine;