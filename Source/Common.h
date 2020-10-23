#pragma once

#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")
#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")
#include <DirectXMath.h>

#include "AutoImGui.h"
#include <Driver/IDriver.h>

#define SAFE_RELEASE(resource) { if (resource != nullptr) { resource->Release(); resource = nullptr; } }
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

#define RAD_TO_DEG 57.2957795f;
#define DEG_TO_RAD 0.0174532925f;

extern IDriver* drv;
extern class WorldRenderer* wr;
extern void create_driver_d3d11();

void ThrowIfFailed(HRESULT hr, const char *errorMsg = nullptr);

using namespace DirectX;