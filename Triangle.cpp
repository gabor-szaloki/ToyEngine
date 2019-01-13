#include "Triangle.h"

Triangle::Triangle() : Drawable()
{
}

Triangle::~Triangle()
{
}

void Triangle::Init(ID3D11Device *device, ID3D11DeviceContext *context)
{
	StandardVertexData vertices[] =
	{
		{ XMFLOAT3( 0.0f,   0.5f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3( 0.45f, -0.5f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-0.45f, -0.5f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }
	};

	WORD indices[] = { 0, 1, 2 };

	InitBuffers(device, context, vertices, ARRAYSIZE(vertices), indices, ARRAYSIZE(indices));
}
