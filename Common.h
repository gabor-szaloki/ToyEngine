#pragma once

#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")
#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")
#include <DirectXMath.h>

#define SAFE_RELEASE(resource) { if (resource != nullptr) { resource->Release(); } }
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

using namespace DirectX;

struct StandardVertexData 
{ 
	XMFLOAT3 position; 
	XMFLOAT4 color;
};