#include "Plane.h"

Plane::Plane()
{
}

Plane::~Plane()
{
}

void Plane::Init(ID3D11Device * device, ID3D11DeviceContext * context)
{
	StandardVertexData vertices[] =
	{
		// Position						 Normal							Tangent								 Color							   UV
		{ XMFLOAT3(-1.0f,  0.0f, -1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), XMFLOAT4( 1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3( 1.0f,  0.0f, -1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), XMFLOAT4( 1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3( 1.0f,  0.0f,  1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), XMFLOAT4( 1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f,  0.0f,  1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), XMFLOAT4( 1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
	};

	WORD indices[] =
	{
		3,1,0,
		2,1,3,
	};

	InitBuffers(device, context, vertices, ARRAYSIZE(vertices), indices, ARRAYSIZE(indices));
}
