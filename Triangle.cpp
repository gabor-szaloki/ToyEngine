#include "Triangle.h"

Triangle::Triangle()
{
}


Triangle::~Triangle()
{
}

void Triangle::Init(ID3D11Device *device, ID3D11DeviceContext *context)
{
	StandardVertexData vertices[] =
	{
		{  0.0f,    0.5f, 0.0f, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{  0.45f,  -0.5,  0.0f, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{  -0.45f, -0.5f, 0.0f, { 0.0f, 0.0f, 1.0f, 1.0f } }
	};

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DYNAMIC; // write access access by CPU and GPU
	bd.ByteWidth = sizeof(StandardVertexData) * 3;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;


	device->CreateBuffer(&bd, nullptr, &vertexBuffer);

	D3D11_MAPPED_SUBRESOURCE ms;
	context->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	memcpy(ms.pData, vertices, sizeof(vertices));
	context->Unmap(vertexBuffer, 0);
}
