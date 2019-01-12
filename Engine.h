#pragma once

#include <windows.h>

#include "Common.h"
#include "Drawable.h"

using namespace DirectX;

class Engine
{
public:
	
	Engine();
	~Engine();

	void Init(HWND hWnd, float viewportWidth, float viewportHeight);
	void Release();

	void InitD3D(HWND hWnd);
	void InitViewport(float w, float h);
	void InitPipeline();
	void InitScene();
	void ReleaseD3D();
	void RenderFrame(void);

private:

	ID3D11Device *device;
	ID3D11DeviceContext *context;
	IDXGISwapChain *swapchain;
	ID3D11RenderTargetView *backbuffer;

	ID3D11InputLayout *standardInputLayout;
	ID3D11VertexShader *standardVS;
	ID3D11PixelShader *standardOpaquePS;

	Drawable *triangle;
};

extern Engine *gEngine;