#pragma once

#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")
#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")
#include <DirectXMath.h>

#define SAFE_RELEASE(resource) { if (resource != nullptr) { resource->Release(); } }

struct StandardVertexData 
{ 
	DirectX::XMFLOAT3 position; 
	DirectX::XMFLOAT4 color;
};