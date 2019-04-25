#include "Drawable.h"

#include "LodePNG/lodepng.h"

Drawable::Drawable()
{
	worldTransform = XMMatrixIdentity();
	material = nullptr;

	vertexBuffer = indexBuffer = nullptr;
	indexCount = 0;
}

Drawable::~Drawable()
{
	SAFE_RELEASE(vertexBuffer);
	SAFE_RELEASE(indexBuffer);
}

void Drawable::Draw(ID3D11DeviceContext *context, ID3D11Buffer *perObjectCB, bool shadowPass)
{
	UINT stride = sizeof(StandardVertexData);
	UINT offset = 0;

	context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	PerObjectConstantBufferData perObjectCBData;
	perObjectCBData.world = worldTransform;

	context->UpdateSubresource(perObjectCB, 0, nullptr, &perObjectCBData, 0, 0);
	context->VSSetConstantBuffers(1, 1, &perObjectCB);
	context->PSSetConstantBuffers(1, 1, &perObjectCB);

	material->SetToContext(context, shadowPass);

	context->DrawIndexed(indexCount, 0, 0);
}

void Drawable::InitBuffers(ID3D11Device *device, ID3D11DeviceContext *context, 
	StandardVertexData *vertices, UINT vertexCount, WORD *indices, UINT indexCount)
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
	bd.Usage = D3D11_USAGE_DYNAMIC; // write access access by CPU and GPU
	bd.ByteWidth = sizeof(StandardVertexData) * vertexCount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));
	initData.pSysMem = vertices;

	if (FAILED(device->CreateBuffer(&bd, &initData, &vertexBuffer)))
		throw;

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * indexCount;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	initData.pSysMem = indices;

	if (FAILED(device->CreateBuffer(&bd, &initData, &indexBuffer)))
		throw;

	this->indexCount = indexCount;
}
