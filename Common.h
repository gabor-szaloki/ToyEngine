#pragma once

#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")
#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")
#include <DirectXMath.h>

#define SAFE_RELEASE(resource) { if (resource != nullptr) { resource->Release(); } }

struct StandardVertexData { float X, Y, Z; float Color[4]; };