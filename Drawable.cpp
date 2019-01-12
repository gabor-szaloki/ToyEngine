#include "Drawable.h"

Drawable::Drawable()
{
}


Drawable::~Drawable()
{
	SAFE_RELEASE(vertexBuffer);
}

void Drawable::Draw(ID3D11DeviceContext *context)
{
	UINT stride = sizeof(StandardVertexData);
	UINT offset = 0;

	context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->Draw(3, 0);
}
