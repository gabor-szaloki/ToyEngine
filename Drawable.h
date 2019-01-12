#pragma once

#include "Common.h"

__declspec(align(16)) class Drawable
{
public:

	XMMATRIX worldTransform;

	Drawable();
	virtual ~Drawable();

	void* operator new(size_t i) { return _mm_malloc(i, 16); }
	void operator delete(void* p) { _mm_free(p); }

	virtual void Init(ID3D11Device *device, ID3D11DeviceContext *context) = 0;
	virtual void Draw(ID3D11DeviceContext *context, ID3D11Buffer *perObjectCB);

protected:

	ID3D11Buffer *vertexBuffer;
	ID3D11Buffer *indexBuffer;
	UINT indexCount;

	void InitBuffers(ID3D11Device *device, ID3D11DeviceContext *context, 
		StandardVertexData *vertices, UINT vertexCount, WORD *indices, UINT indexCount);
};

