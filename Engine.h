#pragma once

#include "Common.h"

#include <windows.h>

#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")
#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

class Engine
{
public:
	
	Engine();
	~Engine();

	void Init(HWND hWnd);
	void Release();

	void InitD3D(HWND hWnd);
	void InitPipeline();
	void InitGraphics();
	void ReleaseD3D();
	void RenderFrame(void);

private:

	ID3D11Device *device;
	ID3D11DeviceContext *context;
	IDXGISwapChain *swapchain;
	ID3D11RenderTargetView *backbuffer;

	struct StandardVertexData { float X, Y, Z; float Color[4]; };
	ID3D11InputLayout *standardInputLayout;
	ID3D11VertexShader *standardVS;
	ID3D11PixelShader *standardOpaquePS;

	ID3D11Buffer *vertexBuffer;
};

extern Engine *gEngine;